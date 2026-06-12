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
#define SUN_IO_NOTIFY_LED_PENDING    (1u << 1)
#define SUN_IO_NOTIFY_RESET_PENDING  (1u << 2)
#define NOTIFY_SETTINGS_ENTER        (1u << 3)   /* 长按计时器烧到 → 进设置模式 */

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

/**
 * Request the keyboard LEDs be updated.
 *
 * Stores the HID LED bitmap (last write wins) and notifies the consumer
 * task via SUN_IO_NOTIFY_LED_PENDING. Safe to call from any context
 * (intended use: USB SET_REPORT callback).
 *
 * @param hid_led_bitmap HID Output report byte 0 (per HID 1.11):
 *                       bit 0 = NumLock, bit 1 = CapsLock,
 *                       bit 2 = ScrollLock, bit 3 = Compose.
 */
void sun_io_request_led(uint8_t hid_led_bitmap);

/**
 * Flush pending LED state to the keyboard via USART1 TX.
 *
 * Translates HID LED bits to Sun LED bits and transmits the
 * 2-byte command (0x0E + sun_led_byte). Blocking — at 1200 baud
 * the 2 bytes take ~16.7 ms.
 *
 * Called by sunKeyboardTask when SUN_IO_NOTIFY_LED_PENDING is observed
 * in the thread flags. Safe to call multiple times; each call sends
 * one command reflecting the latest requested state.
 */
void sun_io_flush_led(void);

/**
 * Directly set Sun LED Bitmap.
 * @param sun_bitmap 0=NumLock 1=Compose 2=ScrollLock 3=CapsLock 0x0F = full
 * 仅限任务上下文，阻塞式 UART 发送。
 */
void sun_io_set_raw_led(uint8_t sun_bitmap);

/**
 * Request a Sun reset command (0x01) be sent to the keyboard.
 *
 * Notifies the consumer task via SUN_IO_NOTIFY_RESET_PENDING.
 * Safe to call from any context including ISR — used from the
 * USER button EXTI handler.
 */
void sun_io_request_reset(void);

/**
 * Send the Sun reset command byte (0x01) over USART1 TX.
 *
 * Called by sunKeyboardTask when SUN_IO_NOTIFY_RESET_PENDING is
 * observed. Blocking; ~8.3 ms at 1200 baud.
 */
void sun_io_flush_reset(void);

void sun_io_bell_on(void);

void sun_io_bell_off(void);

void sun_io_click_on(void);

void sun_io_click_off(void);
#endif /* SUN_IO_H */