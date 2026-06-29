/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_cursor.h
 * @brief B-tree cursor handle for positional iteration.
 */

#ifndef GARRY_BTREE_CURSOR_H
#define GARRY_BTREE_CURSOR_H

#include "storage_types.h"
#include "storage_core.h"
#include "btree_node.h"
#include "buffer_pool.h"

typedef struct {
    garry_i32 root;
    garry_byte_array prefix;
    garry_u32 prefix_len;
    garry_i32 current_page;
    garry_u32 current_slot;
    garry_byte_array current;
    garry_u32 current_len;
    garry_byte_array current_value;
    garry_bool exhausted;
    garry_byte_array saved_key;
    garry_u32 saved_klen;
    garry_bool has_saved;
    garry_bool node_loaded;
    garry_btree_node current_node;
} garry_btree_cursor_handle;

garry_btree_cursor_handle garry_btree_cursor_open(garry_buffer_pool *pool, garry_i32 root,
                                      const garry_byte_array *prefix, garry_u32 plen);
garry_bool garry_btree_cursor_next(garry_buffer_pool *pool, garry_btree_cursor_handle *cur,
                             garry_byte_array *key, garry_u32 *klen);
garry_bool garry_btree_cursor_peek(garry_buffer_pool *pool, const garry_btree_cursor_handle *cur,
                             garry_byte_array *key, garry_u32 *klen);
void garry_btree_cursor_value(const garry_btree_cursor_handle *cur, garry_byte_array *out);
void garry_btree_cursor_close(garry_btree_cursor_handle *cur);

#endif /* GARRY_BTREE_CURSOR_H */
