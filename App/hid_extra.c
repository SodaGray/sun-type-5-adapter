//
// App/hid_extra.c
//

#include "hid_extra.h"
#include "tusb.h"

/* Interface 1 承载 consumer + system 两个 collection */
#define INSTANCE         1u
#define REPORT_CONSUMER  1u
#define REPORT_SYSTEM    2u

/* System Control usage 范围（对应描述符 Usage Min/Max） */
#define SYSTEM_USAGE_MIN 0x81u   /* System Power Down */
#define SYSTEM_USAGE_MAX 0x83u   /* System Wake Up    */

/* ── Consumer ─────────────────────────────────────────────────────────────*/
/* 与 hid_keyboard 的 send_if_changed 同样的节奏：
 * 先 dedup；未挂载则只更新基线不发；否则等端点空闲后发，再更新基线。 */
static void consumer_send(hid_keyboard_state_t *s, uint16_t usage)
{
    if (usage == s->consumer_usage) return;

    if (!tud_mounted()) {
        s->consumer_usage = usage;
        return;
    }

    while (!tud_hid_n_ready(INSTANCE)) {
        osDelay(1);
    }

    uint8_t payload[2] = { (uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8) };
    tud_hid_n_report(INSTANCE, REPORT_CONSUMER, payload, sizeof(payload));

    s->consumer_usage = usage;
}

void hid_keyboard_press_consumer(hid_keyboard_state_t *s, uint16_t usage)
{
    consumer_send(s, usage);
}

void hid_keyboard_release_consumer(hid_keyboard_state_t *s)
{
    consumer_send(s, 0x0000);
}

void hid_keyboard_reset_consumer(hid_keyboard_state_t *s)
{
    consumer_send(s, 0x0000);
}

/* ── System ───────────────────────────────────────────────────────────────*/
static void system_send(hid_keyboard_state_t *s, uint8_t bitmap)
{
    if (bitmap == s->system_bitmap) return;

    if (!tud_mounted()) {
        s->system_bitmap = bitmap;
        return;
    }

    while (!tud_hid_n_ready(INSTANCE)) {
        osDelay(1);
    }

    tud_hid_n_report(INSTANCE, REPORT_SYSTEM, &bitmap, sizeof(bitmap));

    s->system_bitmap = bitmap;
}

void hid_keyboard_press_system(hid_keyboard_state_t *s, uint8_t usage_code)
{
    if (usage_code < SYSTEM_USAGE_MIN || usage_code > SYSTEM_USAGE_MAX) return;
    uint8_t bm = (uint8_t)(s->system_bitmap | (1u << (usage_code - SYSTEM_USAGE_MIN)));
    system_send(s, bm);
}

void hid_keyboard_release_system(hid_keyboard_state_t *s, uint8_t usage_code)
{
    if (usage_code < SYSTEM_USAGE_MIN || usage_code > SYSTEM_USAGE_MAX) return;
    uint8_t bm = (uint8_t)(s->system_bitmap & ~(1u << (usage_code - SYSTEM_USAGE_MIN)));
    system_send(s, bm);
}

void hid_keyboard_reset_system(hid_keyboard_state_t *s)
{
    system_send(s, 0);
}