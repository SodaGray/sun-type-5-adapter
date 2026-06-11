//
// App/usb_mode.h
//
// USB 复合设备的运行时模式：
//   basic — 仅 interface 0（boot-capable NKRO 键盘），到处兼容
//   full  — interface 0 + interface 1（consumer + system control）
//
// 切换 = 翻转 current_mode + 重连，让主机重读配置描述符。
//

#ifndef USB_MODE_H
#define USB_MODE_H

typedef enum {
    USB_MODE_BASIC = 0,   /* 仅 interface 0：boot-capable NKRO 键盘 */
    USB_MODE_FULL  = 1,   /* interface 0 + interface 1（consumer + system） */
} usb_mode_t;

/* 设初始模式。当前为静态默认 BASIC；step 4 改为从存储层读上次的选择。
 * 必须在 USB 枚举之前调用（即 tusb_init 之前）。 */
void usb_mode_init(void);

/* 读当前模式。tud_descriptor_configuration_cb 用它决定呈现哪份配置描述符。 */
usb_mode_t usb_mode_get(void);

/* 切换模式：与当前相同则不动；不同则翻转并触发重连，让主机重新枚举。 */
void usb_mode_set(usb_mode_t mode);

#endif /* USB_MODE_H */