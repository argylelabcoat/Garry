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

void garry_btree_cursor_value(const garry_btree_cursor_handle *cur, garry_byte_array *out)
{
    memcpy(*out, cur->current_value, sizeof(garry_byte_array));
}

void garry_btree_cursor_close(garry_btree_cursor_handle *cur)
{
    cur->exhausted = 1;
}
