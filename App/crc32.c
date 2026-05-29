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

void crc32_begin(void)
{
    __HAL_CRC_DR_RESET(&hcrc);
}

void crc32_update(const void *data, size_t len)
{
    HAL_CRC_Accumulate(&hcrc, (uint32_t*)data, len);
}

uint32_t crc32_end(void)
{
    return hcrc.Instance->DR ^ 0xFFFFFFFFu;
}