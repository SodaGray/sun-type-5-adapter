//
// Created by Cherry on 2026/6/12.
//

#ifndef SETTINGS_H
#define SETTINGS_H

#include "sun_keymap.h"   /* sun_key_t */

/* 喂一个按键给状态机后的结果。调用方（键盘任务）据此做 LED 反馈、
 * 并决定留在还是离开设置模式。 */
typedef enum {
    SETTINGS_CONTINUE,   /* 有效输入，进了一步 —— 留下，无反馈        */
    SETTINGS_ERROR,      /* 意料外的键 —— 留下，快闪三次              */
    SETTINGS_DONE,       /* 设置已应用 —— 全亮 500ms，然后退出        */
    SETTINGS_CANCEL,     /* 空白键 —— 放弃更改，直接退出              */
} settings_result_t;

/* 把状态机复位到顶层（等待选择项）。每次进入设置模式调一次。 */
void settings_enter(void);

/* 设置模式下喂入一次按键（MAKE）。返回调用方该怎么做。
 * 松开（BREAK）不喂这里。 */
settings_result_t settings_key(sun_key_t k);

#endif //SETTINGS_H