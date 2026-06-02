//
// Created by Cherry on 2026/5/31.
//

#include "storage.h"
#include "sector_io.h"
#include "crc32.h"
#include <stdio.h>

_Static_assert(STORAGE_MAX_OBJECT_SIZE == MAX_PAYLOAD_SIZE,
               "object size must track sector_io's MAX_PAYLOAD_SIZE");

/* 每个 sector 的运行时状态(注意:与 sector_io 的 sector_state_t 不同 ——
 * 那个描述 flash 上的物理态,这个是存储层管理的运行态) */
typedef enum {
    SLOT_FREE,      /* virgin 或回收后的空闲;可供分配 */
    SLOT_PENDING,   /* pass1 见到 OBJ magic,待 pass2 细化 */
    SLOT_LIVE,      /* 持有某对象的当前版本 */
    SLOT_RECLAIM,   /* 已废/损坏/墓碑;等 GC */
} slot_state_t;

typedef struct {
    uint16_t type;
    uint16_t instance;
    uint16_t sector;
    uint32_t sequence;   /* 扫描时裁决冲突用 */
} dir_entry_t;

static dir_entry_t directory[STORAGE_MAX_LIVE_OBJECTS];   /* ~3 KB */
static uint16_t    directory_count;

static uint8_t  slot_state[W25Q_SECTOR_COUNT];            /* 2 KB, 存 slot_state_t */
static uint32_t erase_count[W25Q_SECTOR_COUNT];           /* 8 KB, u32 */

static uint32_t next_seq;
static uint16_t free_count;
static uint32_t min_erase;
static uint32_t max_erase;

bool storage_mount(void)
{
    // 重置全局统计变量
    directory_count = 0;
    next_seq = 0;
    free_count = 0;
    min_erase = 0xFFFFFFFFu;
    max_erase = 0;

    // ==========================================
    // PASS 1: 快速扫描 Preamble（8字节），建立寿命表与粗筛状态
    // ==========================================
    for (uint16_t idx = 0; idx < W25Q_SECTOR_COUNT; idx++)
    {
        uint32_t magic = 0;
        uint32_t raw_count = 0;

        if (!sector_read_preamble(idx, &magic, &raw_count))
        {
            erase_count[idx] = 0xFFFFFFFFu;
            slot_state[idx] = SLOT_RECLAIM;
            continue;
        }

        // 映射逻辑擦除次数 (VIRGIN 映射为 0)
        uint32_t logical_count = sector_logical_count(magic, raw_count);
        erase_count[idx] = logical_count;

        sector_state_t p_state = sector_classify(magic, raw_count);

        // 寿命统计只纳入计数可信的扇区:VIRGIN(=0) / FREE / OBJECT。
        // 排除 RECLAIM —— 它的 offset 4-7 可能是损坏后的垃圾值,
        // 混进来会把 max_erase 抬高、触发步骤6的 WL 误搬迁。
        if (p_state != SECTOR_STATE_RECLAIM)
        {
            if (logical_count > max_erase) max_erase = logical_count;
            if (logical_count < min_erase) min_erase = logical_count;
        }

        switch (p_state)
        {
            case SECTOR_STATE_VIRGIN:
            case SECTOR_STATE_FREE:
                slot_state[idx] = SLOT_FREE;
                free_count++;
                break;
            case SECTOR_STATE_RECLAIM:
                slot_state[idx] = SLOT_RECLAIM;
                break;
            case SECTOR_STATE_OBJECT:
                slot_state[idx] = SLOT_PENDING; // 留给 Pass 2 细化
                break;
        }
    }

    // 如果整片 Flash 都是干净的（或全部读失败），修正下界
    if (min_erase == 0xFFFFFFFFu) min_erase = 0;

    // ==========================================
    // PASS 2: 细化 PENDING 扇区，裁决断电冲突，重建文件分配目录
    // ==========================================
    for (uint16_t idx = 0; idx < W25Q_SECTOR_COUNT; idx++)
    {
        if (slot_state[idx] != SLOT_PENDING) continue;

        obj_header_t header;
        if (!sector_read_obj_header(idx, &header))
        {
            slot_state[idx] = SLOT_RECLAIM;
            continue;
        }

        // 校验头部自洽性 (CRC32 与 魔法数)
        if (header.magic != SECTOR_MAGIC_OBJ ||
            crc32_compute(&header, HEADER_CRC_RANGE_LEN) != header.header_crc)
        {
            slot_state[idx] = SLOT_RECLAIM;
            continue;
        }

        // 推进全局序列号生成器,确保下一次 save 拿到的号码绝对唯一且递增。
        // 放在 liveness 检查之前:obsolete 但合法的对象也要计入,
        // 才能保证 next_seq > 所有 OBJECT 序号。
        if (header.sequence >= next_seq)
        {
            next_seq = header.sequence + 1;
        }

        // 原子生命周期审查:历史旧数据、或写一半断电的 FRESH 数据,都视为废弃等 GC
        if (!STATE_IS_LIVE(header.state_byte))
        {
            slot_state[idx] = SLOT_RECLAIM;
            continue;
        }

        // 到这里,此扇区持有一个合法且已提交的 Live 对象。开始冲突裁决:
        int found_idx = -1;
        for (uint16_t d = 0; d < directory_count; d++)
        {
            if (directory[d].type == header.type && directory[d].instance == header.instance)
            {
                found_idx = d;
                break;
            }
        }

        if (found_idx == -1)
        {
            if (directory_count < STORAGE_MAX_LIVE_OBJECTS)
            {
                directory[directory_count].type = header.type;
                directory[directory_count].instance = header.instance;
                directory[directory_count].sector = idx;
                directory[directory_count].sequence = header.sequence;
                directory_count++;

                slot_state[idx] = SLOT_LIVE;
            }
            else
            {
                // 目录已满:正常不会发生(save 会在 256 上限处拒绝新对象)。
                // 真撞上=flash 上 live 对象数超过 RAM 目录容量,此对象将被丢弃。
                printf("storage_mount: directory full, discarding type=%u inst=%u\r\n",
                       header.type, header.instance);
                slot_state[idx] = SLOT_RECLAIM;
            }
        }
        else
        {
            // 断电双活冲突
            uint16_t old_sector = directory[found_idx].sector;

            if (header.sequence > directory[found_idx].sequence)
            {
                // 当前扇区更新,新数据胜出
                slot_state[old_sector] = SLOT_RECLAIM;
                directory[found_idx].sector = idx;
                directory[found_idx].sequence = header.sequence;
                slot_state[idx] = SLOT_LIVE;
            }
            else
            {
                slot_state[idx] = SLOT_RECLAIM;
            }
        }
    }

    return true;
}

/* ============================================================
 * 步骤 4-6 待实现 —— 先留桩,使文件可链接
 * ============================================================ */

bool storage_save(uint16_t type, uint16_t instance, const void *data, size_t len)
{
    (void)type; (void)instance; (void)data; (void)len;
    return false;
}

int storage_load(uint16_t type, uint16_t instance, void *buf, size_t max_len)
{
    (void)type; (void)instance; (void)buf; (void)max_len;
    return STORAGE_ERR_NOT_FOUND;
}

bool storage_delete(uint16_t type, uint16_t instance)
{
    (void)type; (void)instance;
    return false;
}

int storage_count(uint16_t type)
{
    (void)type;
    return 0;
}

bool storage_enum(uint16_t type, size_t index, uint16_t *instance_out)
{
    (void)type; (void)index; (void)instance_out;
    return false;
}

/* ============================================================
 * 调试转储 —— bring-up 验证用
 * ============================================================ */

void storage_debug_dump(void)
{
    uint16_t n_free = 0, n_pending = 0, n_live = 0, n_reclaim = 0;
    for (uint16_t s = 0; s < W25Q_SECTOR_COUNT; s++)
    {
        switch (slot_state[s])
        {
            case SLOT_FREE:    n_free++;    break;
            case SLOT_PENDING: n_pending++; break;
            case SLOT_LIVE:    n_live++;    break;
            case SLOT_RECLAIM: n_reclaim++; break;
            default: break;
        }
    }

    printf("\r\n=== storage_debug_dump ===\r\n");
    printf("dir_count=%u  next_seq=%lu  erase[min=%lu max=%lu]\r\n",
           directory_count, (unsigned long)next_seq,
           (unsigned long)min_erase, (unsigned long)max_erase);
    printf("slots: free=%u live=%u reclaim=%u pending=%u  (free_count=%u)\r\n",
           n_free, n_live, n_reclaim, n_pending, free_count);

    for (uint16_t d = 0; d < directory_count; d++)
    {
        printf("  [%u] type=%u inst=%u sector=%u seq=%lu\r\n",
               d, directory[d].type, directory[d].instance,
               directory[d].sector, (unsigned long)directory[d].sequence);
    }
    printf("==========================\r\n");
}


