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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) return;
    ring_buf_push(&rb, rx_byte);
    osThreadFlagsSet(recorded_consumer_task, SUN_IO_NOTIFY_RX_AVAILABLE);
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

