/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_modify.c
 * @brief B-tree insert and delete with page splits and rebalancing.
 *
 * Implements top-down recursive insert/delete. Leaf splits promote a
 * separator key to the parent; internal splits propagate upward. Delete
 * rebalances by borrowing from siblings or merging underflowed nodes.
 * The root page ID is updated in-place when the tree grows or shrinks.
 */

#include "btree_modify.h"
#include "buffer_pool.h"
#include "btree_search.h"
#include "key_encoding.h"
#include "btree_compression.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Recursively insert a key-value pair into the B-tree.
 *
 * At a leaf, inserts or updates the entry directly, splitting if full.
 * At an internal node, recurses into the appropriate child and handles
 * upward split propagation by inserting a separator and new child pointer.
 *
 * @param pool      Buffer pool.
 * @param page      Current page ID being processed.
 * @param key       Key to insert.
 * @param klen      Length of key in bytes.
 * @param value     Value to insert.
 * @param vlen      Length of value in bytes.
 * @param sep       Output buffer for the separator key if a split occurred.
 * @param sep_len   Output location for the separator key length.
 * @param new_child Output location for the new child page ID from a split.
 * @return 1 if the key was inserted or updated, 0 on failure.
 */
static garry_bool btree_insert_rec(garry_buffer_pool *pool, garry_i32 page, const garry_byte *key,
                                    garry_i32 klen, const garry_byte *value, garry_i32 vlen,
                                    garry_byte *sep, garry_i32 *sep_len, garry_i32 *new_child);

static garry_bool leaf_split(garry_buffer_pool *pool, garry_i32 page, garry_btree_node *node,
                             const garry_byte *key, garry_i32 klen, const garry_byte *value,
                             garry_i32 vlen, garry_byte *sep, garry_i32 *sep_len,
                             garry_i32 *new_page);

static garry_bool internal_split(garry_buffer_pool *pool, garry_i32 page, garry_btree_node *node,
                                 garry_i32 insert_idx, const garry_byte *insert_key,
                                 garry_i32 insert_klen, garry_i32 insert_child, garry_byte *sep,
                                 garry_i32 *sep_len, garry_i32 *new_page);

static garry_bool rebalance_child(garry_buffer_pool *pool, garry_i32 parent_page,
                                  garry_btree_node *parent, garry_i32 child_idx);

/**
 * @brief Recursively delete a key from the B-tree.
 *
 * At a leaf, removes the matching entry. At an internal node, recurses
 * into the appropriate child and rebalances if the child underflows.
 *
 * @param pool  Buffer pool.
 * @param page  Current page ID being processed.
 * @param key   Key to delete.
 * @param klen  Length of key in bytes.
 * @return GARRY_TRUE if the current node underflowed after deletion.
 */
static garry_bool btree_delete_rec(garry_buffer_pool *pool, garry_i32 page, const garry_byte *key,
                                    garry_i32 klen);

/**
 * Insert a key-value pair into the B-tree rooted at *root.
 *
 * If the root splits, allocates a new root node and updates *root.
 * Returns true if the key was inserted or updated.
 */
garry_bool garry_btree_insert(garry_buffer_pool *pool, garry_i32 *root, const garry_byte *key,
                              garry_i32 klen, const garry_byte *value, garry_i32 vlen)
{
    garry_bool inserted;
    garry_byte sep[GARRY_MAX_RECORD_SIZE];
    garry_i32 sep_len;
    garry_i32 new_child;
    garry_btree_node new_root_node;
    garry_page_header hdr;
    garry_i32 new_root_page;
    garry_i32 orig_root_entries;
    garry_i32 i;

    new_child = -1;
    sep_len = 0;
    garry_load_node(pool, *root, &new_root_node);
    orig_root_entries = new_root_node.entry_count;
    inserted = btree_insert_rec(pool, *root, key, klen, value, vlen, sep, &sep_len, &new_child);
    if (new_child >= 0)
    {
        garry_load_node(pool, *root, &new_root_node);
        if (new_root_node.entry_count <= orig_root_entries)
        {
            new_root_page = garry_allocate_internal(pool);
            if (new_root_page < 0)
            {
                return 0;
            }
            new_root_node.kind = GARRY_NODE_INTERNAL;
            hdr.page_type = GARRY_NODE_INTERNAL;
            hdr.page_level = 0;
            new_root_node.header = hdr;
            new_root_node.children[0] = *root;
            memcpy(new_root_node.keys[0], sep, sizeof(garry_byte_array));
            new_root_node.key_lens[0] = sep_len;
            new_root_node.children[1] = new_child;
            new_root_node.entry_count = 1;
            new_root_node.next_page = -1;
            new_root_node.prev_page = -1;
            for (i = 2; i < GARRY_MAX_KEYS_PER_NODE + 1; i++)
            {
                new_root_node.children[i] = 0;
            }
            garry_store_node(pool, new_root_page, &new_root_node);
            garry_pool_mark_dirty(pool, new_root_page);
            *root = new_root_page;
        }
    }
    return inserted;
}

static garry_bool btree_insert_rec(garry_buffer_pool *pool, garry_i32 page, const garry_byte *key,
                                   garry_i32 klen, const garry_byte *value, garry_i32 vlen,
                                   garry_byte *sep, garry_i32 *sep_len, garry_i32 *new_child)
{
    garry_btree_node node;
    garry_i32 idx;
    garry_i32 child;
    garry_bool inserted;
    garry_byte child_sep[GARRY_MAX_RECORD_SIZE];
    garry_i32 child_sep_len;
    garry_i32 child_new;
    garry_i32 i;

    *new_child = -1;
    *sep_len = 0;
    garry_load_node(pool, page, &node);

    if (node.kind == GARRY_NODE_LEAF)
    {
        idx = garry_leaf_find(&node, key, klen);
        if (idx >= 0)
        {
            memcpy(node.keys[idx], key, sizeof(garry_byte_array));
            node.key_lens[idx] = klen;
            memcpy(node.values[idx], value, sizeof(garry_byte_array));
            node.value_lens[idx] = vlen;
            garry_store_node(pool, page, &node);
            garry_pool_mark_dirty(pool, page);
            return 1;
        }
        if (node.entry_count < GARRY_MAX_KEYS_PER_NODE - 1)
        {
            i = node.entry_count - 1;
            idx = node.entry_count;
            while (i >= 0 && idx == node.entry_count)
            {
                if (garry_byte_compare(key, klen, node.keys[i], node.key_lens[i]) < 0)
                {
                    idx = i;
                }
                i = i - 1;
            }
            inserted = garry_leaf_insert(&node, key, klen, value, vlen, idx);
            garry_store_node(pool, page, &node);
            garry_pool_mark_dirty(pool, page);
            return inserted;
        }
        inserted = leaf_split(pool, page, &node, key, klen, value, vlen, sep, sep_len, new_child);
        garry_store_node(pool, page, &node);
        return inserted;
    }
    else
    {
        idx = garry_internal_find(&node, key, klen);
        child = node.children[idx];
        child_sep_len = 0;
        child_new = -1;
        inserted = btree_insert_rec(pool, child, key, klen, value, vlen, child_sep, &child_sep_len,
                                    &child_new);
        if (inserted && child_new >= 0)
        {
            if (node.entry_count < GARRY_MAX_KEYS_PER_NODE)
            {
                i = node.entry_count;
                while (i > idx)
                {
                    memcpy(node.keys[i], node.keys[i - 1], sizeof(garry_byte_array));
                    node.key_lens[i] = node.key_lens[i - 1];
                    node.children[i + 1] = node.children[i];
                    i = i - 1;
                }
                memcpy(node.keys[idx], child_sep, sizeof(garry_byte_array));
                node.key_lens[idx] = child_sep_len;
                node.children[idx + 1] = child_new;
                node.entry_count = node.entry_count + 1;
                garry_store_node(pool, page, &node);
                garry_pool_mark_dirty(pool, page);
            }
            else
            {
                if (!internal_split(pool, page, &node, idx, child_sep, child_sep_len, child_new,
                                    sep, sep_len, new_child))
                {
                    return 0;
                }
            }
        }
        return inserted;
    }
}

/**
 * Split a full leaf node.
 *
 * Copies all entries plus the new key into a temporary array, finds the
 * median, and distributes entries between the original node (left half)
 * and a newly allocated right node (right half). The first key of the
 * right node becomes the separator promoted to the parent.
 */
static garry_bool leaf_split(garry_buffer_pool *pool, garry_i32 page, garry_btree_node *node,
                             const garry_byte *key, garry_i32 klen, const garry_byte *value,
                             garry_i32 vlen, garry_byte *sep, garry_i32 *sep_len,
                             garry_i32 *new_page)
{
    garry_i32 mid, right_page, i, insert_pos, all_count;
    garry_btree_node right_node;
    garry_byte_array *tmp_keys;
    garry_i32 *tmp_lens;
    garry_byte_array *tmp_values;
    garry_i32 *tmp_vlens;
    char *tmp_buf;
    garry_i32 n = GARRY_MAX_KEYS_PER_NODE + 1;

    tmp_buf = malloc(sizeof(garry_byte_array) * n + sizeof(garry_i32) * n +
                     sizeof(garry_byte_array) * n + sizeof(garry_i32) * n);
    if (!tmp_buf)
        return 0;

    tmp_keys = (garry_byte_array *)tmp_buf;
    tmp_lens = (garry_i32 *)(tmp_buf + sizeof(garry_byte_array) * n);
    tmp_values =
        (garry_byte_array *)(tmp_buf + sizeof(garry_byte_array) * n + sizeof(garry_i32) * n);
    tmp_vlens = (garry_i32 *)(tmp_buf + sizeof(garry_byte_array) * n + sizeof(garry_i32) * n +
                              sizeof(garry_byte_array) * n);

    all_count = node->entry_count + 1;
    mid = all_count / 2;
    insert_pos = node->entry_count;
    for (i = 0; i < node->entry_count; i++)
    {
        if (insert_pos == node->entry_count)
        {
            if (garry_byte_compare(key, klen, node->keys[i], node->key_lens[i]) < 0)
            {
                insert_pos = i;
            }
        }
    }
    for (i = 0; i < insert_pos; i++)
    {
        memcpy(tmp_keys[i], node->keys[i], sizeof(garry_byte_array));
        tmp_lens[i] = node->key_lens[i];
        memcpy(tmp_values[i], node->values[i], sizeof(garry_byte_array));
        tmp_vlens[i] = node->value_lens[i];
    }
    memcpy(tmp_keys[insert_pos], key, sizeof(garry_byte_array));
    tmp_lens[insert_pos] = klen;
    memcpy(tmp_values[insert_pos], value, sizeof(garry_byte_array));
    tmp_vlens[insert_pos] = vlen;
    for (i = insert_pos; i < node->entry_count; i++)
    {
        memcpy(tmp_keys[i + 1], node->keys[i], sizeof(garry_byte_array));
        tmp_lens[i + 1] = node->key_lens[i];
        memcpy(tmp_values[i + 1], node->values[i], sizeof(garry_byte_array));
        tmp_vlens[i + 1] = node->value_lens[i];
    }

    right_page = garry_allocate_leaf(pool);
    if (right_page < 0)
    {
        free(tmp_buf);
        return 0;
    }
    garry_load_node(pool, right_page, &right_node);
    right_node.kind = GARRY_NODE_LEAF;
    right_node.entry_count = all_count - mid;
    for (i = 0; i < right_node.entry_count; i++)
    {
        memcpy(right_node.keys[i], tmp_keys[mid + i], sizeof(garry_byte_array));
        right_node.key_lens[i] = tmp_lens[mid + i];
        memcpy(right_node.values[i], tmp_values[mid + i], sizeof(garry_byte_array));
        right_node.value_lens[i] = tmp_vlens[mid + i];
    }

    node->entry_count = mid;
    for (i = 0; i < node->entry_count; i++)
    {
        memcpy(node->keys[i], tmp_keys[i], sizeof(garry_byte_array));
        node->key_lens[i] = tmp_lens[i];
        memcpy(node->values[i], tmp_values[i], sizeof(garry_byte_array));
        node->value_lens[i] = tmp_vlens[i];
    }

    right_node.next_page = node->next_page;
    node->next_page = right_page;
    right_node.prev_page = page;

    memcpy(sep, right_node.keys[0], sizeof(garry_byte_array));
    *sep_len = right_node.key_lens[0];

    garry_pool_mark_dirty(pool, page);
    garry_store_node(pool, right_page, &right_node);
    garry_store_node(pool, page, node);
    *new_page = right_page;

    free(tmp_buf);
    return 1;
}

/**
 * Split a full internal node.
 *
 * Inserts the new key/child into a temporary copy at the correct position,
 * splits at the median, and promotes the median key to the parent. The
 * left node keeps keys[0..mid-1], the right node gets keys[mid+1..end].
 * Child pointers are split accordingly.
 */
static garry_bool internal_split(garry_buffer_pool *pool, garry_i32 page, garry_btree_node *node,
                                 garry_i32 insert_idx, const garry_byte *insert_key,
                                 garry_i32 insert_klen, garry_i32 insert_child, garry_byte *sep,
                                 garry_i32 *sep_len, garry_i32 *new_page)
{
    garry_byte_array *tmp_keys;
    garry_i32 *tmp_lens;
    garry_i32 *tmp_children;
    garry_i32 right_page, left_count, right_count, all_count, mid, i;
    garry_btree_node right_node;
    char *tmp_buf;
    garry_i32 n = GARRY_MAX_KEYS_PER_NODE + 1;

    tmp_buf =
        malloc(sizeof(garry_byte_array) * n + sizeof(garry_i32) * n + sizeof(garry_i32) * (n + 1));
    if (!tmp_buf)
        return 0;

    tmp_keys = (garry_byte_array *)tmp_buf;
    tmp_lens = (garry_i32 *)(tmp_buf + sizeof(garry_byte_array) * n);
    tmp_children = (garry_i32 *)(tmp_buf + sizeof(garry_byte_array) * n + sizeof(garry_i32) * n);

    all_count = node->entry_count + 1;
    mid = all_count / 2;

    for (i = 0; i < insert_idx; i++)
    {
        memcpy(tmp_keys[i], node->keys[i], sizeof(garry_byte_array));
        tmp_lens[i] = node->key_lens[i];
        tmp_children[i] = node->children[i];
    }
    memcpy(tmp_keys[insert_idx], insert_key, sizeof(garry_byte_array));
    tmp_lens[insert_idx] = insert_klen;
    tmp_children[insert_idx] = node->children[insert_idx];
    tmp_children[insert_idx + 1] = insert_child;
    for (i = insert_idx + 1; i <= node->entry_count; i++)
    {
        memcpy(tmp_keys[i], node->keys[i - 1], sizeof(garry_byte_array));
        tmp_lens[i] = node->key_lens[i - 1];
        tmp_children[i + 1] = node->children[i];
    }

    right_page = garry_allocate_internal(pool);
    if (right_page < 0)
    {
        free(tmp_buf);
        return 0;
    }
    garry_load_node(pool, right_page, &right_node);
    right_node.kind = GARRY_NODE_INTERNAL;
    right_node.header.page_type = GARRY_NODE_INTERNAL;
    right_node.header.page_level = node->header.page_level;

    left_count = mid;
    right_count = all_count - mid - 1;
    right_node.entry_count = right_count;
    right_node.next_page = node->next_page;
    right_node.prev_page = page;

    for (i = 0; i < right_node.entry_count; i++)
    {
        memcpy(right_node.keys[i], tmp_keys[mid + 1 + i], sizeof(garry_byte_array));
        right_node.key_lens[i] = tmp_lens[mid + 1 + i];
    }
    for (i = 0; i <= right_node.entry_count; i++)
    {
        right_node.children[i] = tmp_children[mid + 1 + i];
    }

    node->entry_count = left_count;
    for (i = 0; i < node->entry_count; i++)
    {
        memcpy(node->keys[i], tmp_keys[i], sizeof(garry_byte_array));
        node->key_lens[i] = tmp_lens[i];
    }
    for (i = 0; i <= node->entry_count; i++)
    {
        node->children[i] = tmp_children[i];
    }

    node->next_page = right_page;
    memcpy(sep, tmp_keys[mid], sizeof(garry_byte_array));
    *sep_len = tmp_lens[mid];

    garry_store_node(pool, right_page, &right_node);
    garry_store_node(pool, page, node);
    garry_pool_mark_dirty(pool, right_page);
    garry_pool_mark_dirty(pool, page);
    *new_page = right_page;

    free(tmp_buf);
    return 1;
}

/**
 * Rebalance a child node that underflowed after deletion.
 *
 * Tries to merge the underflowed child with a sibling. If a left sibling
 * exists, merges into it (borrowing the parent separator). Otherwise
 * merges with the right sibling into the current child. If the merged
 * result exceeds capacity, does nothing (child stays underflowed).
 * Removes the separator key from the parent after a successful merge.
 */
static garry_bool rebalance_child(garry_buffer_pool *pool, garry_i32 parent_page,
                                  garry_btree_node *parent, garry_i32 child_idx)
{
    garry_i32 left_page, right_page, child_page;
    garry_btree_node left_node;
    garry_btree_node tmp;
    garry_i32 i, base;

    child_page = parent->children[child_idx];
    if (child_idx > 0)
    {
        left_page = parent->children[child_idx - 1];
        garry_load_node(pool, left_page, &left_node);
        garry_load_node(pool, child_page, &tmp);
        {
            garry_i32 merged_keys = left_node.entry_count + tmp.entry_count;
            if (left_node.kind == GARRY_NODE_INTERNAL)
            {
                merged_keys = merged_keys + 1;
            }
            if (merged_keys > GARRY_MAX_KEYS_PER_NODE)
            {
                return GARRY_FALSE;
            }
        }
        if (left_node.kind == GARRY_NODE_INTERNAL)
        {
            memcpy(left_node.keys[left_node.entry_count], parent->keys[child_idx - 1],
                   sizeof(garry_byte_array));
            left_node.key_lens[left_node.entry_count] = parent->key_lens[child_idx - 1];
            left_node.entry_count = left_node.entry_count + 1;
        }
        base = left_node.entry_count;
        for (i = 0; i < tmp.entry_count; i++)
        {
            memcpy(left_node.keys[base + i], tmp.keys[i], sizeof(garry_byte_array));
            left_node.key_lens[base + i] = tmp.key_lens[i];
            memcpy(left_node.values[base + i], tmp.values[i], sizeof(garry_byte_array));
            left_node.value_lens[base + i] = tmp.value_lens[i];
        }
        if (tmp.kind == GARRY_NODE_INTERNAL)
        {
            for (i = 0; i <= tmp.entry_count; i++)
            {
                left_node.children[base + i] = tmp.children[i];
            }
        }
        left_node.entry_count = left_node.entry_count + tmp.entry_count;
        left_node.next_page = tmp.next_page;
        garry_store_node(pool, left_page, &left_node);
        garry_pool_mark_dirty(pool, left_page);

        i = child_idx - 1;
        while (i < parent->entry_count - 1)
        {
            memcpy(parent->keys[i], parent->keys[i + 1], sizeof(garry_byte_array));
            parent->key_lens[i] = parent->key_lens[i + 1];
            parent->children[i + 1] = parent->children[i + 2];
            i = i + 1;
        }
        parent->entry_count = parent->entry_count - 1;
        garry_store_node(pool, parent_page, parent);
        garry_pool_mark_dirty(pool, parent_page);
    }
    else
    {
        right_page = parent->children[child_idx + 1];
        garry_load_node(pool, child_page, &tmp);
        {
            garry_btree_node right_node;
            garry_load_node(pool, right_page, &right_node);
            {
                garry_i32 merged_keys = tmp.entry_count + right_node.entry_count;
                if (tmp.kind == GARRY_NODE_INTERNAL)
                {
                    merged_keys = merged_keys + 1;
                }
                if (merged_keys > GARRY_MAX_KEYS_PER_NODE)
                {
                    return GARRY_FALSE;
                }
            }
            if (tmp.kind == GARRY_NODE_INTERNAL)
            {
                memcpy(tmp.keys[tmp.entry_count], parent->keys[child_idx],
                       sizeof(garry_byte_array));
                tmp.key_lens[tmp.entry_count] = parent->key_lens[child_idx];
                tmp.entry_count = tmp.entry_count + 1;
            }
            base = tmp.entry_count;
            for (i = 0; i < right_node.entry_count; i++)
            {
                memcpy(tmp.keys[base + i], right_node.keys[i], sizeof(garry_byte_array));
                tmp.key_lens[base + i] = right_node.key_lens[i];
                memcpy(tmp.values[base + i], right_node.values[i], sizeof(garry_byte_array));
                tmp.value_lens[base + i] = right_node.value_lens[i];
            }
            if (right_node.kind == GARRY_NODE_INTERNAL)
            {
                for (i = 0; i <= right_node.entry_count; i++)
                {
                    tmp.children[base + i] = right_node.children[i];
                }
            }
            tmp.entry_count = tmp.entry_count + right_node.entry_count;
            tmp.next_page = right_node.next_page;
        }
        garry_store_node(pool, child_page, &tmp);
        garry_pool_mark_dirty(pool, child_page);

        i = child_idx;
        while (i < parent->entry_count - 1)
        {
            memcpy(parent->keys[i], parent->keys[i + 1], sizeof(garry_byte_array));
            parent->key_lens[i] = parent->key_lens[i + 1];
            parent->children[i + 1] = parent->children[i + 2];
            i = i + 1;
        }
        parent->entry_count = parent->entry_count - 1;
        garry_store_node(pool, parent_page, parent);
        garry_pool_mark_dirty(pool, parent_page);
    }
    return GARRY_TRUE;
}

static garry_bool btree_delete_rec(garry_buffer_pool *pool, garry_i32 page, const garry_byte *key,
                                   garry_i32 klen)
{
    garry_btree_node node;
    garry_i32 idx, i, child;
    garry_bool underflowed;

    garry_load_node(pool, page, &node);
    if (node.kind == GARRY_NODE_LEAF)
    {
        idx = garry_leaf_find(&node, key, klen);
        if (idx >= 0)
        {
            i = idx;
            while (i < node.entry_count - 1)
            {
                memcpy(node.keys[i], node.keys[i + 1], sizeof(garry_byte_array));
                node.key_lens[i] = node.key_lens[i + 1];
                memcpy(node.values[i], node.values[i + 1], sizeof(garry_byte_array));
                node.value_lens[i] = node.value_lens[i + 1];
                i = i + 1;
            }
            node.entry_count = node.entry_count - 1;
            garry_store_node(pool, page, &node);
            garry_pool_mark_dirty(pool, page);
        }
        return (node.entry_count < GARRY_BTREE_MIN_KEYS);
    }
    else
    {
        idx = garry_internal_find(&node, key, klen);
        child = node.children[idx];
        underflowed = btree_delete_rec(pool, child, key, klen);
        if (underflowed)
        {
            rebalance_child(pool, page, &node, idx);
        }
        return (node.entry_count < GARRY_BTREE_MIN_KEYS);
    }
}

/**
 * @brief Delete a key from the B-tree and shrink the root if needed.
 *
 * Performs a recursive delete. If the root becomes an empty internal
 * node, it is freed and the root pointer is updated to its only child.
 *
 * @param pool  Buffer pool.
 * @param root  Pointer to the root page ID (updated if root shrinks).
 * @param key   Key to delete.
 * @param klen  Length of key in bytes.
 */
void garry_btree_delete(garry_buffer_pool *pool, garry_i32 *root, const garry_byte *key,
                         garry_i32 klen)
{
    garry_bool underflowed;
    garry_btree_node rnode;

    underflowed = btree_delete_rec(pool, *root, key, klen);
    if (underflowed)
    {
        garry_load_node(pool, *root, &rnode);
        if (rnode.kind == GARRY_NODE_INTERNAL && rnode.entry_count == 0)
        {
            garry_pool_free_page(pool, *root);
            *root = rnode.children[0];
        }
    }
}
