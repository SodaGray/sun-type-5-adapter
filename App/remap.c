//
// remap.c — 物理键 → 和弦的重映射覆盖层
//

#include "remap.h"
#include "storage.h"
#include "storage_types.h"
#include <string.h>

/* 存进 payload 的记录：源键 + 目标。源键在这里，instance 不承载含义。 */
typedef struct {
    uint8_t        source;   /* 这条映射针对哪个扫描码 */
    remap_target_t target;
} remap_record_t;

/* RAM 覆盖表每格，按扫描码索引（remap 层自己的组织方式）。 */
typedef struct {
    bool           active;
    uint16_t       instance;   /* 这条映射的存储句柄，开机记下；不透明 */
    remap_target_t target;
} remap_slot_t;

static remap_slot_t remap_table[REMAP_MAX_ENTRIES];

/* 找一个当前未被占用的不透明 instance：扫描 active 格收集已用值，取最小空缺。
 * active 格 ≤ REMAP_MAX_ENTRIES，故 0..REMAP_MAX_ENTRIES 间必有空缺。 */
static uint16_t alloc_instance(void)
{
    for (uint16_t cand = 0; cand <= REMAP_MAX_ENTRIES; cand++)
    {
        bool used = false;
        for (size_t i = 0; i < REMAP_MAX_ENTRIES; i++)
        {
            if (remap_table[i].active && remap_table[i].instance == cand)
            {
                used = true;
                break;
            }
        }
        if (!used) return cand;
    }
    return REMAP_MAX_ENTRIES;   /* 不可达 */
}

void remap_init(void)
{
    memset(remap_table, 0, sizeof(remap_table));   /* 全部 active = false */

    for (size_t i = 0; i < REMAP_MAX_ENTRIES; i++)
    {
        uint16_t instance = 0;
        if (!storage_enum(STORAGE_TYPE_KEYMAP, i, &instance))
        {
            break;   /* 没有更多对象 */
        }

        remap_record_t rec;
        if (storage_load(STORAGE_TYPE_KEYMAP, instance, &rec, sizeof(rec)) < 0)
        {
            continue;   /* 读失败，跳过 */
        }
        if (rec.source >= REMAP_MAX_ENTRIES)
        {
            continue;   /* payload 源越界，防御 */
        }

        remap_table[rec.source].active   = true;
        remap_table[rec.source].instance = instance;
        remap_table[rec.source].target   = rec.target;
    }
}

const remap_target_t *remap_lookup(uint8_t scancode)
{
    if (scancode >= REMAP_MAX_ENTRIES)  return NULL;
    if (!remap_table[scancode].active)  return NULL;
    return &remap_table[scancode].target;
}

bool remap_set(uint8_t scancode, const remap_target_t *target)
{
    if (scancode >= REMAP_MAX_ENTRIES || target == NULL) return false;

    uint16_t instance = remap_table[scancode].active
                      ? remap_table[scancode].instance   /* 复用，覆盖更新 */
                      : alloc_instance();                /* 新分配一个不透明句柄 */

    remap_record_t rec;
    rec.source = scancode;
    rec.target = *target;

    if (!storage_save(STORAGE_TYPE_KEYMAP, instance, &rec, sizeof(rec)))
    {
        return false;   /* 落盘失败，表不动 */
    }

    remap_table[scancode].active   = true;
    remap_table[scancode].instance = instance;
    remap_table[scancode].target   = *target;
    return true;
}

void remap_clear(uint8_t scancode)
{
    if (scancode >= REMAP_MAX_ENTRIES)  return;
    if (!remap_table[scancode].active)  return;

    storage_delete(STORAGE_TYPE_KEYMAP, remap_table[scancode].instance);
    remap_table[scancode].active = false;
}