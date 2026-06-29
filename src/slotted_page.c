/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file slotted_page.c
 * @brief Slotted page implementation for variable-length records.
 *
 * Pages use a header + slot directory growing forward, with record
 * data growing backward from the end. This layout supports insertions
 * and deletions without full-page compaction. The page header tracks
 * node type, level, and the B-tree node metadata.
 */

#include "slotted_page.h"
#include "db_header.h"
#include <string.h>

garry_slot_entry garry_read_slot(garry_page_buffer* buf, garry_i32 idx)
{
    garry_slot_entry entry;
    garry_i32 slot_offset, lo, hi;
    slot_offset = GARRY_PAGE_HEADER_SIZE + idx * GARRY_SLOT_SIZE;
    lo = (garry_i32)(*buf)[slot_offset];
    hi = (garry_i32)(*buf)[slot_offset + 1];
    if (lo < 0) lo += 256;
    if (hi < 0) hi += 256;
    entry.offset = lo + hi * 256;
    lo = (garry_i32)(*buf)[slot_offset + 2];
    hi = (garry_i32)(*buf)[slot_offset + 3];
    if (lo < 0) lo += 256;
    if (hi < 0) hi += 256;
    entry.length = lo + hi * 256;
    return entry;
}

void garry_write_slot(garry_page_buffer buf, garry_i32 idx, garry_slot_entry entry)
{
    garry_i32 slot_offset;
    slot_offset = GARRY_PAGE_HEADER_SIZE + idx * GARRY_SLOT_SIZE;
    buf[slot_offset]     = (garry_byte)(entry.offset % 256);
    buf[slot_offset + 1] = (garry_byte)((entry.offset / 256) % 256);
    buf[slot_offset + 2] = (garry_byte)(entry.length % 256);
    buf[slot_offset + 3] = (garry_byte)((entry.length / 256) % 256);
}

garry_i32 garry_page_record_count(garry_page_buffer* buf)
{
    return garry_read_int32((garry_byte*)*buf, 8);
}

void garry_page_init(garry_page_buffer buf, garry_node_kind ptype, garry_i32 level, garry_i32 page_size)
{
    garry_i32 i;
    for (i = 0; i < page_size; i++) {
        buf[i] = 0;
    }
    garry_write_int32(buf, 0, (garry_i32)ptype);
    garry_write_int32(buf, 4, level);
    garry_write_int32(buf, 8, 0);
    garry_write_int32(buf, 12, GARRY_PAGE_HEADER_SIZE);
    garry_write_int32(buf, 16, page_size);
    garry_write_int32(buf, 20, 0);
    garry_write_int32(buf, 24, 0);
    garry_write_int32(buf, 28, 0);
}

garry_i32 garry_page_insert(garry_page_buffer buf, const garry_byte* data, garry_i32 dlen, garry_i32 page_size)
{
    garry_i32 count, free_start, free_end, needed;
    garry_slot_entry slot;
    (void)page_size;
    count = garry_read_int32(buf, 8);
    free_start = garry_read_int32(buf, 12);
    free_end = garry_read_int32(buf, 16);
    if (free_start + GARRY_SLOT_SIZE > free_end) {
        return -1;
    }
    needed = dlen + GARRY_SLOT_SIZE;
    if (free_end - free_start < needed) {
        return -1;
    }
    if (free_end - dlen < free_start + GARRY_SLOT_SIZE) {
        return -1;
    }
    free_end = free_end - dlen;
    slot.offset = free_end;
    slot.length = dlen;
    garry_write_slot(buf, count, slot);
    free_start = free_start + GARRY_SLOT_SIZE;
    garry_write_int32(buf, 8, count + 1);
    garry_write_int32(buf, 12, free_start);
    garry_write_int32(buf, 16, free_end);
    garry_copy_bytes_in(buf, free_end, data, dlen);
    return count;
}

garry_i32 garry_page_get(garry_page_buffer* buf, garry_i32 slot_idx, garry_byte* data, garry_i32 page_size)
{
    garry_slot_entry entry;
    garry_i32 i;
    (void)page_size;
    if (slot_idx < 0 || slot_idx >= garry_page_record_count(buf)) return -1;
    entry = garry_read_slot(buf, slot_idx);
    for (i = 0; i < entry.length; i++) {
        data[i] = (*buf)[entry.offset + i];
    }
    return entry.length;
}

void garry_copy_bytes_in(garry_page_buffer buf, garry_i32 offset, const garry_byte* src, garry_i32 len)
{
    garry_i32 i;
    for (i = 0; i < len; i++) {
        buf[offset + i] = src[i];
    }
}
