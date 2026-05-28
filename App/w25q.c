//
// Created by Cherry on 2026/5/27.
//

#include "w25q.h"
#include "main.h"   // W25Q_CS_Pin, W25Q_CS_GPIO_Port (CubeMX 生成的宏)
#include "spi.h"

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
    uint8_t tx[4] = {
        0x9F, W25Q_DUMMY_BYTE, W25Q_DUMMY_BYTE, W25Q_DUMMY_BYTE
    };
    w25q_cs_low();
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi2, tx, rx, 4, HAL_MAX_DELAY);
    w25q_cs_high();
    if (status != HAL_OK)
    {
        return 0xFFFFFFFF;
    }
    id = (uint32_t)rx[1] << 16 | (uint32_t)rx[2] << 8 | (uint32_t)rx[3];
    return id;
}

uint8_t w25q_read_status_reg1(void)
{
    uint8_t rx[2] = {0};
    uint8_t tx[2] = {0x05, W25Q_DUMMY_BYTE};
    w25q_cs_low();
    HAL_SPI_TransmitReceive(&hspi2, tx, rx, 2, HAL_MAX_DELAY);
    w25q_cs_high();
    return rx[1];
}

bool w25q_busy_wait(uint32_t timeout_ms)
{
    // 刚写完数据时，芯片可能在几十微秒内就搞定了。
    for (int retry = 0; retry < 100; retry++)
    {
        if ((w25q_read_status_reg1() & W25Q_SR1_BUSY) == 0) return true;
        __NOP(); // 硬件空操作，消耗一个时钟周期（12.5 纳秒）
    }

    uint32_t start_tick = osKernelGetTickCount();
    uint32_t timeout_ticks = pdMS_TO_TICKS(timeout_ms); // 兼容不同的系统节拍率

    while ((osKernelGetTickCount() - start_tick) < timeout_ticks)
    {
        if ((w25q_read_status_reg1() & W25Q_SR1_BUSY) == 0) return true;

        osDelay(1);
    }

    if ((w25q_read_status_reg1() & W25Q_SR1_BUSY) == 0) return true;

    return false;
}

void w25q_write_enable(void)
{
    uint8_t cmd = 0x06u;

    w25q_cs_low();

    HAL_SPI_Transmit(&hspi2, &cmd, 1, 5);

    w25q_cs_high();
}

bool w25q_read(uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return true;
    if (addr + len > W25Q_CHIP_SIZE) return false;
    if (buf == NULL) return false;

    uint8_t header[4];
    header[0] = 0x03;
    header[1] = (uint8_t)((addr >> 16) & 0xFFu);
    header[2] = (uint8_t)((addr >> 8)  & 0xFFu);
    header[3] = (uint8_t)(addr         & 0xFFu);

    w25q_cs_low();

    // 3. 吐出车头
    if (HAL_SPI_Transmit(&hspi2, header, 4, 5) != HAL_OK)
    {
        w25q_cs_high();
        return false;
    }

    if (HAL_SPI_Receive(&hspi2, buf, len, 5 + (len / 100)) != HAL_OK)
    {
        w25q_cs_high();
        return false;
    }

    w25q_cs_high();

    return true;
}

bool w25q_page_program(uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0 || len > 256 || buf == NULL) {
        return false;
    }

    if ((addr % W25Q_PAGE_SIZE) + len > W25Q_PAGE_SIZE) {
        return false;
    }

    w25q_write_enable();

    uint8_t header[4];
    header[0] = 0x02u;
    header[1] = (uint8_t)((addr >> 16) & 0xFFu); // A23 - A16
    header[2] = (uint8_t)((addr >> 8)  & 0xFFu); // A15 - A8
    header[3] = (uint8_t)(addr         & 0xFFu); // A7  - A0

    w25q_cs_low();

    if (HAL_SPI_Transmit(&hspi2, header, 4, 5) != HAL_OK) {
        w25q_cs_high();
        return false;
    }

    if (HAL_SPI_Transmit(&hspi2, (uint8_t *)buf, len, 10) != HAL_OK) {
        w25q_cs_high();
        return false;
    }

    w25q_cs_high();
    return true;
}

bool w25q_erase_sector(uint32_t addr)
{
    if (addr >= W25Q_CHIP_SIZE) return false;
    uint8_t header[4];
    header[0] = 0x20;
    header[1] = (uint8_t)((addr >> 16) & 0xFFu);
    header[2] = (uint8_t)((addr >> 8)  & 0xFFu);
    header[3] = (uint8_t)(addr         & 0xFFu);

    w25q_write_enable();

    w25q_cs_low();
    if (HAL_SPI_Transmit(&hspi2, header, 4, 5) != HAL_OK)
    {
        w25q_cs_high();
        return false;
    }

    w25q_cs_high();
    return true;
}

bool w25q_program(uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0) return true;
    if (buf == NULL) return false;
    if (addr + len > W25Q_CHIP_SIZE) return false;

    // 写入前等待可能的最大长度占用结束
    if (!w25q_busy_wait(W25Q_TIMEOUT_SECTOR_ERASE_MS)) return false;

    uint32_t current_addr = addr;
    const uint8_t *current_buf = buf;
    size_t remain_len = len;

    while (remain_len > 0)
    {
        // 计算当前地址所在的页还有多少字节可用
        size_t page_remain = W25Q_PAGE_SIZE - (current_addr % W25Q_PAGE_SIZE);

        // 如果剩余总数据量小于当前页可用空间，则只写剩余数据量
        size_t write_len = (remain_len < page_remain) ? remain_len : page_remain;

        // 调用单页写入函数
        if (!w25q_page_program(current_addr, current_buf, write_len)) return false;

        // 等待写入完成
        if (!w25q_busy_wait(W25Q_TIMEOUT_PAGE_PROGRAM_MS)) return false;

        // 动态偏移地址、数据指针，并扣减剩余长度
        current_addr += write_len;
        current_buf += write_len;
        remain_len -= write_len;
    }

    return true;
}

bool w25q_erase_range(uint32_t addr, size_t len)
{
    if (len == 0) return true;
    if (addr + len > W25Q_CHIP_SIZE) return false;

    if (!w25q_busy_wait(W25Q_TIMEOUT_SECTOR_ERASE_MS)) return false;

    // 计算范围所在的第一个和最后一个扇区的起始地址
    uint32_t first_sector_addr = addr & ~(W25Q_SECTOR_SIZE - 1);
    uint32_t last_byte_addr = addr + len - 1;
    uint32_t last_sector_addr = last_byte_addr & ~(W25Q_SECTOR_SIZE - 1);

    // 遍历擦除所有受影响的扇区
    for (uint32_t current_addr = first_sector_addr;
         current_addr <= last_sector_addr;
         current_addr += W25Q_SECTOR_SIZE)
    {
        if (!w25q_erase_sector(current_addr)) return false;
        if (!w25q_busy_wait(W25Q_TIMEOUT_SECTOR_ERASE_MS)) return false;
    }

    return true;
}