//
// Created by Cherry on 2026/5/26.
//

// App/hid_keyboard.h

#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include <stdint.h>
#include "cmsis_os2.h"

#define PROTOCOL_BOOT   0
#define PROTOCOL_REPORT 1

/**
 * HID Boot Keyboard 状态。
 * Caller 拥有实例。详细设计见 hid_keyboard.c 注释。
 */
typedef struct {
    uint8_t modifier_byte;
    uint8_t key_bitmap[32];    /* 256-bit 位图：bit n = HID Usage n 是否按下 */
    uint8_t key_count;
    uint8_t last_report[21];
    uint16_t consumer_usage;
    uint8_t  system_bitmap;
} hid_keyboard_state_t;

void hid_keyboard_init(hid_keyboard_state_t *s);

/* 按下一个普通键（HID Keyboard/Keypad Usage Page）。 */
void hid_keyboard_press_key(hid_keyboard_state_t *s, uint8_t hid_usage);

/* 松开一个普通键。 */
void hid_keyboard_release_key(hid_keyboard_state_t *s, uint8_t hid_usage);

/* 按下修饰键（mask 是 modifier 位的位掩码，e.g. 0x01 = LCtrl）。 */
void hid_keyboard_press_modifier(hid_keyboard_state_t *s, uint8_t mask);

/* 松开修饰键。 */
void hid_keyboard_release_modifier(hid_keyboard_state_t *s, uint8_t mask);

/* 清空所有按键状态并发送空报告。响应 ALL_KEYS_UP / RESET。 */
void hid_keyboard_reset(hid_keyboard_state_t *s);

#endif
