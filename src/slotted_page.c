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

/**
 * @brief Read a slot entry from the slot directory.
 *
 * Decodes the offset and length fields from the slot at position @p idx.
 *
 * @param buf  Page buffer.
 * @param idx  Slot index to read.
 * @return Decoded slot entry with offset and length.
 */
garry_slot_entry garry_read_slot(garry_page_buffer *buf, garry_i32 idx)
{
    garry_slot_entry entry;
    garry_i32 slot_offset, lo, hi;
    slot_offset = GARRY_PAGE_HEADER_SIZE + idx * GARRY_SLOT_SIZE;
    lo = (garry_i32)(*buf)[slot_offset];
    hi = (garry_i32)(*buf)[slot_offset + 1];
    if (lo < 0)
        lo += 256;
    if (hi < 0)
        hi += 256;
    entry.offset = lo + hi * 256;
    lo = (garry_i32)(*buf)[slot_offset + 2];
    hi = (garry_i32)(*buf)[slot_offset + 3];
    if (lo < 0)
        lo += 256;
    if (hi < 0)
        hi += 256;
    entry.length = lo + hi * 256;
    return entry;
}

/**
 * @brief Write a slot entry into the slot directory.
 *
 * Encodes the offset and length fields into the slot at position @p idx.
 *
 * @param buf    Page buffer.
 * @param idx    Slot index to write.
 * @param entry  Slot entry to encode.
 */
void garry_write_slot(garry_page_buffer buf, garry_i32 idx, garry_slot_entry entry)
{
    garry_i32 slot_offset;
    slot_offset = GARRY_PAGE_HEADER_SIZE + idx * GARRY_SLOT_SIZE;
    buf[slot_offset] = (garry_byte)(entry.offset % 256);
    buf[slot_offset + 1] = (garry_byte)((entry.offset / 256) % 256);
    buf[slot_offset + 2] = (garry_byte)(entry.length % 256);
    buf[slot_offset + 3] = (garry_byte)((entry.length / 256) % 256);
}

/**
 * @brief Read the record count from the page header.
 *
 * @param buf  Page buffer.
 * @return Number of records stored in the page.
 */
garry_i32 garry_page_record_count(garry_page_buffer *buf)
{
    return garry_read_int32((garry_byte *)*buf, GARRY_PAGE_HDR_OFF_COUNT);
}

/**
 * @brief Initialize a page buffer with default header values.
 *
 * Zeros the entire page and writes the node type, level, and
 * initializes free-space pointers to their initial positions.
 *
 * @param buf       Page buffer to initialize.
 * @param ptype     Node type (leaf, internal, etc.).
 * @param level     Tree level of this node.
 * @param page_size Total page size in bytes.
 */
void garry_page_init(garry_page_buffer buf, garry_node_kind ptype, garry_i32 level,
                     garry_i32 page_size)
{
    garry_i32 i;
    for (i = 0; i < page_size; i++)
    {
        buf[i] = 0;
    }
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_TYPE, (garry_i32)ptype);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_LEVEL, level);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_COUNT, 0);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_FREE_START, GARRY_PAGE_HEADER_SIZE);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_FREE_END, page_size);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_COMPRESSION, 0);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_PARENT, 0);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_CHECKSUM, 0);
}

/**
 * @brief Insert a record into a slotted page.
 *
 * Appends a new slot to the directory and copies the record data
 * into the free space at the end of the page. Returns the index
 * of the newly inserted slot.
 *
 * @param buf       Page buffer.
 * @param data      Record data to insert.
 * @param dlen      Length of record data.
 * @param page_size Total page size in bytes.
 * @return Slot index of the inserted record, or -1 if the page is full.
 */
garry_i32 garry_page_insert(garry_page_buffer buf, const garry_byte *data, garry_i32 dlen,
                            garry_i32 page_size)
{
    garry_i32 count, free_start, free_end, needed;
    garry_slot_entry slot;
    count = garry_read_int32(buf, GARRY_PAGE_HDR_OFF_COUNT);
    free_start = garry_read_int32(buf, GARRY_PAGE_HDR_OFF_FREE_START);
    free_end = garry_read_int32(buf, GARRY_PAGE_HDR_OFF_FREE_END);
    if (count >= GARRY_MAX_SLOTS)
    {
        return -1;
    }
    if (free_start + GARRY_SLOT_SIZE > free_end)
    {
        return -1;
    }
    if (free_end - dlen < GARRY_PAGE_HEADER_SIZE)
    {
        return -1;
    }
    needed = dlen + GARRY_SLOT_SIZE;
    if (free_end - free_start < needed)
    {
        return -1;
    }
    if (free_end - dlen < free_start + GARRY_SLOT_SIZE)
    {
        return -1;
    }
    free_end = free_end - dlen;
    slot.offset = free_end;
    slot.length = dlen;
    garry_write_slot(buf, count, slot);
    free_start = free_start + GARRY_SLOT_SIZE;
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_COUNT, count + 1);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_FREE_START, free_start);
    garry_write_int32(buf, GARRY_PAGE_HDR_OFF_FREE_END, free_end);
    garry_copy_bytes_in(buf, free_end, data, dlen);
    return count;
}

/**
 * @brief Retrieve a record from a slotted page by slot index.
 *
 * Looks up the slot entry and copies the record data into the output buffer.
 *
 * @param buf       Page buffer.
 * @param slot_idx  Index of the slot to retrieve.
 * @param data      Output buffer for the record data.
 * @param page_size Total page size in bytes (for bounds checking).
 * @return Length of the retrieved record, or -1 on invalid index.
 */
garry_i32 garry_page_get(garry_page_buffer *buf, garry_i32 slot_idx, garry_byte *data,
                         garry_i32 page_size)
{
    garry_slot_entry entry;
    garry_i32 i;
    if (slot_idx < 0 || slot_idx >= garry_page_record_count(buf))
        return -1;
    entry = garry_read_slot(buf, slot_idx);
    if (entry.length > page_size - GARRY_PAGE_HEADER_SIZE)
        return -1;
    for (i = 0; i < entry.length; i++)
    {
        data[i] = (*buf)[entry.offset + i];
    }
    return entry.length;
}

/**
 * @brief Copy bytes into a page buffer at a given offset.
 *
 * Byte-by-byte copy from a source buffer into the page at the
 * specified offset position.
 *
 * @param buf    Page buffer to write into.
 * @param offset Byte offset within the page.
 * @param src    Source data to copy.
 * @param len    Number of bytes to copy.
 */
void garry_copy_bytes_in(garry_page_buffer buf, garry_i32 offset, const garry_byte *src,
                         garry_i32 len)
{
    garry_i32 i;
    for (i = 0; i < len; i++)
    {
        buf[offset + i] = src[i];
    }
}
