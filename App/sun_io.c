//
// Created by Cherry on 2026/5/25.
//

#include "sun_io.h"

#include "usart.h"


/* Initialize Sun keyboard I/O:
 *  - reset internal ring buffer
 *  - record consumer task handle for notifications
 *  - start UART1 reception (interrupt-driven)
 *
 * Must be called AFTER consumer_task is created via osThreadNew.
 * Returns false if HAL_UART_Receive_IT fails. */

static ring_buf_t rb;
static osThreadId_t recorded_consumer_task;
static uint8_t rx_byte;

static volatile uint8_t pending_led;   /* 由 USB callback 写、task 读 */

bool sun_io_init(osThreadId_t consumer_task)
{
    ring_buf_init(&rb);
    recorded_consumer_task = consumer_task;
    if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1) != HAL_OK) return false;
    return true;
}

/* Pop one received byte from the ring buffer.
 * Returns false if no byte available.
 * Intended to be called from the consumer task (sunKeyboardTask).
 * Must NOT be called from ISR. */
bool sun_io_get_byte(uint8_t *out)
{
    return ring_buf_pop(&rb, out);
}

/* Snapshot query: any bytes pending?
 * Convenient for "drain everything" loops in the consumer task. */
bool sun_io_has_data(void)
{
    return !ring_buf_is_empty(&rb);
}

/**
 * @brief HAL UART RX complete callback — Sun keyboard byte handler.
 *
 * Overrides HAL's __weak function of the same name. HAL's USART ISR
 * invokes this after each completed receive (length=1, armed by
 * sun_io_init and re-armed at the end of this callback).
 *
 * The callback is *global* across all UARTs configured in the system,
 * so this implementation filters by @c huart->Instance and acts only
 * for USART1.
 *
 * Runs in ISR context. Four-step sequence:
 *   1. Filter for USART1 (other UARTs may share this callback).
 *   2. Push the received byte into the SPSC ring buffer.
 *   3. Signal the consumer task via @c osThreadFlagsSet (ISR-safe
 *      CMSIS-RTOS V2 wrapper; internally yields if a higher-priority
 *      task is woken).
 *   4. Re-arm @c HAL_UART_Receive_IT for the next byte — HAL receive
 *      is one-shot; without this, reception silently stops after the
 *      first byte.
 *
 * @param huart The UART handle that triggered the callback. May be any
 *              UART; this function only acts for &huart1.
 *
 * @warning Step 4 (re-arm) is the most common omission in this pattern.
 *          Symptom: exactly one byte received, then permanent silence.
 * @note Do not add lengthy work — ISR context. All real processing
 *       happens in the consumer task.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) return;
    ring_buf_push(&rb, rx_byte);
    osThreadFlagsSet(recorded_consumer_task, SUN_IO_NOTIFY_RX_AVAILABLE);
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

void sun_io_request_led(uint8_t hid_led_bitmap)
{
    pending_led = hid_led_bitmap;
    osThreadFlagsSet(recorded_consumer_task, SUN_IO_NOTIFY_LED_PENDING);
}

void sun_io_flush_led(void)
{
    uint8_t hid = pending_led;

    /* HID → Sun bit 重新排列 */
    uint8_t sun_led = 0;
    if (hid & 0x01) sun_led |= 0x01;   /* NumLock     HID 0 → Sun 0 */
    if (hid & 0x02) sun_led |= 0x08;   /* CapsLock    HID 1 → Sun 3 */
    if (hid & 0x04) sun_led |= 0x04;   /* ScrollLock  HID 2 → Sun 2 */
    if (hid & 0x08) sun_led |= 0x02;   /* Compose     HID 3 → Sun 1 */

    /* 发送 2 字节命令到 USART1（反相已硬件配好，不用关心） */
    uint8_t cmd[2] = { 0x0E, sun_led };
    HAL_UART_Transmit(&huart1, cmd, 2, HAL_MAX_DELAY);
}

void sun_io_set_raw_led(uint8_t sun_bitmap)
{
    uint8_t cmd[2] = { 0x0E, sun_bitmap };
    HAL_UART_Transmit(&huart1, cmd, 2, HAL_MAX_DELAY);
}

void sun_io_request_reset(void)
{
    osThreadFlagsSet(recorded_consumer_task, SUN_IO_NOTIFY_RESET_PENDING);
}

void sun_io_flush_reset(void)
{
    HAL_UART_Transmit(&huart1, &(uint8_t){0x01}, 1, HAL_MAX_DELAY);
}