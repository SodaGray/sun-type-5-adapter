//
// Created by Cherry on 2026/6/8.
//

#include "usb_mode.h"

void usb_reenumerate(void)
{
    tud_disconnect();
    osDelay(50);
    tud_connect();
}

