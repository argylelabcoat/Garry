/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_core.h
 * @brief Core page type definitions and page header utilities.
 */

#ifndef GARRY_STORAGE_CORE_H
#define GARRY_STORAGE_CORE_H

#include "garry/config.h"
#include "garry/types.h"

/**
 * @brief Page type identifiers for the storage engine.
 *
 * Ordinal values must stay stable: internal=0, leaf=1, overflow=2, free=3, chain=4.
 * These values are stored on disk and used for page dispatch.
 */
typedef enum {
    GARRY_NODE_INTERNAL = 0,
    GARRY_NODE_LEAF,
    GARRY_NODE_OVERFLOW,
    GARRY_NODE_FREE,
    GARRY_NODE_CHAIN
} garry_node_kind;

typedef garry_node_kind garry_page_type;

typedef struct {
    garry_page_type page_type;
    garry_i32 page_level;
    garry_i32 num_records;
    garry_i32 free_start;
    garry_i32 free_end;
    garry_i32 compression;
    garry_i32 parent_page;
    garry_i32 checksum;
} garry_page_header;

/**
 * @brief Fixed-size page buffer for the storage engine.
 *
 * Large enough for the maximum page size (64KB). All page I/O
 * uses this buffer type to ensure consistent alignment.
 */
typedef garry_byte garry_page_buffer[GARRY_MAX_PAGE_SIZE];

/**
 * @brief Get the size of a page header in bytes.
 *
 * Returns the fixed size of the serialized page header structure.
 * Used to calculate available space for records within a page.
 *
 * @return Size of page header in bytes
 */
garry_i32 garry_page_header_size(void);

/**
 * @brief Create a new page header with the given type and level.
 *
 * Initializes a header with zero records and appropriate free space bounds.
 * Precondition: level >= 0.
 *
 * @param pt     Page type (leaf, internal, overflow, etc.)
 * @param level  B-tree level (0 = leaf)
 * @return Initialized page header
 */
garry_page_header garry_create_header(garry_page_type pt, garry_i32 level);

#endif /* GARRY_STORAGE_CORE_H */
