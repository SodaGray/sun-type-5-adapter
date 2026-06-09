//
// App/hid_extra.h
//
// Consumer Control (Usage Page 0x0C) 和 System Control (Usage Page 0x01 0x80+)
// 的发送接口，对应复合设备 interface 1（Report ID 1 / 2）。
//
// 状态复用 hid_keyboard_state_t，由调用方持有，与键盘共用同一个实例。
// 命名与调用方式跟 hid_keyboard_press_key 等保持一致。
//

#ifndef HID_EXTRA_H
#define HID_EXTRA_H

#include <stdint.h>
#include "hid_keyboard.h"

// ── Consumer Control (Report ID 1) ─────────────────────────────────────────
// 报文：[0x01][usage 低字节][usage 高字节]。一次报告一个 usage，0 = 松开。

/* 按下一个 consumer 键（e.g. HID_USAGE_CONSUMER_VOLUME_INCREMENT）。 */
void hid_keyboard_press_consumer(hid_keyboard_state_t *s, uint16_t usage);

/* 松开当前 consumer 键（发送 0x0000）。 */
void hid_keyboard_release_consumer(hid_keyboard_state_t *s);

/* 清空 consumer 状态（响应 ALL_KEYS_UP / RESET）。 */
void hid_keyboard_reset_consumer(hid_keyboard_state_t *s);

// ── System Control (Report ID 2) ───────────────────────────────────────────
// 报文：[0x02][bitmap]。bit 0/1/2 = Power Down(0x81)/Sleep(0x82)/Wake Up(0x83)。
// usage_code 传 HID_USAGE_DESKTOP_SYSTEM_POWER_DOWN / _SLEEP / _WAKE_UP。

/* 按下一个 system 键（在位图中置位并发送）。 */
void hid_keyboard_press_system(hid_keyboard_state_t *s, uint8_t usage_code);

/* 松开一个 system 键（在位图中清位并发送）。 */
void hid_keyboard_release_system(hid_keyboard_state_t *s, uint8_t usage_code);

/* 清空 system 状态（响应 ALL_KEYS_UP / RESET）。 */
void hid_keyboard_reset_system(hid_keyboard_state_t *s);

#endif /* HID_EXTRA_H */