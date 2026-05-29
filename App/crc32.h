//
// Created by Cherry on 2026/5/29.
//

/*
 * crc32.h
 *
 * Thin wrapper over the STM32L4 hardware CRC unit, producing
 * standard CRC32 (zlib / IEEE 802.3 / PNG):
 *   polynomial   0x04C11DB7
 *   init         0xFFFFFFFF
 *   input        bit-reflected per byte (hardware)
 *   output       bit-reflected (hardware)
 *   final xor    0xFFFFFFFF (software, applied here)
 *
 * Result is byte-for-byte compatible with:
 *   - Python: binascii.crc32(data)
 *   - command line: crc32 <file>
 *   - PNG/Ethernet/zlib reference implementations
 *
 * Threading: NOT safe to call from ISR. NOT safe to call
 *            concurrently from multiple tasks — the hardware
 *            unit has a single shared accumulator. In our
 *            project storage is single-task; if that changes,
 *            wrap calls in an osMutex.
 */

#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Compute the standard CRC32 of a byte buffer.
 *
 * Resets the hardware CRC accumulator to 0xFFFFFFFF, feeds the
 * buffer byte by byte through the hardware (which handles input
 * reflection per byte), reads the reflected output, and XORs
 * with 0xFFFFFFFF to produce the final standard CRC32 value.
 *
 * @param data  pointer to data buffer; if NULL or len==0, returns 0
 * @param len   number of bytes to feed
 * @return      standard CRC32 of the data
 *
 * @note Empty input returns 0 (not 0xFFFFFFFF^0xFFFFFFFF=0, but
 *       0 by convention for "nothing to check"). This matches
 *       Python's binascii.crc32(b'') == 0.
 *
 * @warning See file header re: ISR / concurrency restrictions.
 */
uint32_t crc32_compute(const void *data, size_t len);

#endif /* CRC32_H */