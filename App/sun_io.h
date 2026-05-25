// App/sun_io.h

#ifndef SUN_IO_H
#define SUN_IO_H

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os2.h"
#include "ring_buf.h"

/**
 * Notification flag bits used to wake the consumer task.
 * Bit-based so multiple event sources can multiplex onto
 * a single task notification (future: LED state change).
 */
#define SUN_IO_NOTIFY_RX_AVAILABLE   (1u << 0)
/* SUN_IO_NOTIFY_LED_PENDING reserved for Phase 2+ */

/**
 * @brief Initialize the Sun keyboard input subsystem.
 *
 * Performs three actions in order:
 *   1. Reset the internal ring buffer to empty.
 *   2. Record @p consumer_task for future RX notifications.
 *   3. Arm USART1 reception via @c HAL_UART_Receive_IT — from this
 *      point on, incoming bytes are captured, pushed to the ring
 *      buffer, and the consumer task is notified via
 *      @c SUN_IO_NOTIFY_RX_AVAILABLE.
 *
 * @param consumer_task The task to notify (via @c osThreadFlagsSet)
 *                      when a byte becomes available. Must be a valid
 *                      handle from a previously created task.
 * @return @c true on success; @c false if @c HAL_UART_Receive_IT
 *         failed (rare — indicates UART hardware busy or misconfigured).
 *
 * @pre USART1 must be initialized (CubeMX's @c MX_USART1_UART_Init
 *      has already run).
 * @pre @p consumer_task must already exist (@c osThreadNew returned
 *      a valid handle, NOT @c NULL).
 * @note Not re-entrant; not idempotent. Call exactly once during
 *       system init.
 * @see sun_io_get_byte
 * @see SUN_IO_NOTIFY_RX_AVAILABLE
 */
bool sun_io_init(osThreadId_t consumer_task);

/**
 * @brief Pop one received byte from the ring buffer.
 * @param out Where the byte is to store
 * @return False if no byte available
 * @note Must NOT be called from ISR?
 */
bool sun_io_get_byte(uint8_t *out);

/**
 * @brief Snapshot query if any bytes pending?
 *
 * Convenient for "drain everything" loops in the consumer task.
 * @return False if no data available
 */
bool sun_io_has_data(void);


#endif /* SUN_IO_H */