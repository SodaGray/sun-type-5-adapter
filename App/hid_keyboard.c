//
// Created by Cherry on 2026/5/26.
//

// App/hid_keyboard.c

#include "hid_keyboard.h"
#include <string.h>
#include "tusb.h"

/* 前向声明：所有 state 修改后调一遍，必要时发送 */
static void send_if_changed(hid_keyboard_state_t *s);

void hid_keyboard_init(hid_keyboard_state_t *s)
{
    memset(s, 0, sizeof(*s));
}

void hid_keyboard_press_key(hid_keyboard_state_t *s, uint8_t hid_usage)
{
    uint8_t byte_idx = hid_usage / 8; // 计算在数组的第几个字节
    uint8_t bit_idx  = hid_usage % 8; // 计算在该字节的第几个二进制位

    // 检查该键之前是否未被按下（对应位为 0）
    if (!(s->key_bitmap[byte_idx] & (1 << bit_idx)))
    {
        s->key_bitmap[byte_idx] |= (1 << bit_idx); // 将该位置 1（标记为按下）
        s->key_count++;
    }
    send_if_changed(s);
}
void hid_keyboard_release_key(hid_keyboard_state_t *s, uint8_t hid_usage)
{
    uint8_t byte_idx = hid_usage / 8;
    uint8_t bit_idx  = hid_usage % 8;

    /* 只在位为 1（键真的按着）时才动作 */
    if (s->key_bitmap[byte_idx] & (1 << bit_idx)) {
        s->key_bitmap[byte_idx] &= ~(1 << bit_idx);   /* 清那一位 */
        s->key_count--;
    }
    send_if_changed(s);
}
void hid_keyboard_press_modifier(hid_keyboard_state_t *s, uint8_t mask)
{
    s->modifier_byte |= mask;
    send_if_changed(s);
}
void hid_keyboard_release_modifier(hid_keyboard_state_t *s, uint8_t mask)
{
    s->modifier_byte &= ~mask;
    send_if_changed(s);
}
void hid_keyboard_reset(hid_keyboard_state_t *s)
{
    s->modifier_byte = 0;
    memset(s->key_bitmap, 0, sizeof(s->key_bitmap));
    s->key_count = 0;
    send_if_changed(s);
}

static void send_if_changed(hid_keyboard_state_t *s)
{
    uint8_t report[8] = {0};
    report[0] = s->modifier_byte;
    /* report[1] 保留字节，永远 0 */

    if (s->key_count > 6) {
        /* Phantom state: 6 个槽全填 ErrorRollOver (0x01) */
        for (int i = 2; i < 8; i++) {
            report[i] = 0x01;
        }
    } else {
        /* 扫位图，取前 6 个按下的键填进 byte[2..7] */
        int slot = 2;
        for (int usage = 0; usage < 256 && slot < 8; usage++) {
            if (s->key_bitmap[usage / 8] & (1 << (usage % 8))) {
                report[slot++] = usage;
            }
        }
    }

    /* Dedup：与上次发出的对比 */
    if (memcmp(report, s->last_report, 8) == 0) {
        return;
    }

    /* 通过 TinyUSB 发出。fire-and-forget，不查返回值。 */
    tud_hid_keyboard_report(0, report[0], &report[2]);

    /* 更新 dedup 基线 */
    memcpy(s->last_report, report, 8);
}