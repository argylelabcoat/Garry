/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_core.c
 * @brief Page header size calculation and construction helpers.
 *
 * Computes the fixed overhead of the slotted page header and
 * constructs page header values for new pages.
 */

#include "storage_core.h"

/**
 * @brief Return the size in bytes of a slotted page header.
 */
garry_i32 garry_page_header_size(void)
{
    return GARRY_PAGE_HEADER_SIZE;
}

/**
 * @brief Create a new page header with default values.
 *
 * Initializes a page header with the given type and level, zero records,
 * and free space spanning the full page after the header.
 *
 * @param pt    Page type (leaf, internal, chain, overflow, etc.).
 * @param level Tree level (0 for leaf).
 *
 * @return Initialized page header struct.
 */
garry_page_header garry_create_header(garry_page_type pt, garry_i32 level)
{
    garry_page_header hdr;
    /* precondition: level >= 0 */
    hdr.page_type = pt;
    hdr.page_level = level;
    hdr.num_records = 0;
    hdr.free_start = GARRY_PAGE_HEADER_SIZE;
    hdr.free_end = GARRY_MAX_PAGE_SIZE;
    hdr.compression = 0;
    hdr.parent_page = 0;
    hdr.checksum = 0;
    return hdr;
}
