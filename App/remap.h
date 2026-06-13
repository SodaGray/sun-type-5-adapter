//
// remap.h — 物理键 → 和弦的重映射覆盖层
//

#ifndef REMAP_H
#define REMAP_H

#include <stdint.h>
#include <stdbool.h>

/* 目标和弦里普通键的上限 */
#define REMAP_MAX_KEYS     4
/* 覆盖表大小：覆盖 7 位扫描码空间 */
#define REMAP_MAX_ENTRIES  128

/* 一个重映射的目标：被改的键吐出什么。
 * modifiers 为 HID 修饰掩码（可多 bit），外加最多 REMAP_MAX_KEYS 个普通 HID 键码。 */
typedef struct {
    uint8_t modifiers;            /* HID 修饰掩码 */
    uint8_t key_count;            /* keys[] 中有效个数 0..REMAP_MAX_KEYS */
    uint8_t keys[REMAP_MAX_KEYS]; /* 普通键的 HID 键码 */
} remap_target_t;

/* 开机调用一次（在 storage_mount() 之后）：把已存的所有重映射读进 RAM 覆盖表。 */
void remap_init(void);

/* 查某扫描码的映射。已映射返回目标指针，未映射返回 NULL（落回静态 keymap）。 */
const remap_target_t *remap_lookup(uint8_t scancode);

/* 设置/替换某扫描码的映射：持久化 + 更新表。成功返回 true。 */
bool remap_set(uint8_t scancode, const remap_target_t *target);

/* 清除某扫描码的映射：删除存储对象 + 更新表。 */
void remap_clear(uint8_t scancode);

#endif /* REMAP_H */