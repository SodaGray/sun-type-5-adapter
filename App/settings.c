//
// Created by Cherry on 2026/6/12.
//

#include "settings.h"
#include "usb_mode.h"
#include "remap.h"
#include "tusb.h"
#include <string.h>

typedef enum {
    ST_SELECT,
    ST_KBDMODE_VALUE,
    ST_CLICK_VALUE,
    ST_REMAP_SOURCE,   /* 已选 F3：等被改的源键 */
    ST_REMAP_TARGET,   /* 已记源键：攒目标和弦，到 all-keys-up 为止 */
} settings_state_t;

static settings_state_t s_state;
static uint8_t          s_remap_source;   /* 被改键的扫描码 */
static remap_target_t   s_building;       /* 正在攒的目标和弦 */

void settings_enter(void) { s_state = ST_SELECT; }

settings_result_t settings_key(uint8_t scan_code, sun_key_t k)
{
    /* 空白键 = 全局取消 */
    if (k.kind == SUN_KEY_KIND_INTERNAL) return SETTINGS_CANCEL;

    /* 静音键 = 切换我们的声音 */
    if (k.kind == SUN_KEY_KIND_KEYBOARD && k.code == HID_USAGE_CONSUMER_MUTE) {
        if (registry()->feedback_sound) {
            registry()->feedback_sound = 0;
            click_set(false);
        } else {
            registry()->feedback_sound = 1;
            registry_save();
        }
        return SETTINGS_CONTINUE;
    }

    switch (s_state) {
    case ST_SELECT:
        if (k.kind == SUN_KEY_KIND_KEYBOARD && k.code == HID_KEY_F1) { s_state = ST_KBDMODE_VALUE; return SETTINGS_CONTINUE; }
        if (k.kind == SUN_KEY_KIND_KEYBOARD && k.code == HID_KEY_F2) { s_state = ST_CLICK_VALUE;   return SETTINGS_CONTINUE; }
        if (k.kind == SUN_KEY_KIND_KEYBOARD && k.code == HID_KEY_F3) { s_state = ST_REMAP_SOURCE;  return SETTINGS_CONTINUE; }
        return SETTINGS_ERROR;

    case ST_KBDMODE_VALUE: {
        bool one  = (k.code == HID_KEY_1 || k.code == HID_KEY_KEYPAD_1);
        bool zero = (k.code == HID_KEY_0 || k.code == HID_KEY_KEYPAD_0);
        if (k.kind == SUN_KEY_KIND_KEYBOARD && (one || zero)) {
            usb_mode_set(one ? USB_MODE_FULL : USB_MODE_BASIC);
            return SETTINGS_DONE;
        }
        return SETTINGS_ERROR;
    }

    case ST_CLICK_VALUE: {
        bool one  = (k.code == HID_KEY_1 || k.code == HID_KEY_KEYPAD_1);
        bool zero = (k.code == HID_KEY_0 || k.code == HID_KEY_KEYPAD_0);
        if (k.kind == SUN_KEY_KIND_KEYBOARD && (one || zero)) {
            click_set(one);
            return SETTINGS_DONE;
        }
        return SETTINGS_ERROR;
    }

    case ST_REMAP_SOURCE:
        /* 第一个按下的键 = 源键。修饰键不许当源；空白键已被上面的全局取消接走。 */
        if (k.kind == SUN_KEY_KIND_MODIFIER) return SETTINGS_ERROR;
        s_remap_source = scan_code;
        memset(&s_building, 0, sizeof(s_building));
        s_state = ST_REMAP_TARGET;
        return SETTINGS_CONTINUE;

    case ST_REMAP_TARGET:
        /* 攒目标：修饰键并进掩码，普通键进数组，其它种类算误触。提交在 all-keys-up。 */
        if (k.kind == SUN_KEY_KIND_MODIFIER) {
            s_building.modifiers |= (uint8_t)k.code;
            return SETTINGS_CONTINUE;
        }
        if (k.kind == SUN_KEY_KIND_KEYBOARD) {
            if (s_building.key_count < REMAP_MAX_KEYS)
                s_building.keys[s_building.key_count++] = (uint8_t)k.code;
            return SETTINGS_CONTINUE;
        }
        return SETTINGS_ERROR;
    }

    return SETTINGS_ERROR;
}

settings_result_t settings_all_keys_up(void)
{
    /* 只有在捕获目标、且已攒到东西时，all-keys-up 才意味着和弦录完；
     * 其它（含源键松手那次空 all-keys-up）一律忽略。 */
    if (s_state != ST_REMAP_TARGET) return SETTINGS_CONTINUE;
    if (s_building.modifiers == 0 && s_building.key_count == 0) return SETTINGS_CONTINUE;

    /* 映射为自身 = 删除：目标恰是源键默认的那一个普通键、无修饰。 */
    sun_key_t src = sun_keymap_lookup(s_remap_source);
    if (s_building.modifiers == 0 && s_building.key_count == 1 &&
        src.kind == SUN_KEY_KIND_KEYBOARD && s_building.keys[0] == (uint8_t)src.code) {
        remap_clear(s_remap_source);
    } else {
        remap_set(s_remap_source, &s_building);
    }
    return SETTINGS_DONE;
}