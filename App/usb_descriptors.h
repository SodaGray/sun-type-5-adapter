/*
 * usb_descriptors.h
 *
 * Custom HID report descriptor macros.
 *
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include "class/hid/hid.h"

/* ── Interface 0: NKRO Keyboard ───────────────────────────────────────────
 *
 * Boot 协议下回退为固定 8 字节 boot 报告；
 * Report 协议下使用本描述符：1 字节修饰键 + 20 字节按键位图 = 21 字节。
 */
#define TUD_HID_REPORT_DESC_NKRO(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP      )                    ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_KEYBOARD  )                    ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION  )                    ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    /* 8 bits 修饰键 (usage 0xE0-0xE7) */ \
    HID_USAGE_PAGE ( HID_USAGE_PAGE_KEYBOARD                       ),\
      HID_USAGE_MIN    ( 224                                        ),\
      HID_USAGE_MAX    ( 231                                        ),\
      HID_LOGICAL_MIN  ( 0                                          ),\
      HID_LOGICAL_MAX  ( 1                                          ),\
      HID_REPORT_COUNT ( 8                                          ),\
      HID_REPORT_SIZE  ( 1                                          ),\
      HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE     ),\
    /* 5-bit LED 输出 + 3-bit 填充 */ \
    HID_USAGE_PAGE  ( HID_USAGE_PAGE_LED                            ),\
      HID_USAGE_MIN    ( 1                                          ),\
      HID_USAGE_MAX    ( 5                                          ),\
      HID_REPORT_COUNT ( 5                                          ),\
      HID_REPORT_SIZE  ( 1                                          ),\
      HID_OUTPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE     ),\
      HID_REPORT_COUNT ( 1                                          ),\
      HID_REPORT_SIZE  ( 3                                          ),\
      HID_OUTPUT       ( HID_CONSTANT                               ),\
    /* NKRO 按键位图: usage 0x00-0x9F, bit 号 = usage 码, 160 位 = 20 字节 */ \
    HID_USAGE_PAGE ( HID_USAGE_PAGE_KEYBOARD                       ),\
      HID_USAGE_MIN    ( 0                                          ),\
      HID_USAGE_MAX    ( 0x9F                                       ),\
      HID_LOGICAL_MIN  ( 0                                          ),\
      HID_LOGICAL_MAX  ( 1                                          ),\
      HID_REPORT_COUNT ( 160                                        ),\
      HID_REPORT_SIZE  ( 1                                          ),\
      HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE     ),\
  HID_COLLECTION_END


/* ── Interface 1: Consumer Control + System Control ───────────────────────
 *
 * Report ID 1 — Consumer Control (Usage Page 0x0C)
 *   格式: [Report ID=1][Usage 低字节][Usage 高字节]，共 3 字节
 *   语义: 报告当前按下的 consumer usage；无按键时发 0x0000
 *   范围: usage 0x0000–0x00EA（涵盖音量/静音/播放/曲目控制）
 *
 * Report ID 2 — Generic Desktop System Control (Usage Page 0x01, 0x80+)
 *   格式: [Report ID=2][bitmap 字节]，共 2 字节
 *   bitmap 字节布局:
 *     bit 0 = System Power Down (0x81)
 *     bit 1 = System Sleep      (0x82)
 *     bit 2 = System Wake Up    (0x83)
 *     bit 3-7 = 常量填充 0
 */
#define TUD_HID_REPORT_DESC_CONSUMER_SYSTEM() \
  /* ── Consumer Control (Report ID 1) ── */ \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_CONSUMER                         ),\
  HID_USAGE      ( HID_USAGE_CONSUMER_CONTROL                      ),\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION                      ),\
    HID_REPORT_ID  ( 1                                             )  \
    HID_LOGICAL_MIN  ( 0                                           ),\
    HID_LOGICAL_MAX_N( 0x00EA, 2                                   ),\
    HID_USAGE_MIN    ( 0                                           ),\
    HID_USAGE_MAX_N  ( 0x00EA, 2                                   ),\
    HID_REPORT_SIZE  ( 16                                          ),\
    HID_REPORT_COUNT ( 1                                           ),\
    HID_INPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE         ),\
  HID_COLLECTION_END                                                ,\
  /* ── Generic Desktop System Control (Report ID 2) ── */ \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP                          ),\
  HID_USAGE      ( HID_USAGE_DESKTOP_SYSTEM_CONTROL                ),\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION                      ),\
    HID_REPORT_ID  ( 2                                             )  \
    HID_LOGICAL_MIN  ( 0                                           ),\
    HID_LOGICAL_MAX  ( 1                                           ),\
    HID_USAGE_MIN    ( HID_USAGE_DESKTOP_SYSTEM_POWER_DOWN         ),\
    HID_USAGE_MAX    ( HID_USAGE_DESKTOP_SYSTEM_WAKE_UP            ),\
    HID_REPORT_SIZE  ( 1                                           ),\
    HID_REPORT_COUNT ( 3                                           ),\
    HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE      ),\
    HID_REPORT_SIZE  ( 5                                           ),\
    HID_REPORT_COUNT ( 1                                           ),\
    HID_INPUT        ( HID_CONSTANT                                ),\
  HID_COLLECTION_END

#endif /* USB_DESCRIPTORS_H */