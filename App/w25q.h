//
// Created by Cherry on 2026/5/27.
//

#ifndef SUN_TYPE_5_ADAPTER_W25Q_H
#define SUN_TYPE_5_ADAPTER_W25Q_H
#include <stdint.h>

/**
 * Initialize W25Q64 driver state.
 * - Ensure CS is deasserted (high) at boot.
 * - (Future) might do bring-up checks like reading JEDEC ID.
 */
void w25q_init(void);

/**
 * Read the JEDEC ID from the chip.
 *
 * Returns the 3-byte ID packed into a uint32_t:
 *   bits 23-16 = manufacturer ID  (Winbond = 0xEF)
 *   bits 15-8  = memory type      (W25Q SPI NOR = 0x40)
 *   bits 7-0   = capacity code    (8 MB = 0x17, where 2^0x17 = 8 MB)
 *
 * Expected for W25Q64: 0x00EF4017
 * Returns 0xFFFFFFFF on SPI HAL error (chip absent / wiring fault).
 */
uint32_t w25q_read_id(void);

#endif //SUN_TYPE_5_ADAPTER_W25Q_H