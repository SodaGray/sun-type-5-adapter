//
// Created by Cherry on 2026/6/8.
//

#ifndef USB_MODE_H
#define USB_MODE_H

#endif //USB_MODE_H

#include "device/usbd.h"
#include "cmsis_os2.h"

/*
 * @brief reenumerate the USB device.
 *
 * connect then disconnect.
 */
void usb_reenumerate(void);