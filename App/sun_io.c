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

