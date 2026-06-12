//
// Created by Cherry on 2026/6/12.
//

#include "settings.h"
#include "usb_mode.h"
#include "tusb.h"     /* HID_KEY_* */

typedef enum {
    ST_SELECT,         /* 等 F1..F12 选设置项 */
    ST_KBDMODE_VALUE,  /* 已选 F1：等 0/1 */
    ST_CLICK_VALUE,      /* 已选 F2：等 0/1 */
} settings_state_t;

static settings_state_t s_state;

void settings_enter(void) { s_state = ST_SELECT; }

settings_result_t settings_key(sun_key_t k)
{
    /* 空白键 = 全局取消，任何状态都认 */
    if (k.kind == SUN_KEY_KIND_INTERNAL) return SETTINGS_CANCEL;

    /* 静音键 = 切换我们的声音，任何状态都认 */
    if (k.kind == SUN_KEY_KIND_KEYBOARD && k.code == HID_USAGE_CONSUMER_MUTE) {
        if (registry()->feedback_sound) {        /* 开 → 关 */
            registry()->feedback_sound = 0;      /* 先设这个 */
            click_set(false);                    /* 静音时连点击一起关：F2→0 + 当场关 + 落盘 */
        } else {                                  /* 关 → 开 */
            registry()->feedback_sound = 1;      /* 只把提示音放回来，不动点击 */
            registry_save();
        }
        return SETTINGS_CONTINUE;
    }

    switch (s_state) {
    case ST_SELECT:
        if (k.kind == SUN_KEY_KIND_KEYBOARD && k.code == HID_KEY_F1) {
            s_state = ST_KBDMODE_VALUE;
            return SETTINGS_CONTINUE;       /* 选中键盘模式，静默进下一步 */
        }
        if (k.kind == SUN_KEY_KIND_KEYBOARD && k.code == HID_KEY_F2) {
            s_state = ST_CLICK_VALUE;
            return SETTINGS_CONTINUE;
        }
        /* 将来 F3..F12 在这里各自分支 */
        return SETTINGS_ERROR;              /* 顶层只认 F 键，其余都是误触 */

    case ST_KBDMODE_VALUE:
        bool one  = (k.code == HID_KEY_1 || k.code == HID_KEY_KEYPAD_1);
        bool zero = (k.code == HID_KEY_0 || k.code == HID_KEY_KEYPAD_0);
        if (k.kind == SUN_KEY_KIND_KEYBOARD && (one || zero)) {
            usb_mode_set(one ? USB_MODE_FULL : USB_MODE_BASIC);
            return SETTINGS_DONE;
        }
        return SETTINGS_ERROR;
    case ST_CLICK_VALUE: {
            bool one  = (k.code == HID_KEY_1 || k.code == HID_KEY_KEYPAD_1);
            bool zero = (k.code == HID_KEY_0 || k.code == HID_KEY_KEYPAD_0);
            if (k.kind == SUN_KEY_KIND_KEYBOARD && (one || zero)) {
                click_set(one);          /* 1 开，0 关 */
                return SETTINGS_DONE;
            }
            return SETTINGS_ERROR;
    }
    }
    return SETTINGS_ERROR;                  /* 防御 */
}