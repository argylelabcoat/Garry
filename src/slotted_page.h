/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file slotted_page.h
 * @brief Slotted page layout for B-tree node storage.
 */

#ifndef GARRY_SLOTTED_PAGE_H
#define GARRY_SLOTTED_PAGE_H

#include "garry/config.h"
#include "garry/types.h"
#include "storage_core.h"

typedef struct {
    garry_i32 offset;
    garry_i32 length;
} garry_slot_entry;

/**
 * @brief Read a slot entry from the page's slot directory.
 *
 * @param buf  Page buffer containing the slot directory
 * @param idx  Slot index to read
 * @return Slot entry with offset and length fields
 */
garry_slot_entry garry_read_slot(garry_page_buffer* buf, garry_i32 idx);

/**
 * @brief Write a slot entry to the page's slot directory.
 *
 * @param buf    Page buffer to write to
 * @param idx    Slot index to write
 * @param entry  Slot entry with offset and length fields
 */
void garry_write_slot(garry_page_buffer buf, garry_i32 idx, garry_slot_entry entry);

/**
 * @brief Get the number of records stored in a page.
 *
 * Reads the record count from the page header.
 *
 * @param buf  Page buffer to query
 * @return Number of records in the page
 */
garry_i32 garry_page_record_count(garry_page_buffer* buf);

/**
 * @brief Initialize a page with the given type and level.
 *
 * Sets up the page header, initializes the slot directory to empty,
 * and configures free space boundaries.
 *
 * @param buf       Page buffer to initialize
 * @param ptype     Page type (leaf, internal, overflow, etc.)
 * @param level     B-tree level (0 = leaf)
 * @param page_size Total page size in bytes
 */
void garry_page_init(garry_page_buffer buf, garry_node_kind ptype, garry_i32 level, garry_i32 page_size);

/**
 * @brief Insert a record into a slotted page.
 *
 * Appends the record data at the end of the page (growing backward)
 * and adds a slot entry pointing to it. Returns the slot index.
 *
 * @param buf        Page buffer to insert into
 * @param data       Record data to insert
 * @param dlen       Record data length in bytes
 * @param page_size  Total page size in bytes
 * @return Slot index of the inserted record, or -1 if page is full
 */
garry_i32 garry_page_insert(garry_page_buffer buf, const garry_byte* data, garry_i32 dlen, garry_i32 page_size);

/**
 * @brief Read a record from a slotted page by slot index.
 *
 * Looks up the slot entry and copies the record data to the output buffer.
 *
 * @param buf       Page buffer to read from
 * @param slot_idx  Slot index to read
 * @param data      Output buffer for record data
 * @param page_size Total page size in bytes
 * @return Number of bytes copied, or -1 if slot is invalid
 */
garry_i32 garry_page_get(garry_page_buffer* buf, garry_i32 slot_idx, garry_byte* data, garry_i32 page_size);

/**
 * @brief Copy raw bytes into a page at a specific offset.
 *
 * Used for low-level page manipulation. Does not update slot entries.
 *
 * @param buf     Page buffer to write to
 * @param offset  Byte offset within the page
 * @param src     Source bytes to copy
 * @param len     Number of bytes to copy
 */
void garry_copy_bytes_in(garry_page_buffer buf, garry_i32 offset, const garry_byte* src, garry_i32 len);

#endif /* GARRY_SLOTTED_PAGE_H */
