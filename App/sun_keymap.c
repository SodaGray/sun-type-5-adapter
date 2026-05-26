//
// Created by Cherry on 2026/5/26.
//

// App/sun_keymap.c

#include "sun_keymap.h"

/* 索引就是 scan code。
 * 未映射的位置留 {0, 0}。
 * 表大小 128，因为 scan code 范围 0x00-0x7F。 */
static const sun_key_t keymap[128] = {
    /* 0x00 */ {SUN_KEY_KIND_NONE,     0   },  /* unused */
    /* 0x01 */ {SUN_KEY_KIND_KEYBOARD, 0x78},  /* STOP */
    /* 0x02 */ {SUN_KEY_KIND_KEYBOARD, 0x81},  /* VOL- UNIX ONLY*/
    /* 0x03 */ {SUN_KEY_KIND_KEYBOARD, 0x79},  /* AGAIN */
    /* 0x04 */ {SUN_KEY_KIND_KEYBOARD, 0x80},  /* VOL+ UNIX ONLY*/
    /* 0x05 */ {SUN_KEY_KIND_KEYBOARD, 0x3A},  /* F1 */
    /* 0x06 */ {SUN_KEY_KIND_KEYBOARD, 0x3B},  /* F2 */
    /* 0x07 */ {SUN_KEY_KIND_KEYBOARD, 0x43},  /* F10 */
    /* 0x08 */ {SUN_KEY_KIND_KEYBOARD, 0x3C},  /* F3 */
    /* 0x09 */ {SUN_KEY_KIND_KEYBOARD, 0x44},  /* F11 */
    /* 0x0A */ {SUN_KEY_KIND_KEYBOARD, 0x3D},  /* F4 */
    /* 0x0B */ {SUN_KEY_KIND_KEYBOARD, 0x45},  /* F12 */
    /* 0x0C */ {SUN_KEY_KIND_KEYBOARD, 0x3E},  /* F5 */
    /* 0x0D */ {SUN_KEY_KIND_MODIFIER, 0x40},  /* ALTGR (RAlt -> modifier bit 6) */
    /* 0x0E */ {SUN_KEY_KIND_KEYBOARD, 0x3F},  /* F6 */
    /* 0x0F */ {SUN_KEY_KIND_NONE,     0   },  /* BLANK (unused) */
    /* 0x10 */ {SUN_KEY_KIND_KEYBOARD, 0x40},  /* F7 */
    /* 0x11 */ {SUN_KEY_KIND_KEYBOARD, 0x41},  /* F8 */
    /* 0x12 */ {SUN_KEY_KIND_KEYBOARD, 0x42},  /* F9 */
    /* 0x13 */ {SUN_KEY_KIND_MODIFIER, 0x04},  /* ALT (LAlt -> modifier bit 2) */
    /* 0x14 */ {SUN_KEY_KIND_KEYBOARD, 0x52},  /* UP */
    /* 0x15 */ {SUN_KEY_KIND_KEYBOARD, 0x48},  /* PAUSE */
    /* 0x16 */ {SUN_KEY_KIND_KEYBOARD, 0x46},  /* PRTSC */
    /* 0x17 */ {SUN_KEY_KIND_KEYBOARD, 0x47},  /* SCRLK */
    /* 0x18 */ {SUN_KEY_KIND_KEYBOARD, 0x50},  /* LEFT */
    /* 0x19 */ {SUN_KEY_KIND_NONE,     0   },  /* PROPS (unmapped) */
    /* 0x1A */ {SUN_KEY_KIND_KEYBOARD, 0x7A},  /* UNDO */
    /* 0x1B */ {SUN_KEY_KIND_KEYBOARD, 0x51},  /* DOWN */
    /* 0x1C */ {SUN_KEY_KIND_KEYBOARD, 0x4F},  /* RIGHT */
    /* 0x1D */ {SUN_KEY_KIND_KEYBOARD, 0x29},  /* ESC */
    /* 0x1E */ {SUN_KEY_KIND_KEYBOARD, 0x1E},  /* 1 */
    /* 0x1F */ {SUN_KEY_KIND_KEYBOARD, 0x1F},  /* 2 */
    /* 0x20 */ {SUN_KEY_KIND_KEYBOARD, 0x20},  /* 3 */
    /* 0x21 */ {SUN_KEY_KIND_KEYBOARD, 0x21},  /* 4 */
    /* 0x22 */ {SUN_KEY_KIND_KEYBOARD, 0x22},  /* 5 */
    /* 0x23 */ {SUN_KEY_KIND_KEYBOARD, 0x23},  /* 6 */
    /* 0x24 */ {SUN_KEY_KIND_KEYBOARD, 0x24},  /* 7 */
    /* 0x25 */ {SUN_KEY_KIND_KEYBOARD, 0x25},  /* 8 */
    /* 0x26 */ {SUN_KEY_KIND_KEYBOARD, 0x26},  /* 9 */
    /* 0x27 */ {SUN_KEY_KIND_KEYBOARD, 0x27},  /* 0 */
    /* 0x28 */ {SUN_KEY_KIND_KEYBOARD, 0x2D},  /* DASH (-) */
    /* 0x29 */ {SUN_KEY_KIND_KEYBOARD, 0x2E},  /* EQUAL (=) */
    /* 0x2A */ {SUN_KEY_KIND_KEYBOARD, 0x35},  /* ACCENT (`) */
    /* 0x2B */ {SUN_KEY_KIND_KEYBOARD, 0x2A},  /* BKSPC */
    /* 0x2C */ {SUN_KEY_KIND_KEYBOARD, 0x49},  /* INS */
    /* 0x2D */ {SUN_KEY_KIND_KEYBOARD, 0x7F},  /* MUTE UNIX ONLY*/
    /* 0x2E */ {SUN_KEY_KIND_KEYBOARD, 0x54},  /* DIV (Keypad /) */
    /* 0x2F */ {SUN_KEY_KIND_KEYBOARD, 0x55},  /* MUL (Keypad *) */
    /* 0x30 */ {SUN_KEY_KIND_KEYBOARD, 0x66},  /* POWER UNIX ONLY*/
    /* 0x31 */ {SUN_KEY_KIND_NONE,     0   },  /* FRONT (unmapped) */
    /* 0x32 */ {SUN_KEY_KIND_KEYBOARD, 0x63},  /* DOT (Keypad .) */
    /* 0x33 */ {SUN_KEY_KIND_KEYBOARD, 0x7C},  /* COPY */
    /* 0x34 */ {SUN_KEY_KIND_KEYBOARD, 0x4A},  /* HOME */
    /* 0x35 */ {SUN_KEY_KIND_KEYBOARD, 0x2B},  /* TAB */
    /* 0x36 */ {SUN_KEY_KIND_KEYBOARD, 0x14},  /* Q */
    /* 0x37 */ {SUN_KEY_KIND_KEYBOARD, 0x1A},  /* W */
    /* 0x38 */ {SUN_KEY_KIND_KEYBOARD, 0x08},  /* E */
    /* 0x39 */ {SUN_KEY_KIND_KEYBOARD, 0x15},  /* R */
    /* 0x3A */ {SUN_KEY_KIND_KEYBOARD, 0x17},  /* T */
    /* 0x3B */ {SUN_KEY_KIND_KEYBOARD, 0x1C},  /* Y */
    /* 0x3C */ {SUN_KEY_KIND_KEYBOARD, 0x18},  /* U */
    /* 0x3D */ {SUN_KEY_KIND_KEYBOARD, 0x0C},  /* I */
    /* 0x3E */ {SUN_KEY_KIND_KEYBOARD, 0x12},  /* O */
    /* 0x3F */ {SUN_KEY_KIND_KEYBOARD, 0x13},  /* P */
    /* 0x40 */ {SUN_KEY_KIND_KEYBOARD, 0x2F},  /* LBRAC ([) */
    /* 0x41 */ {SUN_KEY_KIND_KEYBOARD, 0x30},  /* RBRAC (]) */
    /* 0x42 */ {SUN_KEY_KIND_KEYBOARD, 0x4C},  /* DEL */
    /* 0x43 */ {SUN_KEY_KIND_KEYBOARD, 0x65},  /* COMPOS (mapped to Application) */
    /* 0x44 */ {SUN_KEY_KIND_KEYBOARD, 0x5F},  /* Keypad 7 */
    /* 0x45 */ {SUN_KEY_KIND_KEYBOARD, 0x60},  /* Keypad 8 */
    /* 0x46 */ {SUN_KEY_KIND_KEYBOARD, 0x61},  /* Keypad 9 */
    /* 0x47 */ {SUN_KEY_KIND_KEYBOARD, 0x56},  /* MINUS (Keypad -) */
    /* 0x48 */ {SUN_KEY_KIND_NONE,     0   },  /* OPEN (unmapped) */
    /* 0x49 */ {SUN_KEY_KIND_KEYBOARD, 0x7D},  /* PASTE */
    /* 0x4A */ {SUN_KEY_KIND_KEYBOARD, 0x4D},  /* END */
    /* 0x4B */ {SUN_KEY_KIND_NONE,     0   },  /* unused */
    /* 0x4C */ {SUN_KEY_KIND_MODIFIER, 0x01},  /* CTRL (LCtrl -> modifier bit 0) */
    /* 0x4D */ {SUN_KEY_KIND_KEYBOARD, 0x04},  /* A */
    /* 0x4E */ {SUN_KEY_KIND_KEYBOARD, 0x16},  /* S */
    /* 0x4F */ {SUN_KEY_KIND_KEYBOARD, 0x07},  /* D */
    /* 0x50 */ {SUN_KEY_KIND_KEYBOARD, 0x09},  /* F */
    /* 0x51 */ {SUN_KEY_KIND_KEYBOARD, 0x0A},  /* G */
    /* 0x52 */ {SUN_KEY_KIND_KEYBOARD, 0x0B},  /* H */
    /* 0x53 */ {SUN_KEY_KIND_KEYBOARD, 0x0D},  /* J */
    /* 0x54 */ {SUN_KEY_KIND_KEYBOARD, 0x0E},  /* K */
    /* 0x55 */ {SUN_KEY_KIND_KEYBOARD, 0x0F},  /* L */
    /* 0x56 */ {SUN_KEY_KIND_KEYBOARD, 0x33},  /* SCOL (;) */
    /* 0x57 */ {SUN_KEY_KIND_KEYBOARD, 0x34},  /* APOS (') */
    /* 0x58 */ {SUN_KEY_KIND_KEYBOARD, 0x31},  /* BSLH (\) */
    /* 0x59 */ {SUN_KEY_KIND_KEYBOARD, 0x28},  /* RETURN */
    /* 0x5A */ {SUN_KEY_KIND_KEYBOARD, 0x58},  /* ENTER (Keypad) */
    /* 0x5B */ {SUN_KEY_KIND_KEYBOARD, 0x5C},  /* Keypad 4 */
    /* 0x5C */ {SUN_KEY_KIND_KEYBOARD, 0x5D},  /* Keypad 5 */
    /* 0x5D */ {SUN_KEY_KIND_KEYBOARD, 0x5E},  /* Keypad 6 */
    /* 0x5E */ {SUN_KEY_KIND_KEYBOARD, 0x62},  /* Keypad 0 */
    /* 0x5F */ {SUN_KEY_KIND_KEYBOARD, 0x7E},  /* FIND */
    /* 0x60 */ {SUN_KEY_KIND_KEYBOARD, 0x4B},  /* PGUP */
    /* 0x61 */ {SUN_KEY_KIND_KEYBOARD, 0x7B},  /* CUT */
    /* 0x62 */ {SUN_KEY_KIND_KEYBOARD, 0x53},  /* NUMLK */
    /* 0x63 */ {SUN_KEY_KIND_MODIFIER, 0x02},  /* LSHIFT (LShift -> modifier bit 1) */
    /* 0x64 */ {SUN_KEY_KIND_KEYBOARD, 0x1D},  /* Z */
    /* 0x65 */ {SUN_KEY_KIND_KEYBOARD, 0x1B},  /* X */
    /* 0x66 */ {SUN_KEY_KIND_KEYBOARD, 0x06},  /* C */
    /* 0x67 */ {SUN_KEY_KIND_KEYBOARD, 0x19},  /* V */
    /* 0x68 */ {SUN_KEY_KIND_KEYBOARD, 0x05},  /* B */
    /* 0x69 */ {SUN_KEY_KIND_KEYBOARD, 0x11},  /* N */
    /* 0x6A */ {SUN_KEY_KIND_KEYBOARD, 0x10},  /* M */
    /* 0x6B */ {SUN_KEY_KIND_KEYBOARD, 0x36},  /* COMMA (,) */
    /* 0x6C */ {SUN_KEY_KIND_KEYBOARD, 0x37},  /* PERIOD (.) */
    /* 0x6D */ {SUN_KEY_KIND_KEYBOARD, 0x38},  /* SLASH (/) */
    /* 0x6E */ {SUN_KEY_KIND_MODIFIER, 0x20},  /* RSHIFT (RShift -> modifier bit 5) */
    /* 0x6F */ {SUN_KEY_KIND_NONE,     0   },  /* unused (Line Feed) */
    /* 0x70 */ {SUN_KEY_KIND_KEYBOARD, 0x59},  /* Keypad 1 */
    /* 0x71 */ {SUN_KEY_KIND_KEYBOARD, 0x5A},  /* Keypad 2 */
    /* 0x72 */ {SUN_KEY_KIND_KEYBOARD, 0x5B},  /* Keypad 3 */
    /* 0x73 */ {SUN_KEY_KIND_NONE,     0   },  /* unused */
    /* 0x74 */ {SUN_KEY_KIND_NONE,     0   },  /* unused */
    /* 0x75 */ {SUN_KEY_KIND_NONE,     0   },  /* unused */
    /* 0x76 */ {SUN_KEY_KIND_KEYBOARD, 0x75},  /* HELP */
    /* 0x77 */ {SUN_KEY_KIND_KEYBOARD, 0x39},  /* CAPSLK */
    /* 0x78 */ {SUN_KEY_KIND_MODIFIER, 0x08},  /* LMETA (LGui -> modifier bit 3) */
    /* 0x79 */ {SUN_KEY_KIND_KEYBOARD, 0x2C},  /* SPACE */
    /* 0x7A */ {SUN_KEY_KIND_MODIFIER, 0x80},  /* RMETA (RGui -> modifier bit 7) */
    /* 0x7B */ {SUN_KEY_KIND_KEYBOARD, 0x4E},  /* PGDN */
    /* 0x7C */ {SUN_KEY_KIND_NONE,     0   },  /* unused */
    /* 0x7D */ {SUN_KEY_KIND_KEYBOARD, 0x57},  /* PLUS (Keypad +) */
    /* 0x7E */ {SUN_KEY_KIND_NONE,     0   },  /* unused */
    /* 0x7F */ {SUN_KEY_KIND_NONE,     0   }   /* All Keys Up / unmapped */
};

sun_key_t sun_keymap_lookup(uint8_t scan_code)
{
    if (scan_code >= 128) {
        return (sun_key_t){0, 0};  /* 越界 */
    }
    return keymap[scan_code];
}