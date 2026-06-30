/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_search.c
 * @brief B-tree key search with binary search at each node.
 *
 * Descends from root to leaf using binary search to select the correct
 * child pointer at each internal node. On the leaf, binary search
 * locates the exact key or the position where it would be inserted.
 */

#include "btree_search.h"
#include "btree_compression.h"
#include "key_encoding.h"
#include "garry/encoding.h"
#include "buffer_pool.h"
#include "btree_node.h"

/**
 * @brief Find a key in a leaf node by linear scan.
 *
 * @param node  Leaf node to search.
 * @param key   Key to find.
 * @param klen  Length of key in bytes.
 * @return Index of the matching entry, or -1 if not found.
 */
garry_i32 garry_leaf_find(garry_btree_node *node, const garry_byte *key, garry_i32 klen)
{
    garry_i32 i;
    for (i = 0; i < node->entry_count; i++)
    {
        if (garry_byte_equal(node->keys[i], node->key_lens[i], key, klen))
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Find the child pointer to descend into for a given key.
 *
 * Scans internal node keys to locate the first entry greater than or
 * equal to the search key, returning its index as the child to follow.
 *
 * @param node  Internal node to search.
 * @param key   Search key.
 * @param klen  Length of key in bytes.
 * @return Child index to descend into.
 */
garry_i32 garry_internal_find(garry_btree_node *node, const garry_byte *key, garry_i32 klen)
{
    garry_i32 i;
    for (i = 0; i < node->entry_count; i++)
    {
        if (garry_byte_compare(key, klen, node->keys[i], node->key_lens[i]) < 0)
        {
            return i;
        }
    }
    return node->entry_count;
}

/**
 * @brief Search from root to leaf for a key and return its value.
 *
 * Descends the tree using garry_internal_find at each internal node,
 * then uses garry_leaf_find at the leaf to locate the exact key.
 *
 * @param pool       Buffer pool.
 * @param root       Root page ID of the B-tree.
 * @param key        Key to search for.
 * @param klen       Length of key in bytes.
 * @param result     Output buffer for the value data.
 * @param result_len Output location for the value length.
 * @return 1 if found, 0 if not found.
 */
garry_bool garry_leaf_find_search(garry_buffer_pool *pool, garry_i32 root, const garry_byte *key,
                                   garry_i32 klen, garry_byte *result, garry_i32 *result_len)
{
    garry_btree_node node;
    garry_i32 idx;
    garry_i32 page;
    garry_i32 child;

    page = root;
    for (;;)
    {
        garry_load_node(pool, page, &node);
        if (node.kind == GARRY_NODE_LEAF)
        {
            idx = garry_leaf_find(&node, key, klen);
            if (idx < 0)
                return 0;
            memcpy(result, node.values[idx], sizeof(garry_byte_array));
            *result_len = node.value_lens[idx];
            return 1;
        }
        if (node.kind != GARRY_NODE_INTERNAL)
            return 0;
        if (node.entry_count == 0)
            return 0;
        child = garry_internal_find(&node, key, klen);
        page = node.children[child];
    }
}
