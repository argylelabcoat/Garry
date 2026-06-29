/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_modify.h
 * @brief B-tree insert and delete operations.
 */

#ifndef GARRY_BTREE_MODIFY_H
#define GARRY_BTREE_MODIFY_H

#include "garry/types.h"
#include "storage_types.h"
#include "storage_core.h"
#include "btree_node.h"

/**
 * @brief Insert a key-value pair into the B-tree.
 *
 * Traverses the tree to find the correct leaf, inserts the entry,
 * and splits nodes if they overflow. May grow the tree height.
 *
 * @param pool   Buffer pool for page allocation
 * @param root   Pointer to root page ID (may be updated on split)
 * @param key    Key bytes to insert
 * @param klen   Key length in bytes
 * @param value  Value bytes to insert
 * @param vlen   Value length in bytes
 * @return GARRY_TRUE on success, GARRY_FALSE on allocation failure
 */
garry_bool garry_btree_insert(garry_buffer_pool *pool, garry_i32 *root,
                              const garry_byte *key, garry_i32 klen,
                              const garry_byte *value, garry_i32 vlen);

/**
 * @brief Delete a key from the B-tree.
 *
 * Locates the key's leaf node and removes the entry. If the leaf
 * underflows, rebalances by merging or redistributing with siblings.
 * May shrink the tree height.
 *
 * @param pool  Buffer pool for page access
 * @param root  Pointer to root page ID (may be updated on merge)
 * @param key   Key bytes to delete
 * @param klen  Key length in bytes
 */
void garry_btree_delete(garry_buffer_pool *pool, garry_i32 *root,
                        const garry_byte *key, garry_i32 klen);

#endif /* GARRY_BTREE_MODIFY_H */
