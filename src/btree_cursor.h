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

typedef struct
{
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

/**
 * @brief Open a B-tree cursor positioned at the first matching key.
 *
 * Descends from the root to the leftmost leaf (or the leaf containing
 * the first key matching @p prefix) and returns a cursor handle.
 *
 * @param pool    Buffer pool for page access
 * @param root    Root page ID of the B-tree
 * @param prefix  Optional prefix filter (NULL for all keys)
 * @param plen    Length of the prefix in bytes
 * @return Cursor handle positioned before the first matching key
 */
garry_btree_cursor_handle garry_btree_cursor_open(garry_buffer_pool *pool, garry_i32 root,
                                                  const garry_byte_array *prefix, garry_u32 plen);

/**
 * @brief Advance the cursor to the next key.
 *
 * Steps to the next leaf slot, crossing page boundaries as needed.
 * Skips keys that don't match the cursor's prefix filter.
 *
 * @param pool  Buffer pool for page access
 * @param cur   Cursor handle to advance
 * @param key   Output: the next key's byte array
 * @param klen  Output: length of the key
 * @return GARRY_TRUE if a next key exists, GARRY_FALSE if exhausted
 */
garry_bool garry_btree_cursor_next(garry_buffer_pool *pool, garry_btree_cursor_handle *cur,
                                   garry_byte_array *key, garry_u32 *klen);

/**
 * @brief Peek at the current cursor position without advancing.
 *
 * Returns the key at the cursor's current position. If no key has
 * been loaded yet, loads the current leaf page to retrieve it.
 *
 * @param pool  Buffer pool for page access
 * @param cur   Cursor handle to peek at
 * @param key   Output: the current key's byte array
 * @param klen  Output: length of the key
 * @return GARRY_TRUE if a key is available, GARRY_FALSE if exhausted
 */
garry_bool garry_btree_cursor_peek(garry_buffer_pool *pool, const garry_btree_cursor_handle *cur,
                                   garry_byte_array *key, garry_u32 *klen);

/**
 * @brief Get the value at the current cursor position.
 *
 * Copies the value from the last next() call into the output buffer.
 * Must be called after a successful next() or peek().
 *
 * @param cur  Cursor handle to read the value from
 * @param out  Output buffer for the value byte array
 */
void garry_btree_cursor_value(const garry_btree_cursor_handle *cur, garry_byte_array *out);

/**
 * @brief Close the cursor and mark it as exhausted.
 *
 * Sets the exhausted flag so no further iterations can occur.
 *
 * @param cur  Cursor handle to close
 */
void garry_btree_cursor_close(garry_btree_cursor_handle *cur);

#endif /* GARRY_BTREE_CURSOR_H */
