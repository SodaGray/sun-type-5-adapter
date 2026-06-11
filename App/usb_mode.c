//
// App/usb_mode.c
//

#include "usb_mode.h"
#include "tusb.h"
#include "cmsis_os2.h"

static usb_mode_t s_mode = USB_MODE_BASIC;

/* 重连原语：软断开 → 等主机察觉 → 重连，逼主机重新枚举。
 *
 * 必须从任务上下文调用（osDelay 需要任务环境），不能在 ISR 里。
 * tud_connect 之后主机会重新枚举，靠 tud_task() 处理——调用方返回到
 * 平时跑 tud_task 的循环即可，本函数不依赖它。 */
static void usb_reenumerate(void)
{
    tud_disconnect();
    osDelay(50);        /* 等主机察觉断开；某些主机不重枚举可加到 100-200 */
    tud_connect();
}

void usb_mode_init(void)
{
    s_mode = USB_MODE_BASIC;   /* step 4: 改为从存储层读上次的选择 */
}

usb_mode_t usb_mode_get(void)
{
    return s_mode;
}

void usb_mode_set(usb_mode_t mode)
{
    if (mode == s_mode) return;   /* 没变，不动，不必打扰主机 */
    s_mode = mode;                /* 先翻转：重连后回调读到的就是新值 */
    usb_reenumerate();
}