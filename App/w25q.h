/*
 * w25q.h
 *
 * Driver for 8 MB JEDEC-standard SPI NOR flash.
 * Verified with EN25Q64 (EON, JEDEC 0x1C3017).
 * Also compatible with W25Q64 (Winbond), GD25Q64 (GigaDevice),
 * and other vendors implementing the standard JEDEC SPI NOR
 * command set (0x03 read, 0x02 program, 0x20 sector erase,
 * 0x06 write enable, 0x05 status, 0x9F JEDEC ID).
 *
 * Layered API:
 *   L2 (chip primitives): 1:1 with SPI commands, caller manages
 *                         alignment / WEL / busy_wait sequencing.
 *   L3 (easy-use):        arbitrary-length program/erase, all
 *                         alignment + waiting handled internally.
 *
 * Thread safety: NONE. All functions assume single-task access.
 * If multiple tasks use the flash, external mutex required.
 */

#ifndef W25Q_H
#define W25Q_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "projdefs.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"

/* ============================================================
 * Geometry constants
 * ============================================================ */

#define W25Q_PAGE_SIZE          256u
#define W25Q_SECTOR_SIZE        4096u
#define W25Q_BLOCK_SIZE         65536u
#define W25Q_CHIP_SIZE          (8u * 1024u * 1024u)        /* 8 MB */

#define W25Q_PAGE_COUNT         (W25Q_CHIP_SIZE / W25Q_PAGE_SIZE)
#define W25Q_SECTOR_COUNT       (W25Q_CHIP_SIZE / W25Q_SECTOR_SIZE)

/* ============================================================
 * Known JEDEC IDs for 8 MB SPI NOR chips we accept as equivalent.
 * Caller can compare against w25q_read_id() return value.
 * ============================================================ */

#define W25Q_JEDEC_WINBOND_W25Q64   0x00EF4017u
#define W25Q_JEDEC_EON_EN25Q64      0x001C3017u
#define W25Q_JEDEC_GIGADEVICE_64    0x00C84017u

/* ============================================================
 * Status Register 1 bits (read by w25q_read_status_reg1)
 * ============================================================ */

#define W25Q_SR1_BUSY               (1u << 0)   /* 1 = internal op in progress */
#define W25Q_SR1_WEL                (1u << 1)   /* 1 = write enable latched */

/* ============================================================
 * Suggested busy_wait timeouts (in milliseconds)
 *
 * Values are generous worst-case derived from W25Q64 datasheet
 * Tpp (page program), Tse (sector erase), Tbe (block erase),
 * Tce (chip erase). Use these when calling w25q_busy_wait.
 * ============================================================ */

#define W25Q_TIMEOUT_PAGE_PROGRAM_MS    10u
#define W25Q_TIMEOUT_SECTOR_ERASE_MS    500u
#define W25Q_TIMEOUT_BLOCK_ERASE_MS     3000u
#define W25Q_TIMEOUT_CHIP_ERASE_MS      60000u


/* ============================================================
 * Some pre-defined bytes
 *
 * Values from the chip datasheet.
 * ============================================================ */
#define W25Q_DUMMY_BYTE             0xFFu


/* ============================================================
 *  LAYER 2 — Chip-level primitives
 *
 *  Each function corresponds 1:1 with a SPI flash command (or a
 *  tight loop on status register).
 *
 *  CALLER RESPONSIBILITIES (across the layer):
 *    - serialize all calls (no concurrent access from multiple tasks)
 *    - respect alignment / length constraints documented per function
 *    - call w25q_write_enable() before every page_program / erase
 *    - call w25q_busy_wait() after every page_program / erase before
 *      issuing any further command other than w25q_read_status_reg1
 *
 *  All L2 functions release CS (set high) before returning, including
 *  on error paths.
 * ============================================================ */

/**
 * @brief Initialize driver-side state.
 *
 * Ensures CS is deasserted (high) to put the SPI bus in a known idle
 * state. Does not communicate with the chip; safe to call before the
 * chip's tVSL power-up time has elapsed.
 *
 * @note Caller is responsible for invoking MX_SPI2_Init() first.
 */
void w25q_init(void);

/**
 * @brief Read the JEDEC ID (RDID, 0x9F).
 *
 * Sends 0x9F then reads three ID bytes: manufacturer, memory type,
 * capacity. Suitable as a basic "is the chip alive and wired correctly"
 * check at startup.
 *
 * @return 24-bit ID packed as 0x00MMTTCC:
 *           MM = manufacturer ID (e.g. 0xEF Winbond, 0x1C EON)
 *           TT = memory type
 *           CC = capacity code (0x17 = 2^23 bytes = 8 MB)
 *         Returns 0xFFFFFFFF on SPI HAL error.
 */
uint32_t w25q_read_id(void);

/**
 * @brief Read Status Register 1 (RDSR1, 0x05).
 *
 * Safe to call at any time, including while a program or erase is
 * in progress (this is the only command guaranteed accepted during
 * BUSY).
 *
 * Bit layout (see W25Q_SR1_* macros):
 *   bit 0 BUSY  - 1 = internal program/erase in progress
 *   bit 1 WEL   - 1 = Write Enable Latch is set
 *   bits 2-7    - block protection / status protect (unused here)
 *
 * @return SR1 contents, or 0xFF on SPI HAL error (caller should
 *         treat 0xFF cautiously — it could be a real value indicating
 *         protection bits set).
 */
uint8_t w25q_read_status_reg1(void);

/**
 * @brief Block the calling task until BUSY clears.
 *
 * Polls SR1 in a loop, yielding 1 ms to other tasks between reads
 * via osDelay(1). The chip itself runs the operation in hardware;
 * this only blocks the caller, not the SPI peripheral.
 *
 * @param timeout_ms  maximum wait time in milliseconds. Suggested
 *                    constants: W25Q_TIMEOUT_PAGE_PROGRAM_MS,
 *                    W25Q_TIMEOUT_SECTOR_ERASE_MS, etc.
 * @return  true if BUSY cleared before timeout
 * @return  false on timeout (chip may still be busy; caller should
 *          treat the flash state as unknown and avoid further writes)
 *
 * @warning Must be called after every page_program and erase before
 *          issuing any further command other than w25q_read_status_reg1.
 */
bool w25q_busy_wait(uint32_t timeout_ms);

/**
 * @brief Set the Write Enable Latch (WREN, 0x06).
 *
 * The chip's WEL bit must be 1 for the next page program or erase
 * command to be accepted. WEL is automatically cleared by the chip
 * after the subsequent write/erase completes (whether successful or
 * not), so this must be called before each individual program/erase.
 *
 * @note No status check is performed. To verify WEL was set, the
 *       caller may read SR1 and check the WEL bit. In practice this
 *       command is reliable enough that explicit verification is
 *       overkill outside of bring-up debugging.
 */
void w25q_write_enable(void);

/**
 * @brief Read arbitrary-length data (READ DATA, 0x03).
 *
 * Issues 0x03 with a 24-bit address, then clocks out `len` bytes
 * into `buf`. The chip auto-increments the internal address pointer,
 * so reads may freely cross page and sector boundaries. CS stays
 * low for the entire transfer.
 *
 * @param addr  flash address to read from; must satisfy
 *              addr + len <= W25Q_CHIP_SIZE
 * @param buf   destination buffer (at least `len` bytes); must not be NULL
 *              unless len == 0
 * @param len   number of bytes to read; 0 is a no-op returning true
 *
 * @return true on success
 * @return false on invalid args (addr/len out of range, buf NULL) or
 *               SPI HAL error
 *
 * @warning The chip must not be BUSY when this is called. Caller is
 *          responsible for ensuring any prior program/erase has
 *          completed via w25q_busy_wait().
 */
bool w25q_read(uint32_t addr, uint8_t *buf, size_t len);

/**
 * @brief Program up to 256 bytes within a single page (PP, 0x02).
 *
 * Programs `len` bytes starting at `addr`. The chip can only flip
 * bits 1→0; the resulting byte at each location is the bitwise AND
 * of the existing flash content with the corresponding input byte.
 * For predictable behavior the target page should be erased to 0xFF
 * beforehand.
 *
 * @param addr  flash address of the first byte to program
 * @param buf   source data (at least `len` bytes); must not be NULL
 * @param len   number of bytes, 1..256
 *
 * @return true if the SPI transfer completed (does NOT confirm the
 *         internal program operation completed — see warning)
 * @return false on invalid args or SPI HAL error
 *
 * @warning Strict preconditions, all caller-enforced:
 *           - chip MUST NOT be busy
 *           - target page SHOULD be erased to 0xFF for correct content
 *           - len MUST be in [1, 256]
 *           - addr + len MUST fit within a single 256-byte page,
 *             i.e. (addr % 256) + len <= 256
 *           - addr + len MUST be <= W25Q_CHIP_SIZE
 *
 * @warning On return, the chip enters BUSY for ~700 µs typical,
 *          3 ms max. The caller MUST invoke w25q_busy_wait() before
 *          any other command (except w25q_read_status_reg1).
 * @note The function now processes the WEL.
 */
bool w25q_page_program(uint32_t addr, const uint8_t *buf, size_t len);

/**
 * @brief Erase one 4 KB sector (SE, 0x20).
 *
 * Sets all bytes in the sector containing `addr` to 0xFF. The chip
 * ignores the low 12 bits of the address — any address within the
 * sector targets the same sector.
 *
 * @param addr  any address within the sector to erase
 *              (must be < W25Q_CHIP_SIZE)
 *
 * @return true if SPI transfer completed
 * @return false on invalid args or SPI HAL error
 *
 * @warning Preconditions (caller-enforced):
 *           - chip MUST NOT be busy
 *
 * @warning On return, the chip enters BUSY for ~45 ms typical,
 *          400 ms max. Caller MUST invoke w25q_busy_wait() before
 *          any other command (except w25q_read_status_reg1).
 *
 * @note The function now processes the WEL.
 */
bool w25q_erase_sector(uint32_t addr);


/* ============================================================
 *  LAYER 3 — Easy-use wrappers
 *
 *  Built on top of L2. Handles alignment, splitting, WEL, and
 *  busy_wait internally. Returns only when the requested operation
 *  has fully completed (chip ready for next command).
 *
 *  Does NOT include semantic concepts (objects, CRC, versioning) —
 *  those belong to L4 storage layer.
 * ============================================================ */

/**
 * @brief Program arbitrary-length data into flash.
 *
 * Splits the input into ≤256-byte page-aligned chunks and programs
 * each one with the L2 sequence (write_enable → page_program →
 * busy_wait). Handles the case where the first byte is mid-page and
 * the last byte is mid-page.
 *
 * @param addr  start address in flash; addr + len <= W25Q_CHIP_SIZE
 * @param buf   source data (at least `len` bytes); must not be NULL
 *              unless len == 0
 * @param len   number of bytes to program; 0 is a no-op returning true
 *
 * @return true if every chunk programmed successfully and the chip
 *         reported ready (BUSY=0) after each
 * @return false if any chunk failed: SPI HAL error, busy_wait timeout,
 *         or invalid args
 *
 * @note Approximate duration: (len / 256) × 1 ms, dominated by
 *       inter-chunk busy_wait. For a full 4 KB sector worth of data
 *       this is ~16 ms.
 *
 * @warning The target region [addr, addr+len) MUST be erased to 0xFF
 *          before calling. This function only programs; it does not
 *          erase. Programming into un-erased flash produces a bitwise
 *          AND of old and new content — almost never what you want.
 *          Use w25q_erase_range() first to clear the target.
 */
bool w25q_program(uint32_t addr, const uint8_t *buf, size_t len);

/**
 * @brief Erase enough sectors to cover an arbitrary byte range.
 *
 * Computes the minimal set of 4 KB sectors that fully covers
 * [addr, addr+len) and erases each. Since erasure is per-sector,
 * the actual region cleared may extend beyond the requested range:
 *
 *   first_sector = addr & ~(W25Q_SECTOR_SIZE - 1)
 *   last_byte    = (addr + len - 1) | (W25Q_SECTOR_SIZE - 1)
 *
 * @param addr  start address (any value within chip)
 * @param len   range length in bytes; addr+len <= W25Q_CHIP_SIZE
 *
 * @return true if all covered sectors erased and chip is ready
 * @return false on SPI HAL error, busy_wait timeout, or invalid args
 *
 * @note Approximate duration: N_sectors × ~50 ms. Erasing a single
 *       sector takes ~50 ms; erasing 32 sectors (one 64 KB block
 *       worth) via this function takes ~1.6 s.
 *
 * @warning Bytes outside [addr, addr+len) but within the covered
 *          sectors WILL be erased. To preserve data adjacent to the
 *          target, place each logical object in its own sector(s).
 */
bool w25q_erase_range(uint32_t addr, size_t len);

#endif /* W25Q_H */