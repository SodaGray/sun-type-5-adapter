//
// Created by Cherry on 2026/5/29.
//

#include "sector_io.h"

#include "crc32.h"

bool sector_read_preamble(uint32_t sector_idx, uint32_t *magic_out, uint32_t *count_out)
{
    if (sector_idx >= W25Q_SECTOR_COUNT || magic_out == NULL || count_out == NULL)
    {
        return false;
    }

    uint32_t sector_base_addr = sector_idx * W25Q_SECTOR_SIZE;

    uint8_t buffer[8] = {0};
    if (!w25q_read(sector_base_addr + OFFSET_MAGIC, buffer, sizeof(buffer)))
    {
        return false;
    }

    *magic_out = ((uint32_t)buffer[3] << 24) |
                 ((uint32_t)buffer[2] << 16) |
                 ((uint32_t)buffer[1] <<  8) |
                 ((uint32_t)buffer[0]      );

    *count_out = ((uint32_t)buffer[7] << 24) |
                 ((uint32_t)buffer[6] << 16) |
                 ((uint32_t)buffer[5] <<  8) |
                 ((uint32_t)buffer[4]      );

    return true;
}

sector_state_t sector_classify(uint32_t magic, uint32_t count)
{
    if (magic == SECTOR_MAGIC_OBJ) return SECTOR_STATE_OBJECT;
    if (magic == SECTOR_MAGIC_BLANK && count == ERASE_COUNT_BLANK) return SECTOR_STATE_VIRGIN;
    if (magic == SECTOR_MAGIC_BLANK && count != ERASE_COUNT_BLANK) return SECTOR_STATE_FREE;
    return SECTOR_STATE_RECLAIM;
}

uint32_t sector_logical_count(uint32_t magic, uint32_t count)
{
    if (sector_classify(magic, count) == SECTOR_STATE_VIRGIN)
    {
        return 0;
    }

    return count;
}

bool sector_read_obj_header(uint32_t sector_idx, obj_header_t *header_out)
{
    if (sector_idx >= W25Q_SECTOR_COUNT || header_out == NULL)
    {
        return false;
    }

    uint32_t sector_base_addr = sector_idx * W25Q_SECTOR_SIZE;

    if (!w25q_read(sector_base_addr + OFFSET_MAGIC,
                   (uint8_t *)header_out,
                   OBJ_HEADER_SIZE))
    {
        return false;
    }

    return true;
}

bool sector_read_payload(uint32_t sector_idx, size_t offset_in_payload, uint8_t *buf, size_t len)
{
    if (sector_idx >= W25Q_SECTOR_COUNT || buf == NULL) return false;

    uint32_t base_addr = sector_idx * W25Q_SECTOR_SIZE + OFFSET_PAYLOAD + offset_in_payload;

    if (!w25q_read(base_addr, buf, len)) return false;

    return true;
}

bool sector_verify_object(uint32_t sector_idx)
{
    obj_header_t header;
    uint8_t scratch[256];

    if (!sector_read_obj_header(sector_idx, &header)) return false;

    if (crc32_compute(&header, HEADER_CRC_RANGE_LEN) != header.header_crc) return false;

    if (header.payload_len == 0 || header.payload_len > MAX_PAYLOAD_SIZE) return false;

    crc32_begin();
    uint16_t remaining = header.payload_len;
    uint16_t offset    = 0;

    while (remaining > 0)
    {
        uint16_t chunk = (remaining < sizeof(scratch)) ? remaining : sizeof(scratch);

        if (!sector_read_payload(sector_idx, offset, scratch, chunk)) return false;

        crc32_update(scratch, chunk);
        offset    += chunk;
        remaining -= chunk;
    }

    if (crc32_end() != header.payload_crc) return false;

    return true;
}

bool sector_write_object(uint32_t sector_idx, uint16_t type, uint16_t instance,
                         uint32_t sequence, uint32_t erase_count,
                         const uint8_t *payload, size_t payload_len)
{
    if (sector_idx >= W25Q_SECTOR_COUNT || payload == NULL) return false;
    if (payload_len == 0 || payload_len > MAX_PAYLOAD_SIZE) return false;

    uint32_t payload_crc = crc32_compute(payload, payload_len);

    obj_header_t header;
    header.magic       = SECTOR_MAGIC_OBJ;
    header.erase_count = erase_count;
    header.type        = type;
    header.instance    = instance;
    header.sequence    = sequence;
    header.payload_len = (uint16_t)payload_len;
    header.reserved_lo = 0xFFFFu;
    header.reserved_hi = 0xFFFFu;
    header.payload_crc = payload_crc;
    header.state_byte  = STATE_FRESH;  // 关键：处于未提交的中间状态
    header.pad         = 0xFF;

    header.header_crc = crc32_compute(&header, HEADER_CRC_RANGE_LEN);

    uint32_t base_addr = sector_idx * W25Q_SECTOR_SIZE;

    if (!w25q_program(base_addr, (uint8_t *)&header, sizeof(header)))
    {
        return false;
    }

    if (!w25q_program(base_addr + OFFSET_PAYLOAD, payload, payload_len))
    {
        return false;
    }

    return true;
}

bool sector_commit(uint32_t sector_idx)
{
    if (sector_idx >= W25Q_SECTOR_COUNT) return false;

    uint32_t base_addr = sector_idx * W25Q_SECTOR_SIZE;

    uint8_t state = STATE_COMMITTED;
    if (!w25q_program(base_addr + OFFSET_STATE_BYTE, &state, sizeof(state))) return false;

    return true;
}

bool sector_mark_obsolete(uint32_t sector_idx)
{
    if (sector_idx >= W25Q_SECTOR_COUNT) return false;

    uint32_t base_addr = sector_idx * W25Q_SECTOR_SIZE;

    uint8_t state = STATE_OBSOLETE;
    if (!w25q_program(base_addr + OFFSET_STATE_BYTE, &state, sizeof(state))) return false;

    return true;
}

bool sector_recycle(uint32_t sector_idx, uint32_t new_erase_count)
{
    if (sector_idx >= W25Q_SECTOR_COUNT) return false;
    uint32_t base_addr = sector_idx * W25Q_SECTOR_SIZE;

    uint32_t dead = SECTOR_MAGIC_DEAD;   /* 0x00000000 */
    if (!w25q_program(base_addr + OFFSET_MAGIC, (uint8_t *)&dead, sizeof(dead)))
        return false;

    if (!w25q_erase_sector(base_addr)) return false;

    if (!w25q_program(base_addr + OFFSET_ERASE_COUNT,
                      (uint8_t *)&new_erase_count, sizeof(new_erase_count)))
        return false;

    return true;
}