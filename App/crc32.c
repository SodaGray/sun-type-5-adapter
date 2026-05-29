//
// Created by Cherry on 2026/5/29.
//

#include "crc32.h"

#include "crc.h"
#include "stm32l4xx_hal_crc.h"

uint32_t crc32_compute(const void *data, size_t len)
{
    return HAL_CRC_Calculate(&hcrc, (uint32_t*) data, len) ^ 0xFFFFFFFFu;
}
