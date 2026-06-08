// App/sun_keymap.h

#ifndef SUN_KEYMAP_H
#define SUN_KEYMAP_H

#include <stdint.h>
#include "class/hid/hid.h"

/**
 * Sun scan code 映射到 HID 的语义种类。
 *
 * 不同种类派发到不同模块、不同 HID Usage Page、不同报告格式。
 *
 * 当前只 KEYBOARD 和 MODIFIER 有对应实现；CONSUMER 和 SYSTEM 是
 * 架构占位，将来按需添加 hid_consumer.c / hid_system.c 模块，同时
 * 扩展 USB HID Report Descriptor 添加对应 Report ID。
 */
typedef enum {
    SUN_KEY_KIND_NONE,      /* 未映射；调度器静默忽略 */
    SUN_KEY_KIND_KEYBOARD,  /* HID Keyboard/Keypad Page 0x07；code = 8-bit Usage */
    SUN_KEY_KIND_MODIFIER,  /* Keyboard 报告 byte[0]；code = 位掩码 (1 << bit) */
    SUN_KEY_KIND_CONSUMER,  /* HID Consumer Control Page 0x0C；code = 16-bit Usage。未实现 */
    SUN_KEY_KIND_SYSTEM,    /* HID Generic Desktop System Control；code = Usage。未实现 */
} sun_key_kind_t;

/**
 * Sun scan code 的 HID 翻译结果。
 *
 * code 的解读由 kind 决定：
 *   NONE      → code 不使用
 *   KEYBOARD  → code = 8-bit HID Keyboard/Keypad Usage
 *   MODIFIER  → code = 8-bit modifier bitmask（bit 0=LCtrl ... bit 7=RGui）
 *   CONSUMER  → code = 16-bit Consumer Control Usage
 *   SYSTEM    → code = System Control Usage
 *
 * 用 uint16_t 容纳 16-bit Consumer Usage 空间。KEYBOARD/MODIFIER 实际只用低 8 位。
 */
typedef struct {
    sun_key_kind_t kind;
    uint16_t code;
} sun_key_t;

sun_key_t sun_keymap_lookup(uint8_t scan_code);

#endif