// App/sun_io.h

#ifndef SUN_IO_H
#define SUN_IO_H

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os2.h"
#include "ring_buf.h"
#include "stm32l4xx_hal_uart.h"

/* Notification flag bits used to wake the consumer task.
 * Bit-based so multiple event sources can multiplex onto
 * a single task notification (future: LED state change). */
#define SUN_IO_NOTIFY_RX_AVAILABLE   (1u << 0)
/* SUN_IO_NOTIFY_LED_PENDING reserved for Phase 2+ */

/* Initialize Sun keyboard I/O:
 *  - reset internal ring buffer
 *  - record consumer task handle for notifications
 *  - start UART1 reception (interrupt-driven)
 *
 * Must be called AFTER consumer_task is created via osThreadNew.
 * Returns false if HAL_UART_Receive_IT fails. */
bool sun_io_init(osThreadId_t consumer_task);

/* Pop one received byte from the ring buffer.
 * Returns false if no byte available.
 * Intended to be called from the consumer task (sunKeyboardTask).
 * Must NOT be called from ISR. */
bool sun_io_get_byte(uint8_t *out);

/* Snapshot query: any bytes pending?
 * Convenient for "drain everything" loops in the consumer task. */
bool sun_io_has_data(void);

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* SUN_IO_H */