//
// Created by Cherry on 2026/5/25.
//

#include "sun_protocol.h"

/**
 * @brief Initialize the protocol with uninitialized keyboard state.
 * @param p Protocol to initialize
 */
void sun_protocol_init(sun_protocol_t *p)
{
    p->state = SUN_STATE_UNINIT;
    p->zero_count = 0;
}

/**
 * @brief Decode byte to event.
 * @param p Protocol
 * @param byte Byte to decode
 * @return Decoded event
 */
sun_event_t sun_protocol_decode_byte(sun_protocol_t *p, uint8_t byte)
{
    sun_event_t ev = {
        .type = SUN_EVENT_NONE,
        .data = 0
    };

    if (byte == 0x00)
    {
        if (p->zero_count < SUN_PROTOCOL_UNPLUG_THRESHOLD)
        {
            p->zero_count++;
        }
        if (p->zero_count >= SUN_PROTOCOL_UNPLUG_THRESHOLD)
        {
            p->state = SUN_STATE_UNINIT;
        }
        ev.type = SUN_EVENT_NONE;
        ev.data = byte;
        return ev;
    }

    p->zero_count = 0;

    /* 核心状态机 */
    switch (p->state)
    {
    case SUN_STATE_UNINIT:
        if (byte == 0xFF) p->state = SUN_STATE_AFTER_FF;
        break;

    case SUN_STATE_AFTER_FF:
        if (byte == 0x04)
        {
            p->state = SUN_STATE_AFTER_FF_04;
        }
        else if (byte == 0xFF)
        {
            p->state = SUN_STATE_AFTER_FF;
        }
        else
        {
            p->state = SUN_STATE_UNINIT;
            ev.type = SUN_EVENT_UNKNOWN;
            ev.data = byte;
        }
        break;

    case SUN_STATE_AFTER_FF_04:
        if (byte == 0x7F || (byte >= 0x01 && byte <= 0x7E))
        {
            p->state = SUN_STATE_NORMAL;
            ev.type = SUN_EVENT_RESET;
        }
        else
        {
            p->state = SUN_STATE_UNINIT;
            ev.type = SUN_EVENT_UNKNOWN;
            ev.data = byte;
        }
        break;

    case SUN_STATE_NORMAL:
        if (byte == 0xFF)
        {
            p->state = SUN_STATE_AFTER_FF;
        }
        else if (byte >= 0x01 && byte <= 0x7E)
        {
            ev.type = SUN_EVENT_MAKE;
            ev.data = byte;
        }
        else if (byte == 0x7F)
        {
            ev.type = SUN_EVENT_ALL_KEYS_UP;
        }
        else if (byte >= 0x80 && byte <= 0xFD)
        {
            ev.type = SUN_EVENT_BREAK;
            ev.data = byte & 0x7F;
        }

        break;

    default:
        p->state = SUN_STATE_UNINIT;
        ev.type = SUN_EVENT_UNKNOWN;
        break;
    }

    return ev;
}