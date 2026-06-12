//
// Created by Cherry on 2026/6/12.
//

#ifndef CLICK_H
#define CLICK_H
#include <stdbool.h>
void click_init(void);          /* 把持久化的点击状态下发给键盘 */
void click_set(bool enabled);   /* 持久化 + 下发 */
#endif