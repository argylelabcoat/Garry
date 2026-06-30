/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_cursor.c
 * @brief Prefix-filtered cursor for sequential B-tree traversal.
 *
 * Maintains a stack of (page_id, slot_index) pairs representing the
 * current position. next() advances to the next leaf slot, crossing
 * page boundaries as needed, and skips keys that don't match the
 * configured prefix.
 */

#include "btree_cursor.h"
#include "btree_search.h"
#include <string.h>

static garry_bool prefix_match(const garry_byte *key, garry_i32 klen, const garry_byte *prefix,
                               garry_i32 plen)
{
    garry_i32 i;
    if (plen > klen)
        return 0;
    for (i = 0; i < plen; i++)
    {
        if (key[i] != prefix[i])
            return 0;
    }
    return 1;
}

/**
 * @brief Open a B-tree cursor at the first matching leaf position.
 *
 * Traverses the B-tree from the root to the leftmost leaf that could
 * contain keys matching the prefix. Positions the cursor at slot 0.
 *
 * @param pool   Buffer pool for page access.
 * @param root   Root page ID of the B-tree.
 * @param prefix Key prefix to filter results (NULL for no filter).
 * @param plen   Length of the prefix in bytes.
 *
 * @return Initialized cursor handle.
 */
garry_btree_cursor_handle garry_btree_cursor_open(garry_buffer_pool *pool, garry_i32 root,
                                                  const garry_byte_array *prefix, garry_u32 plen)
{
    garry_btree_cursor_handle cur;
    garry_btree_node node;
    garry_i32 page;
    garry_i32 idx;

    memset(&cur, 0, sizeof(cur));
    cur.root = root;
    if (prefix != NULL)
    {
        memcpy(cur.prefix, *prefix, sizeof(garry_byte_array));
    }
    else
    {
        memset(cur.prefix, 0, sizeof(garry_byte_array));
    }
    cur.prefix_len = plen;
    cur.exhausted = 0;
    cur.has_saved = 0;

    page = root;
    for (;;)
    {
        garry_load_node(pool, page, &node);
        if (node.kind == GARRY_NODE_LEAF)
        {
            cur.current_page = page;
            cur.current_slot = 0;
            return cur;
        }
        if (node.entry_count > 0)
        {
            if (plen > 0 && prefix != NULL)
            {
                idx = garry_internal_find(&node, (const garry_byte *)*prefix, (garry_i32)plen);
                if (idx >= node.entry_count)
                {
                    idx = node.entry_count - 1;
                }
            }
            else
            {
                idx = 0;
            }
            page = node.children[idx];
        }
        else
        {
            cur.exhausted = 1;
            return cur;
        }
    }
}

/**
 * @brief Advance the cursor to the next matching key.
 *
 * Skips keys that do not match the configured prefix and crosses
 * leaf page boundaries via next_page links.
 *
 * @param pool Buffer pool for page access.
 * @param cur  Cursor handle to advance.
 * @param key  Output buffer for the next key.
 * @param klen Output parameter receiving the key length.
 *
 * @return GARRY_TRUE if a key was found, GARRY_FALSE if exhausted.
 */
garry_bool garry_btree_cursor_next(garry_buffer_pool *pool, garry_btree_cursor_handle *cur,
                                   garry_byte_array *key, garry_u32 *klen)
{
    garry_btree_node node;
    garry_i32 i;
    garry_bool match;

    if (cur->exhausted)
        return 0;

    if (cur->has_saved)
    {
        memcpy(*key, cur->saved_key, sizeof(garry_byte_array));
        *klen = cur->saved_klen;
        memcpy(cur->current, cur->saved_key, sizeof(garry_byte_array));
        cur->current_len = cur->saved_klen;
        cur->has_saved = 0;
        cur->saved_klen = 0;
        return 1;
    }

    for (;;)
    {
        garry_load_node(pool, cur->current_page, &node);
        memcpy(&cur->current_node, &node, sizeof(garry_btree_node));
        cur->node_loaded = 1;
        for (i = cur->current_slot; i < node.entry_count; i++)
        {
            match = 0;
            if (cur->prefix_len > 0)
            {
                match = prefix_match(node.keys[i], node.key_lens[i], cur->prefix,
                                     (garry_i32)cur->prefix_len);
            }
            if (match || cur->prefix_len == 0)
            {
                memcpy(*key, node.keys[i], sizeof(garry_byte_array));
                memcpy(cur->current, node.keys[i], sizeof(garry_byte_array));
                cur->current_len = node.key_lens[i];
                memcpy(cur->current_value, node.values[i], sizeof(garry_byte_array));
                cur->current_slot = i + 1;
                *klen = node.key_lens[i];
                return 1;
            }
        }
        if (node.next_page >= 0)
        {
            cur->current_page = node.next_page;
            cur->current_slot = 0;
            cur->node_loaded = 0;
        }
        else
        {
            cur->exhausted = 1;
            cur->node_loaded = 0;
            return 0;
        }
    }
}

/**
 * @brief Peek at the current cursor position without advancing.
 *
 * Returns the key at the current position. If no key is cached,
 * loads the current leaf page to read it.
 *
 * @param pool Buffer pool for page access.
 * @param cur  Cursor handle to peek at.
 * @param key  Output buffer for the key.
 * @param klen Output parameter receiving the key length.
 *
 * @return GARRY_TRUE if a key is available, GARRY_FALSE otherwise.
 */
garry_bool garry_btree_cursor_peek(garry_buffer_pool *pool, const garry_btree_cursor_handle *cur,
                                   garry_byte_array *key, garry_u32 *klen)
{
    garry_btree_node node;

    if (cur->exhausted)
        return 0;
    if (cur->current_len > 0)
    {
        memcpy(*key, cur->current, sizeof(garry_byte_array));
        *klen = cur->current_len;
        return 1;
    }
    garry_load_node(pool, cur->current_page, &node);
    if (cur->current_slot < (garry_u32)node.entry_count)
    {
        memcpy(*key, node.keys[cur->current_slot], sizeof(garry_byte_array));
        *klen = node.key_lens[cur->current_slot];
        return 1;
    }
    return 0;
}

/**
 * @brief Get the value associated with the current cursor position.
 *
 * Copies the value from the current position into the output buffer.
 * Must be called after a successful next() or peek().
 *
 * @param cur Cursor handle to read from.
 * @param out Output buffer for the value.
 */
void garry_btree_cursor_value(const garry_btree_cursor_handle *cur, garry_byte_array *out)
{
    memcpy(*out, cur->current_value, sizeof(garry_byte_array));
}

/**
 * @brief Close a B-tree cursor.
 *
 * Marks the cursor as exhausted so no further iteration is possible.
 *
 * @param cur Cursor handle to close.
 */
void garry_btree_cursor_close(garry_btree_cursor_handle *cur)
{
    cur->exhausted = 1;
}
