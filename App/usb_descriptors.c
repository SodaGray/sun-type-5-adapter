/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "stm32l4xx_hal.h"
#include "tusb.h"
#include "sun_io.h"
#include "usb_descriptors.h"

/*
 * PREREQUISITE: tusb_config.h に CFG_TUD_HID = 2 を設定すること。
 * Interface 0 (keyboard) と Interface 1 (consumer+system) の
 * 2 インスタンスが必要。
 */

#define PID_MAP(itf, n)  ((CFG_TUD_##itf) ? (1 << (n)) : 0)
#define USB_PID           (0x4000 | PID_MAP(CDC, 0) | PID_MAP(MSC, 1) | PID_MAP(HID, 2) | \
                           PID_MAP(MIDI, 3) | PID_MAP(VENDOR, 4) )

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
static tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCafe,         // TODO: CHANGE THIS
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptors
//--------------------------------------------------------------------+

/* Interface 0: NKRO Keyboard */
static uint8_t const desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_NKRO()
};

/* Interface 1: Consumer Control + System Control */
static uint8_t const desc_hid_report2[] =
{
  TUD_HID_REPORT_DESC_CONSUMER_SYSTEM()
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf)
{
  if (itf == 0) return desc_hid_report;
  if (itf == 1) return desc_hid_report2;
  return NULL;
}

//--------------------------------------------------------------------+
// Configuration Descriptors
//--------------------------------------------------------------------+

enum
{
  ITF_NUM_HID  = 0,   /* Boot-capable NKRO keyboard */
  ITF_NUM_HID2 = 1,   /* Consumer + System control  */
  ITF_NUM_TOTAL
};

/* Endpoint addresses */
#if CFG_TUD_ENDPOINT_ONE_DIRECTION_ONLY
  #define EPNUM_HID_OUT    0x01
  #define EPNUM_HID_IN     0x82
  #define EPNUM_HID2_IN    0x83
#else
  #define EPNUM_HID_OUT    0x01
  #define EPNUM_HID_IN     0x81
  #define EPNUM_HID2_IN    0x82
#endif

/* Basic mode: Interface 0 only (1-interface config, works on any host) */
#define CONFIG_TOTAL_LEN_BASIC \
  (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

/* Full mode: Interface 0 + Interface 1 (composite, for capable hosts) */
#define CONFIG_TOTAL_LEN_FULL \
  (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN + TUD_HID_DESC_LEN)

/* ── Basic config: boot-capable NKRO keyboard only ── */
static uint8_t const desc_configuration_basic[] =
{
  TUD_CONFIG_DESCRIPTOR(1, 1 /*num_itf*/, 0, CONFIG_TOTAL_LEN_BASIC, 0x00, 100),

  TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD,
                            sizeof(desc_hid_report),
                            EPNUM_HID_OUT, EPNUM_HID_IN,
                            CFG_TUD_HID_EP_BUFSIZE, 1)
};

/* ── Full config: keyboard + consumer/system ── */
static uint8_t const desc_configuration_full[] =
{
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL /*num_itf*/, 0, CONFIG_TOTAL_LEN_FULL, 0x00, 100),

  /* Interface 0: boot-capable NKRO keyboard (unchanged from basic) */
  TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD,
                            sizeof(desc_hid_report),
                            EPNUM_HID_OUT, EPNUM_HID_IN,
                            CFG_TUD_HID_EP_BUFSIZE, 1),

  /* Interface 1: consumer + system control, IN only, no boot subclass */
  TUD_HID_DESCRIPTOR(ITF_NUM_HID2, 0, HID_ITF_PROTOCOL_NONE,
                     sizeof(desc_hid_report2),
                     EPNUM_HID2_IN,
                     CFG_TUD_HID_EP_BUFSIZE, 1)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index;
  /* TODO (step 3): return desc_configuration_basic or desc_configuration_full
   * based on usb_mode_get() once the mode-switching layer is in place.
   * For now, hardcode full to test the composite descriptor. */
  return desc_configuration_full;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
};

static char const *string_desc_arr[] =
{
  (const char[]) { 0x09, 0x04 },  // 0: English (0x0409)
  "EPICRAFT",                      // 1: Manufacturer
  "SodaGray Adapter Device",       // 2: Product
  NULL,                            // 3: Serial (generated from UID)
};

static uint16_t _desc_str[32 + 1];

static inline size_t board_usb_get_serial(uint16_t desc_str[], size_t max_chars)
{
  static uint32_t uid32[3];
  uid32[0] = HAL_GetUIDw0();
  uid32[1] = HAL_GetUIDw1();
  uid32[2] = HAL_GetUIDw2();

  const uint8_t *uid = (const uint8_t *)uid32;
  size_t uid_len = 12;
  if ( uid_len > max_chars / 2u ) uid_len = max_chars / 2u;

  const unsigned char nibble_to_hex[16] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
  };

  for ( size_t i = 0; i < uid_len; i++ ) {
    for ( size_t j = 0; j < 2; j++ ) {
      const uint8_t nibble = (uint8_t)((uid[i] >> (j * 4u)) & 0xfu);
      desc_str[i * 2 + (1 - j)] = nibble_to_hex[nibble];
    }
  }
  return uid_len * 2;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;
  size_t chr_count;

  switch ( index ) {
    case STRID_LANGID:
      memcpy(&_desc_str[1], string_desc_arr[0], 2);
      chr_count = 1;
      break;

    case STRID_SERIAL:
      chr_count = board_usb_get_serial(_desc_str + 1, 32);
      break;

    default:
      if ( !(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) ) return NULL;
      const char *str = string_desc_arr[index];
      chr_count = strlen(str);
      size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
      if ( chr_count > max_count ) chr_count = max_count;
      for ( size_t i = 0; i < chr_count; i++ ) _desc_str[1 + i] = str[i];
      break;
  }

  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}