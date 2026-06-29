/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_search.h
 * @brief B-tree key lookup operations.
 */

#ifndef GARRY_BTREE_SEARCH_H
#define GARRY_BTREE_SEARCH_H

#include "garry/types.h"
#include "storage_types.h"
#include "btree_node.h"

#ifndef GARRY_LEAF_NODE
#define GARRY_LEAF_NODE     GARRY_NODE_LEAF
#endif
#ifndef GARRY_INTERNAL_NODE
#define GARRY_INTERNAL_NODE GARRY_NODE_INTERNAL
#endif

/**
 * @brief Find the insertion position for a key in a leaf node.
 *
 * Binary searches the leaf's keys to locate where the given key would
 * be inserted. Returns the index where the key matches or should be inserted.
 *
 * @param node  Leaf node to search
 * @param key   Key bytes to find
 * @param klen  Key length in bytes
 * @return Index of matching key, or insertion position (0..entry_count)
 */
garry_i32 garry_leaf_find(garry_btree_node* node, const garry_byte* key, garry_i32 klen);

/**
 * @brief Find the child pointer for a key in an internal node.
 *
 * Binary searches the internal node's keys to find which child subtree
 * contains the given key. Returns the child index for descent.
 *
 * @param node  Internal node to search
 * @param key   Key bytes to find
 * @param klen  Key length in bytes
 * @return Child index (0..entry_count) for the subtree containing key
 */
garry_i32 garry_internal_find(garry_btree_node* node, const garry_byte* key, garry_i32 klen);

/* Search for a key's value in a leaf tree rooted at `root`. */
garry_bool garry_leaf_find_search(garry_buffer_pool *pool, garry_i32 root,
                                  const garry_byte *key, garry_i32 klen,
                                  garry_byte *result, garry_i32 *result_len);

#endif /* GARRY_BTREE_SEARCH_H */
