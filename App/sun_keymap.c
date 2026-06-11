//
// Created by Cherry on 2026/5/26.
//

// App/sun_keymap.c

#include "sun_keymap.h"

/* 索引就是 scan code。
 * 未映射的位置留 {0, 0}。
 * 表大小 128，因为 scan code 范围 0x00-0x7F。 */
static const sun_key_t keymap[128] = {
    /* 0x00 */ {SUN_KEY_KIND_NONE,     0                              },  /* unused */
    /* 0x01 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_STOP                   },  /* STOP */
    /* 0x02 */ {SUN_KEY_KIND_CONSUMER, HID_USAGE_CONSUMER_VOLUME_DECREMENT            },  /* VOL- UNIX ONLY*/
    /* 0x03 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_AGAIN                  },  /* AGAIN */
    /* 0x04 */ {SUN_KEY_KIND_CONSUMER, HID_USAGE_CONSUMER_VOLUME_INCREMENT              },  /* VOL+ UNIX ONLY*/
    /* 0x05 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F1                     },  /* F1 */
    /* 0x06 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F2                     },  /* F2 */
    /* 0x07 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F10                    },  /* F10 */
    /* 0x08 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F3                     },  /* F3 */
    /* 0x09 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F11                    },  /* F11 */
    /* 0x0A */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F4                     },  /* F4 */
    /* 0x0B */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F12                    },  /* F12 */
    /* 0x0C */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F5                     },  /* F5 */
    /* 0x0D */ {SUN_KEY_KIND_MODIFIER, KEYBOARD_MODIFIER_RIGHTALT     },  /* ALTGR (RAlt -> modifier bit 6) */
    /* 0x0E */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F6                     },  /* F6 */
    /* 0x0F */ {SUN_KEY_KIND_INTERNAL, 0                              },  /* BLANK (reserved) */
    /* 0x10 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F7                     },  /* F7 */
    /* 0x11 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F8                     },  /* F8 */
    /* 0x12 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F9                     },  /* F9 */
    /* 0x13 */ {SUN_KEY_KIND_MODIFIER, KEYBOARD_MODIFIER_LEFTALT      },  /* ALT (LAlt -> modifier bit 2) */
    /* 0x14 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_ARROW_UP               },  /* UP */
    /* 0x15 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_PAUSE                  },  /* PAUSE */
    /* 0x16 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_PRINT_SCREEN           },  /* PRTSC */
    /* 0x17 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_SCROLL_LOCK            },  /* SCRLK */
    /* 0x18 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_ARROW_LEFT             },  /* LEFT */
    /* 0x19 */ {SUN_KEY_KIND_NONE,     0                              },  /* PROPS (unmapped) */
    /* 0x1A */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_UNDO                   },  /* UNDO */
    /* 0x1B */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_ARROW_DOWN             },  /* DOWN */
    /* 0x1C */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_ARROW_RIGHT            },  /* RIGHT */
    /* 0x1D */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_ESCAPE                 },  /* ESC */
    /* 0x1E */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_1                      },  /* 1 */
    /* 0x1F */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_2                      },  /* 2 */
    /* 0x20 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_3                      },  /* 3 */
    /* 0x21 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_4                      },  /* 4 */
    /* 0x22 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_5                      },  /* 5 */
    /* 0x23 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_6                      },  /* 6 */
    /* 0x24 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_7                      },  /* 7 */
    /* 0x25 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_8                      },  /* 8 */
    /* 0x26 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_9                      },  /* 9 */
    /* 0x27 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_0                      },  /* 0 */
    /* 0x28 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_MINUS                  },  /* DASH (-) */
    /* 0x29 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_EQUAL                  },  /* EQUAL (=) */
    /* 0x2A */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_GRAVE                  },  /* ACCENT (`) */
    /* 0x2B */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_BACKSPACE              },  /* BKSPC */
    /* 0x2C */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_INSERT                 },  /* INS */
    /* 0x2D */ {SUN_KEY_KIND_KEYBOARD, HID_USAGE_CONSUMER_MUTE                   },  /* MUTE UNIX ONLY*/
    /* 0x2E */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_DIVIDE          },  /* DIV (Keypad /) */
    /* 0x2F */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_MULTIPLY        },  /* MUL (Keypad *) */
    /* 0x30 */ {SUN_KEY_KIND_SYSTEM,   HID_USAGE_DESKTOP_SYSTEM_POWER_DOWN                  },  /* POWER UNIX ONLY*/
    /* 0x31 */ {SUN_KEY_KIND_NONE,     0                              },  /* FRONT (unmapped) */
    /* 0x32 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_DECIMAL         },  /* DOT (Keypad .) */
    /* 0x33 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_COPY                   },  /* COPY */
    /* 0x34 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_HOME                   },  /* HOME */
    /* 0x35 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_TAB                    },  /* TAB */
    /* 0x36 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_Q                      },  /* Q */
    /* 0x37 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_W                      },  /* W */
    /* 0x38 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_E                      },  /* E */
    /* 0x39 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_R                      },  /* R */
    /* 0x3A */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_T                      },  /* T */
    /* 0x3B */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_Y                      },  /* Y */
    /* 0x3C */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_U                      },  /* U */
    /* 0x3D */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_I                      },  /* I */
    /* 0x3E */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_O                      },  /* O */
    /* 0x3F */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_P                      },  /* P */
    /* 0x40 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_BRACKET_LEFT           },  /* LBRAC ([) */
    /* 0x41 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_BRACKET_RIGHT          },  /* RBRAC (]) */
    /* 0x42 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_DELETE                 },  /* DEL */
    /* 0x43 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_APPLICATION            },  /* COMPOS (mapped to Application) */
    /* 0x44 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_7               },  /* Keypad 7 */
    /* 0x45 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_8               },  /* Keypad 8 */
    /* 0x46 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_9               },  /* Keypad 9 */
    /* 0x47 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_SUBTRACT        },  /* MINUS (Keypad -) */
    /* 0x48 */ {SUN_KEY_KIND_NONE,     0                              },  /* OPEN (unmapped) */
    /* 0x49 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_PASTE                  },  /* PASTE */
    /* 0x4A */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_END                    },  /* END */
    /* 0x4B */ {SUN_KEY_KIND_NONE,     0                              },  /* unused */
    /* 0x4C */ {SUN_KEY_KIND_MODIFIER, KEYBOARD_MODIFIER_LEFTCTRL     },  /* CTRL (LCtrl -> modifier bit 0) */
    /* 0x4D */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_A                      },  /* A */
    /* 0x4E */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_S                      },  /* S */
    /* 0x4F */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_D                      },  /* D */
    /* 0x50 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_F                      },  /* F */
    /* 0x51 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_G                      },  /* G */
    /* 0x52 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_H                      },  /* H */
    /* 0x53 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_J                      },  /* J */
    /* 0x54 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_K                      },  /* K */
    /* 0x55 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_L                      },  /* L */
    /* 0x56 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_SEMICOLON              },  /* SCOL (;) */
    /* 0x57 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_APOSTROPHE             },  /* APOS (') */
    /* 0x58 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_BACKSLASH              },  /* BSLH (\) */
    /* 0x59 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_ENTER                  },  /* RETURN */
    /* 0x5A */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_ENTER           },  /* ENTER (Keypad) */
    /* 0x5B */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_4               },  /* Keypad 4 */
    /* 0x5C */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_5               },  /* Keypad 5 */
    /* 0x5D */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_6               },  /* Keypad 6 */
    /* 0x5E */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_0               },  /* Keypad 0 */
    /* 0x5F */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_FIND                   },  /* FIND */
    /* 0x60 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_PAGE_UP                },  /* PGUP */
    /* 0x61 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_CUT                    },  /* CUT */
    /* 0x62 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_NUM_LOCK               },  /* NUMLK */
    /* 0x63 */ {SUN_KEY_KIND_MODIFIER, KEYBOARD_MODIFIER_LEFTSHIFT    },  /* LSHIFT (LShift -> modifier bit 1) */
    /* 0x64 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_Z                      },  /* Z */
    /* 0x65 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_X                      },  /* X */
    /* 0x66 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_C                      },  /* C */
    /* 0x67 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_V                      },  /* V */
    /* 0x68 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_B                      },  /* B */
    /* 0x69 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_N                      },  /* N */
    /* 0x6A */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_M                      },  /* M */
    /* 0x6B */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_COMMA                  },  /* COMMA (,) */
    /* 0x6C */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_PERIOD                 },  /* PERIOD (.) */
    /* 0x6D */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_SLASH                  },  /* SLASH (/) */
    /* 0x6E */ {SUN_KEY_KIND_MODIFIER, KEYBOARD_MODIFIER_RIGHTSHIFT   },  /* RSHIFT (RShift -> modifier bit 5) */
    /* 0x6F */ {SUN_KEY_KIND_NONE,     0                              },  /* unused (Line Feed) */
    /* 0x70 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_1               },  /* Keypad 1 */
    /* 0x71 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_2               },  /* Keypad 2 */
    /* 0x72 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_3               },  /* Keypad 3 */
    /* 0x73 */ {SUN_KEY_KIND_NONE,     0                              },  /* unused */
    /* 0x74 */ {SUN_KEY_KIND_NONE,     0                              },  /* unused */
    /* 0x75 */ {SUN_KEY_KIND_NONE,     0                              },  /* unused */
    /* 0x76 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_HELP                   },  /* HELP */
    /* 0x77 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_CAPS_LOCK              },  /* CAPSLK */
    /* 0x78 */ {SUN_KEY_KIND_MODIFIER, KEYBOARD_MODIFIER_LEFTGUI      },  /* LMETA (LGui -> modifier bit 3) */
    /* 0x79 */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_SPACE                  },  /* SPACE */
    /* 0x7A */ {SUN_KEY_KIND_MODIFIER, KEYBOARD_MODIFIER_RIGHTGUI     },  /* RMETA (RGui -> modifier bit 7) */
    /* 0x7B */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_PAGE_DOWN              },  /* PGDN */
    /* 0x7C */ {SUN_KEY_KIND_NONE,     0                              },  /* unused */
    /* 0x7D */ {SUN_KEY_KIND_KEYBOARD, HID_KEY_KEYPAD_ADD             },  /* PLUS (Keypad +) */
    /* 0x7E */ {SUN_KEY_KIND_NONE,     0                              },  /* unused */
    /* 0x7F */ {SUN_KEY_KIND_NONE,     0                              }   /* All Keys Up / unmapped */
};

sun_key_t sun_keymap_lookup(uint8_t scan_code)
{
    if (scan_code >= 128) {
        return (sun_key_t){0, 0};  /* 越界 */
    }
    return keymap[scan_code];
}