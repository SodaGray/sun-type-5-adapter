//
// Created by Cherry on 2026/6/12.
//

#ifndef SETTINGS_H
#define SETTINGS_H

#include "sun_keymap.h"
#include "click.h"
#include "registry.h"

typedef enum {
    SETTINGS_CONTINUE,
    SETTINGS_ERROR,
    SETTINGS_DONE,
    SETTINGS_CANCEL,
} settings_result_t;

void settings_enter(void);

/* 设置模式下喂一次按键（MAKE）。scan_code 是 ev.data。 */
settings_result_t settings_key(uint8_t scan_code, sun_key_t k);

/* 设置模式下喂一次 all-keys-up。重映射的目标和弦在这里定格提交；
 * 其它情形一律忽略（返回 CONTINUE）。 */
settings_result_t settings_all_keys_up(void);

#endif //SETTINGS_H