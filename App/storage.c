//
// Created by Cherry on 2026/5/31.
//

#define SECTOR_NONE  0xFFFFu
#define DIR_NONE 0xFFFFu

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
        // 放在 liveness 检查之前：obsolete 但合法的对象也要计入,
        // 才能保证 next_seq > 所有 OBJECT 序号。
        if (header.sequence >= next_seq)
        {
            next_seq = header.sequence + 1;
        }

        // 原子生命周期审查：历史旧数据、或写一半断电的 FRESH 数据,都视为废弃等 GC
        if (!STATE_IS_LIVE(header.state_byte))
        {
            slot_state[idx] = SLOT_RECLAIM;
            continue;
        }

        // 到这里，此扇区持有一个合法且已提交的 Live 对象。开始冲突裁决:
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
                // 目录已满：正常不会发生(save 会在 256 上限处拒绝新对象)。
                // 如果确实发生，flash 上 live 对象数超过 RAM 目录容量,此对象将被丢弃。
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
                // 当前扇区更新
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


static uint16_t alloc_free_sector(void)
{
    uint32_t min_ec = 0xFFFFFFFFu;
    uint16_t sector = SECTOR_NONE;
    for (uint16_t i = 0; i < W25Q_SECTOR_COUNT; i++)
    {
        if (slot_state[i] != SLOT_FREE) continue;
        if (erase_count[i] < min_ec)
        {
            min_ec = erase_count[i];
            sector = i;
        }
    }

    return sector;
}

static uint16_t directory_find(uint16_t type, uint16_t instance)
{
    for (uint16_t idx = 0; idx < directory_count; idx++)
    {
        if (directory[idx].type == type && directory[idx].instance == instance)
        {
            return idx;
        }
    }
    return DIR_NONE;
}

static bool storage_gc(void)
{
    uint32_t min_ec = 0xFFFFFFFFu;
    uint16_t victim_idx = SECTOR_NONE;

    for (uint16_t i = 0; i < W25Q_SECTOR_COUNT; i++)
    {
        if (slot_state[i] != SLOT_RECLAIM) continue;
        if (erase_count[i] == 0xFFFFFFFFu) continue;

        if (erase_count[i] < min_ec)
        {
            min_ec = erase_count[i];
            victim_idx = i;
        }
    }

    if (victim_idx == SECTOR_NONE)
    {
        return false;
    }

    uint32_t new_ec = erase_count[victim_idx] + 1;

    if (!sector_recycle(victim_idx, new_ec))
    {
        // 但是问题是无法确定 recycle 函数是在哪个时间节点返回的 false。
        // 不管了到时候再说吧
        erase_count[victim_idx] = 0xFFFFFFFFu;   // 退役坏块,免得永远被重挑
        return false;
    }

    slot_state[victim_idx] = SLOT_FREE;
    erase_count[victim_idx] = new_ec;
    free_count++;

    if (new_ec > max_erase) max_erase = new_ec;

    return true;
}

bool storage_save(uint16_t type, uint16_t instance, const void *data, size_t len)
{
    if (data == NULL || !(len >= 1 && len <= STORAGE_MAX_OBJECT_SIZE)) return false;

    uint16_t item = directory_find(type, instance);
    bool create = (item == DIR_NONE);

    if (create && directory_count >= STORAGE_MAX_LIVE_OBJECTS)
    {
        return false;
    }

    uint16_t s_new = alloc_free_sector();
    if (s_new == SECTOR_NONE)
    {
        if (!storage_gc()) return false;
        s_new = alloc_free_sector();
        if (s_new == SECTOR_NONE) return false;
    }
    free_count--;

    uint32_t ec = erase_count[s_new];
    uint32_t seq = next_seq++;

    if (!sector_write_object(s_new, type, instance, seq, ec, data, len)) {
        slot_state[s_new] = SLOT_RECLAIM;
        return false;
    }
    if (!sector_verify_object(s_new)) {
        slot_state[s_new] = SLOT_RECLAIM;
        return false;
    }
    if (!sector_commit(s_new)) {
        slot_state[s_new] = SLOT_RECLAIM;
        return false;
    }

    slot_state[s_new] = SLOT_LIVE;

    if (create)
    {
        item = directory_count++;
        directory[item].type = type;
        directory[item].instance = instance;
    }
    else
    {
        uint16_t s_old = directory[item].sector;
        sector_mark_obsolete(s_old);
        slot_state[s_old] = SLOT_RECLAIM;

    }

    directory[item].sector = s_new;
    directory[item].sequence = seq;

    return true;
}

int storage_load(uint16_t type, uint16_t instance, void *buf, size_t max_len)
{
    if (buf == NULL) return STORAGE_ERR_INVALID_PARAM;

    uint16_t item = directory_find(type, instance);
    if (item == DIR_NONE) return STORAGE_ERR_NOT_FOUND;

    uint16_t s = directory[item].sector;
    obj_header_t hdr;

    if (!sector_read_obj_header(s, &hdr)) return STORAGE_ERR_IO;
    if (hdr.payload_len > max_len) return STORAGE_ERR_INSUFFICIENT_SPACE;

    if (!sector_verify_object(s)) return STORAGE_ERR_CORRUPT;

    if (!sector_read_payload(s, 0, (uint8_t*) buf, hdr.payload_len)) return STORAGE_ERR_IO;

    return hdr.payload_len;
}

bool storage_delete(uint16_t type, uint16_t instance)
{
    uint16_t item = directory_find(type, instance);
    if (item == DIR_NONE) return false;

    uint16_t s_old = directory[item].sector;
    if (!sector_mark_obsolete(s_old)) return false;

    slot_state[s_old] = SLOT_RECLAIM;

    directory_count--;
    if (item < directory_count)
    {
        directory[item] = directory[directory_count];
    }

    return true;
}

int storage_count(uint16_t type)
{
    int count = 0;
    for (uint16_t idx = 0; idx < directory_count; idx++)
    {
        if (directory[idx].type == type)
        {
            count++;
        }
    }
    return count;
}

bool storage_enum(uint16_t type, size_t index, uint16_t *instance_out)
{
    if (instance_out == NULL) return false;
    size_t match = 0;
    for (uint16_t idx = 0; idx < directory_count; idx++)
        if (directory[idx].type == type) {
            if (match == index)
            {
                *instance_out = directory[idx].instance; return true;
            }
            match++;
        }
    return false;
}

bool storage_format(void)
{
    for (uint16_t i = 0; i < W25Q_SECTOR_COUNT; i++)
    {
        uint32_t magic, count;
        /* 已是 virgin(全 0xFF)就跳过——省时间,也不白擦一次寿命 */
        if (sector_read_preamble(i, &magic, &count)
            && magic == SECTOR_MAGIC_BLANK && count == ERASE_COUNT_BLANK)
        {
            continue;
        }
        if (!w25q_erase_range((uint32_t)i * W25Q_SECTOR_SIZE, W25Q_SECTOR_SIZE))
        {
            return false;
        }
    }
    return storage_mount();   /* 擦完重建 RAM:dir_count=0, free=2048 */
}

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