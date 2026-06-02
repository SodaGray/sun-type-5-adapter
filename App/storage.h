//
// Created by Cherry on 2026/5/31.
//

/*
 * storage.h
 *
 * L4 object store: persistent, power-fail-safe, wear-leveled storage
 * of typed objects on the SPI NOR flash.
 *
 * An "object" is identified by (type, instance) and holds an opaque
 * payload of 1..STORAGE_MAX_OBJECT_SIZE bytes, occupying one sector.
 * Writes are copy-on-write: a new sector is written, verified, and
 * committed before the old one is retired — atomic updates, crash safe.
 * Allocation prefers least-worn sectors (wear leveling); reclamation
 * of obsolete sectors (GC) is automatic and invisible to the caller.
 *
 * Layering: storage.c orchestrates sector_io (per-sector mechanism) and
 * crc32; it owns all in-RAM bookkeeping. The application sees only the
 * API below — never sectors, CRCs, GC, or wear leveling.
 *
 * Threading: single-task. Shared RAM tables + shared hardware CRC make
 * it unsafe for concurrent calls; wrap in a mutex if that changes.
 *
 * Lifecycle: call storage_mount() exactly once at boot, before anything
 * else here.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "sector_io.h"   /* MAX_PAYLOAD_SIZE, W25Q geometry */

/* ============================================================
 * Limits
 * ============================================================ */

/** Max payload bytes per object (one sector minus its 32-byte header). */
#define STORAGE_MAX_OBJECT_SIZE   MAX_PAYLOAD_SIZE   /* 4064 */

/** Max number of distinct live objects across all types (directory cap). */
#define STORAGE_MAX_LIVE_OBJECTS  256u

/* ============================================================
 * storage_load() return codes (>= 0 = bytes read; negative = failure)
 * ============================================================ */

#define STORAGE_ERR_NOT_FOUND   (-1)   /* no such (type, instance)        */
#define STORAGE_ERR_TOO_SMALL   (-2)   /* caller buffer smaller than payload */
#define STORAGE_ERR_CORRUPT     (-3)   /* payload CRC mismatch            */
#define STORAGE_ERR_IO          (-4)   /* flash read error                */

/* ============================================================
 * API
 * ============================================================ */

/**
 * @brief Scan the whole flash and rebuild the in-RAM storage state.
 *
 * Two-pass: pass 1 reads every sector's 8-byte preamble to fill the
 * erase-count table and coarse per-sector state; pass 2 reads the full
 * header of object sectors to build the live-object directory and
 * resolve crash-leftover conflicts (highest sequence wins).
 *
 * Best-effort: unreadable or corrupt sectors are flagged for reclaim
 * and skipped, never fatal. Payload CRCs are NOT checked here (deferred
 * to storage_load) so the scan stays header-only and fast (~50 ms,
 * hideable behind USB enumeration).
 *
 * Call exactly once at boot before any other storage_* call.
 *
 * @return true after the (best-effort) scan; false only on an
 *         unrecoverable init error.
 */
bool storage_mount(void);

/**
 * @brief Persist an object, atomically superseding any prior version
 *        of the same (type, instance).
 *
 * Copy-on-write: allocate a least-worn free sector, write header +
 * payload, verify both CRCs, commit, then retire the previous version.
 * On power loss at any step the previous version survives (or, if the
 * new one was already committed, it is chosen by sequence at next
 * mount). If no free sector exists, GC runs and allocation retries.
 *
 * @param type      object type id (caller's namespace)
 * @param instance  instance id within type
 * @param data      payload (must not be NULL)
 * @param len       1 .. STORAGE_MAX_OBJECT_SIZE
 * @return true on success; false on invalid args, no space after GC,
 *         or write/verify failure.
 */
bool storage_save(uint16_t type, uint16_t instance,
                  const void *data, size_t len);

/**
 * @brief Load the current version of an object into a caller buffer.
 *
 * Directory lookup, then the deferred payload CRC verification, then
 * copy out.
 *
 * @param type
 * @param instance
 * @param buf       destination (must not be NULL)
 * @param max_len   capacity of buf in bytes
 * @return >= 0 : number of payload bytes copied
 * @return STORAGE_ERR_* (negative) on failure; caller falls back to
 *         defaults.
 */
int storage_load(uint16_t type, uint16_t instance,
                 void *buf, size_t max_len);

/**
 * @brief Delete an object: retire its current version, drop it from the
 *        directory. Space reclaimed later by GC.
 * @return true if an object was deleted; false if none existed or on
 *         flash error.
 */
bool storage_delete(uint16_t type, uint16_t instance);

/**
 * @brief Count live instances of a given type.
 * @return number currently stored (>= 0).
 */
int storage_count(uint16_t type);

/**
 * @brief Get the instance id of the index-th object of a type, to
 *        iterate all instances (e.g. list every macro) without knowing
 *        ids in advance.
 *
 * Order is unspecified but stable as long as no save/delete occurs
 * between calls. Do not modify the store mid-iteration.
 *
 * @param type
 * @param index         0 .. storage_count(type)-1
 * @param instance_out  receives the instance id (must not be NULL)
 * @return true if found; false if index out of range.
 */
bool storage_enum(uint16_t type, size_t index, uint16_t *instance_out);

/**
 * @brief Print the mounted state to the debug console (directory
 *        entries, free count, min/max erase, next sequence). Bring-up
 *        aid; guard out or remove in production.
 */
void storage_debug_dump(void);

#endif /* STORAGE_H */