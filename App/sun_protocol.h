//
// Created by Cherry on 2026/5/25.
//

// App/sun_protocol.h

#ifndef SUN_PROTOCOL_H
#define SUN_PROTOCOL_H

#include <stdint.h>

/* ---------- 事件类型 ---------- */
typedef enum {
    SUN_EVENT_NONE,         /* 字节被消费，无可观察事件 */
    SUN_EVENT_MAKE,         /* 按下；data = scan code (0x01-0x7E) */
    SUN_EVENT_BREAK,        /* 松开；data = scan code（已剥掉 bit 7） */
    SUN_EVENT_ALL_KEYS_UP,  /* 0x7F sync */
    SUN_EVENT_RESET,        /* 键盘自检完成；data 未使用 */
    SUN_EVENT_UNKNOWN,      /* 协议异常字节；data = 原字节 */
} sun_event_type_t;

typedef struct {
    sun_event_type_t type;
    uint8_t data;
} sun_event_t;

/* ---------- 解析器状态 ---------- */
typedef enum {
    SUN_STATE_UNINIT,        /* 初始/不可信状态；等待自检序列 */
    SUN_STATE_NORMAL,        /* 信任的工作状态 */
    SUN_STATE_AFTER_FF,      /* 收到 0xFF，等待 0x04 验证 */
    SUN_STATE_AFTER_FF_04,   /* 已验证自检前缀，等待第三字节 */
} sun_state_t;

/* 连续多少个 0x00 触发 UNINIT（拔出感知）。
 * 1200 baud 下一字节 ~8.3 ms，4 个 ≈ 33 ms。 */
#define SUN_PROTOCOL_UNPLUG_THRESHOLD  4

typedef struct {
    sun_state_t state;
    uint8_t zero_count;
} sun_protocol_t;

/* ---------- API ---------- */
void sun_protocol_init(sun_protocol_t *p);
sun_event_t sun_protocol_decode_byte(sun_protocol_t *p, uint8_t byte);

/* TODO Phase 5: Layout Response 支持
 *   - 新增 SUN_STATE_AFTER_FE 状态
 *   - 新增 SUN_EVENT_LAYOUT_ID 事件
 *   - 新增 sun_protocol_mark_layout_pending() 接口，
 *     供 TX 端发完 0x0F 后通知 parser 下一个 0xFE 是布局响应。 */

#endif /* SUN_PROTOCOL_H */