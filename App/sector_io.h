//
// Created by Cherry on 2026/5/29.
//

/*
 * sector_io.h
 *
 * On-flash layout + low-level sector operations for the L4 object store.
 * Sits on w25q.h (L2/L3) and crc32.h.
 *
 * THREE sector states, distinguished by the first 8 bytes:
 *   OBJECT:  magic == OBJ_MAGIC, full 32-byte header follows
 *   FREE:    magic == 0xFFFFFFFF AND erase_count != 0xFFFFFFFF
 *   VIRGIN:  magic == 0xFFFFFFFF AND erase_count == 0xFFFFFFFF (count = 0)
 *   RECLAIM: tombstoned or corrupt or anything else
 *
 * CRITICAL invariant — the FREE→OBJECT transition must be pure 1→0
 * (no erase). This is why:
 *   - a FREE sector writes ONLY erase_count at offset 4..7 and leaves
 *     offset 0..3 = 0xFFFFFFFF, offset 8.. = 0xFF
 *   - the object header's magic (offset 0) is reachable by clearing bits
 *     from 0xFFFFFFFF, and its erase_count (offset 4) is the SAME value
 *     already present, so writing the full header never needs a 0->1 flip
 * Do NOT introduce any field written in the FREE state other than
 * erase_count, or you re-break this invariant.
 *
 * On-flash object sector layout (little-endian, packed):
 *   off  sz  field          in header_crc[0..25]?
 *   0    4   magic           yes
 *   4    4   erase_count     yes
 *   8    2   type            yes
 *   10   2   instance        yes
 *   12   4   sequence        yes
 *   16   2   payload_len     yes
 *   18   4   reserved        yes  (=0xFFFFFFFF, future FIXED fields)
 *   22   4   payload_crc     yes
 *   26   4   header_crc      --   (covers bytes [0..25])
 *   30   1   state_byte      no   (mutable: 0xFF->0x7F->0x3F)
 *   31   1   pad             no   (0xFF, future MUTABLE flags)
 *   32   ..  payload (<= 4064 bytes)
 *
 * Field widths verified against chip lifetime: erase_count and sequence
 * are u32 and provably cannot overflow before the chip's ~100k-cycle
 * endurance is exhausted (see project notes). u16 would be insufficient
 * for both.
 */

#ifndef SECTOR_IO_H
#define SECTOR_IO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "w25q.h"

/* ============================================================
 * Magic
 * ============================================================ */

#define SECTOR_MAGIC_OBJ        0x4B424453u   /* "KBDS" */
#define SECTOR_MAGIC_BLANK      0xFFFFFFFFu   /* erased flash; virgin or free */
#define SECTOR_MAGIC_DEAD       0x00000000u   /* tombstone: written before erase */
#define ERASE_COUNT_BLANK       0xFFFFFFFFu   /* unwritten count = virgin */

/* ============================================================
 * State byte (offset 30). Transitions clear bits only (1->0).
 * ============================================================ */

#define STATE_FRESH        0xFFu   /* uninitialized / writing in progress */
#define STATE_COMMITTED    0x7Fu   /* bit 7 cleared = committed (valid)  */
#define STATE_OBSOLETE     0x3Fu   /* bits 7,6 cleared = committed+superseded */

/* Helpers for interpreting a state byte (pure, no I/O). */
#define STATE_IS_COMMITTED(s)   (((s) & 0x80u) == 0u)               /* bit7 cleared */
#define STATE_IS_OBSOLETE(s)    (((s) & 0x40u) == 0u)               /* bit6 cleared */
#define STATE_IS_LIVE(s)        (STATE_IS_COMMITTED(s) && !STATE_IS_OBSOLETE(s))
/* Illegal combo: bit6 cleared but bit7 set ("obsolete-but-uncommitted") -> corrupt */
#define STATE_IS_CORRUPT(s)     ((((s) & 0x80u) != 0u) && (((s) & 0x40u) == 0u))

/* ============================================================
 * Layout
 * ============================================================ */

#define OBJ_HEADER_SIZE        32u
#define MAX_PAYLOAD_SIZE       (W25Q_SECTOR_SIZE - OBJ_HEADER_SIZE)  /* 4064 */

#define OFFSET_MAGIC           0u
#define OFFSET_ERASE_COUNT     4u
#define OFFSET_TYPE            8u
#define OFFSET_INSTANCE        10u
#define OFFSET_SEQUENCE        12u
#define OFFSET_PAYLOAD_LEN     16u
#define OFFSET_RESERVED        18u
#define OFFSET_PAYLOAD_CRC     22u
#define OFFSET_HEADER_CRC      26u
#define OFFSET_STATE_BYTE      30u
#define OFFSET_PAD             31u
#define OFFSET_PAYLOAD         32u

#define HEADER_CRC_RANGE_START 0u
#define HEADER_CRC_RANGE_LEN   26u   /* bytes [0..25], magic .. payload_crc */

#define RESERVED_DEFAULT       0xFFFFFFFFu  /* written into reserved field */

/* ============================================================
 * Types
 * ============================================================ */

typedef struct __attribute__((packed)) {
    uint32_t magic;            /* SECTOR_MAGIC_OBJ */
    uint32_t erase_count;      /* erases this sector has seen (u32: never overflows) */
    uint16_t type;
    uint16_t instance;
    uint32_t sequence;         /* global monotonic (u32: never overflows) */
    uint16_t payload_len;      /* 1 .. MAX_PAYLOAD_SIZE */
    uint16_t reserved_lo;      /* part of 4-byte reserved; = 0xFFFF */
    uint16_t reserved_hi;      /* part of 4-byte reserved; = 0xFFFF */
    uint32_t payload_crc;      /* CRC32 of payload */
    uint32_t header_crc;       /* CRC32 of bytes [0..25] */
    uint8_t  state_byte;       /* STATE_FRESH / COMMITTED / OBSOLETE */
    uint8_t  pad;              /* 0xFF */
} obj_header_t;

_Static_assert(sizeof(obj_header_t) == OBJ_HEADER_SIZE,
               "obj_header_t must be exactly 32 bytes");

typedef enum {
    SECTOR_STATE_VIRGIN,    /* never written; erase_count = 0 */
    SECTOR_STATE_FREE,      /* erased, has valid erase_count */
    SECTOR_STATE_OBJECT,    /* magic == OBJ_MAGIC; inspect header for live/obsolete */
    SECTOR_STATE_RECLAIM,   /* tombstoned / corrupt / mid-erase residue → GC erase */
} sector_state_t;

/* ============================================================
 * Preamble (universal first 8 bytes) + classification
 * ============================================================ */

/**
 * @brief Read the universal preamble (magic + erase_count) from a sector.
 *
 * What the boot scan calls in pass 1 for every sector.
 *
 * @param sector_idx  0 .. W25Q_SECTOR_COUNT-1
 * @param magic_out   receives the magic field
 * @param count_out   receives the raw erase_count field. For a VIRGIN
 *                    sector this reads ERASE_COUNT_BLANK (0xFFFFFFFF);
 *                    callers should map that to a logical count of 0.
 * @return true on success, false on flash read error.
 */
bool sector_read_preamble(uint32_t sector_idx,
                          uint32_t *magic_out,
                          uint32_t *count_out);

/**
 * @brief Classify a sector from its preamble (pure function, no I/O).
 *
 * @param magic   magic field from sector_read_preamble
 * @param count   erase_count field from sector_read_preamble
 * @return VIRGIN / FREE / OBJECT. OBJECT still requires reading the
 *         full header to determine live vs obsolete vs corrupt.
 */
sector_state_t sector_classify(uint32_t magic, uint32_t count);

/**
 * @brief Logical erase count for a sector given its raw preamble.
 *
 * Maps a VIRGIN sector's 0xFFFFFFFF to 0; otherwise returns count.
 */
uint32_t sector_logical_count(uint32_t magic, uint32_t count);

/* ============================================================
 * Object sector — read side
 * ============================================================ */

/**
 * @brief Read the full 32-byte object header into RAM.
 *        Does not validate CRC or magic. Caller verifies.
 * @return true on success, false on flash read error.
 */
bool sector_read_obj_header(uint32_t sector_idx, obj_header_t *header_out);

/**
 * @brief Read payload bytes from an object sector.
 *        Reads len bytes at (sector base + OFFSET_PAYLOAD + offset_in_payload).
 *        Caller ensures offset_in_payload + len <= payload_len.
 * @return true on success, false on read error or invalid args.
 */
bool sector_read_payload(uint32_t sector_idx,
                         size_t offset_in_payload,
                         uint8_t *buf, size_t len);

/**
 * @brief Verify an on-flash object end-to-end.
 *
 * Reads the header, recomputes CRC32 over bytes [0..25] and compares
 * with header.header_crc; then reads the full payload, recomputes
 * CRC32 and compares with header.payload_crc.
 *
 * Used (a) right after sector_write_object before commit, and
 * (b) during storage_load for the lazy payload verification deferred
 * from boot scan.
 *
 * @return true iff both CRCs match the stored values
 * @return false on read error or any CRC mismatch (treat as corrupt)
 */
bool sector_verify_object(uint32_t sector_idx);

/* ============================================================
 * Object sector — write side
 * ============================================================ */

/**
 * @brief Write a new object into a FREE or VIRGIN sector.
 *
 * Builds the 32-byte header in RAM with:
 *   magic = OBJ_MAGIC, erase_count = (passed in), type, instance,
 *   sequence, payload_len, reserved = 0xFFFFFFFF, payload_crc and
 *   header_crc (computed via crc32_compute), state_byte = STATE_FRESH,
 *   pad = 0xFF.
 * Then programs header + payload via w25q_program.
 *
 * Sector MUST already be erased (FREE or VIRGIN). Does NOT erase and
 * does NOT commit. Caller (storage layer) must:
 *   1. pick an erased sector and pass its current erase_count
 *      (VIRGIN -> pass 0)
 *   2. call sector_verify_object after this returns
 *   3. call sector_commit only if verify succeeded
 *
 * @param erase_count  the sector's current count, recorded into the
 *                     header so boot scan recovers it. For a FREE
 *                     sector this equals the value already at offset 4
 *                     (so the write is a bit-level no-op there); for a
 *                     VIRGIN sector pass 0.
 * @param payload_len  1 .. MAX_PAYLOAD_SIZE
 * @return true if header+payload programmed; false on invalid args
 *         or flash write error.
 */
bool sector_write_object(uint32_t sector_idx,
                         uint16_t type, uint16_t instance,
                         uint32_t sequence, uint32_t erase_count,
                         const uint8_t *payload, size_t payload_len);

/**
 * @brief Commit: clear bit 7 of state_byte (STATE_FRESH -> STATE_COMMITTED).
 *        Single-byte program at OFFSET_STATE_BYTE. The atomic commit point.
 * @return true on success, false on flash error.
 */
bool sector_commit(uint32_t sector_idx);

/**
 * @brief Mark obsolete: clear bit 6 of state_byte (COMMITTED -> OBSOLETE).
 *        Called when a newer version of this (type,instance) is committed.
 *        (Obsolescence is also derivable from sequence comparison; this
 *        marker is a GC-acceleration cache.)
 * @return true on success, false on flash error.
 */
bool sector_mark_obsolete(uint32_t sector_idx);

/* ============================================================
 * GC / recycle
 * ============================================================ */

/**
 * @brief Reclaim a sector: erase, then record the new erase_count.
 *
 *   1. tombstone before erase
 *   2. w25q_erase_range over the whole sector (the wear event)
 *   3. program ONLY offset 4..7 with new_erase_count
 *
 * Leaves offset 0..3 = 0xFFFFFFFF and offset 8.. = 0xFF, producing a
 * valid FREE sector that a later object write can overwrite with pure
 * 1->0 transitions.
 *
 * @param new_erase_count  the post-increment count (old count + 1).
 *                         Caller is responsible for the increment and
 *                         for not passing ERASE_COUNT_BLANK (which would
 *                         make the sector look VIRGIN); given the field
 *                         is u32 and the chip dies near 1e5 erases, this
 *                         is never a practical concern.
 * @return true on success, false on flash erase/program error.
 */
bool sector_recycle(uint32_t sector_idx, uint32_t new_erase_count);

#endif /* SECTOR_IO_H */