// App/sun_keymap.h

#ifndef SUN_KEYMAP_H
#define SUN_KEYMAP_H

#include <stdint.h>

/**
 * 一个 Sun 键的 HID 翻译结果。
 *
 * 三种情况：
 *   普通键：    hid_usage != 0, modifier_mask == 0
 *   修饰键：    hid_usage == 0, modifier_mask != 0
 *   未映射：    hid_usage == 0, modifier_mask == 0
 *
 * 永远不会两个字段都非零——物理键互斥。
 */
typedef struct {
    uint8_t hid_usage;       /* HID Keyboard Usage Page (0x07) 编码 */
    uint8_t modifier_mask;   /* HID report byte[0] 的位掩码 */
} sun_key_t;

/**
 * 查询一个 Sun scan code 的 HID 翻译。
 *
 * @param scan_code Sun scan code (0x01-0x7E 有效)
 * @return 翻译结果。范围外或未映射返回 {0, 0}。
 */
sun_key_t sun_keymap_lookup(uint8_t scan_code);

#endif