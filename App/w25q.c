//
// Created by Cherry on 2026/5/27.
//

#include "w25q.h"
#include "main.h"   // W25Q_CS_Pin, W25Q_CS_GPIO_Port (CubeMX 生成的宏)
#include "spi.h"    // 暴露 hspi2 全局句柄（CubeMX 在 Core/Src/spi.c 生成）

static inline void w25q_cs_low(void)
{
    HAL_GPIO_WritePin(W25Q_CS_GPIO_Port, W25Q_CS_Pin, GPIO_PIN_RESET);
}

static inline void w25q_cs_high(void)
{
    HAL_GPIO_WritePin(W25Q_CS_GPIO_Port, W25Q_CS_Pin, GPIO_PIN_SET);
}

void w25q_init(void)
{
    w25q_cs_high();
}

uint32_t w25q_read_id(void)
{
    uint32_t id = 0;
    uint8_t rx[4] = {0};
    uint8_t tx[4] = {0x9F, 0, 0, 0};
    HAL_StatusTypeDef status;
    w25q_cs_low();
    status = HAL_SPI_TransmitReceive(&hspi2, tx, rx, 4, HAL_MAX_DELAY);
    w25q_cs_high();
    if (status != HAL_OK)
    {
        return 0xFFFFFFFF;
    }
    id = (uint32_t)rx[1] << 16 | (uint32_t)rx[2] << 8 | (uint32_t)rx[3];
    return id;
}