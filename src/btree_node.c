/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_node.c
 * @brief B-tree node allocation, serialization, and leaf manipulation.
 *
 * Handles CBOR encoding/decoding of node metadata, allocation of new
 * leaf and internal pages from the buffer pool, and leaf-level insert
 * operations. Node keys use prefix compression relative to predecessors.
 */

#include "btree_node.h"
#include "slotted_page.h"
#include "record_codec.h"
#include "db_header.h"
#include <string.h>

static garry_i32 encode_cbor_uint(garry_byte* buf, garry_i32 pos, garry_i32 val)
{
    if (val < 0) return pos;
    if (val < 24) {
        buf[pos] = (garry_byte)val;
        return pos + 1;
    } else if (val < 256) {
        buf[pos] = (garry_byte)24;
        buf[pos + 1] = (garry_byte)val;
        return pos + 2;
    } else if (val < 65536) {
        buf[pos] = (garry_byte)25;
        buf[pos + 1] = (garry_byte)((val / 256) % 256);
        buf[pos + 2] = (garry_byte)(val % 256);
        return pos + 3;
    } else {
        buf[pos] = (garry_byte)26;
        buf[pos + 1] = (garry_byte)((val / 16777216) % 256);
        buf[pos + 2] = (garry_byte)((val / 65536) % 256);
        buf[pos + 3] = (garry_byte)((val / 256) % 256);
        buf[pos + 4] = (garry_byte)(val % 256);
        return pos + 5;
    }
}

static garry_i32 decode_cbor_uint(const garry_byte* buf, garry_i32 pos, garry_i32 elen, garry_i32* val)
{
    garry_i32 fb, b1, b2, b3, b4;
    if (pos >= elen) return pos;
    fb = (garry_i32)buf[pos];
    if (fb < 0) fb += 256;
    if (fb < 24) {
        *val = fb;
        return pos + 1;
    } else if (fb == 24) {
        if (pos + 1 >= elen) return pos + 1;
        b1 = (garry_i32)buf[pos + 1];
        if (b1 < 0) b1 += 256;
        *val = b1;
        return pos + 2;
    } else if (fb == 25) {
        if (pos + 2 >= elen) return pos + 1;
        b1 = (garry_i32)buf[pos + 1];
        b2 = (garry_i32)buf[pos + 2];
        if (b1 < 0) b1 += 256;
        if (b2 < 0) b2 += 256;
        *val = b1 * 256 + b2;
        return pos + 3;
    } else if (fb == 26) {
        if (pos + 4 >= elen) return pos + 1;
        b1 = (garry_i32)buf[pos + 1];
        b2 = (garry_i32)buf[pos + 2];
        b3 = (garry_i32)buf[pos + 3];
        b4 = (garry_i32)buf[pos + 4];
        if (b1 < 0) b1 += 256;
        if (b2 < 0) b2 += 256;
        if (b3 < 0) b3 += 256;
        if (b4 < 0) b4 += 256;
        *val = b1 * 16777216 + b2 * 65536 + b3 * 256 + b4;
        return pos + 5;
    }
    return pos + 1;
}

garry_i32 garry_encode_node_meta(garry_btree_node* node, garry_byte* buf)
{
    garry_i32 pos, num_children, i, meta_count;
    if (node->kind == GARRY_NODE_INTERNAL) {
        num_children = node->entry_count + 1;
    } else {
        num_children = 0;
    }
    meta_count = 2 + num_children;
    pos = 0;
    if (meta_count < 24) {
        buf[pos] = (garry_byte)(128 + meta_count);
        pos = pos + 1;
    } else {
        buf[pos] = (garry_byte)(128 + 24);
        pos = pos + 1;
        buf[pos] = (garry_byte)meta_count;
        pos = pos + 1;
    }
    pos = encode_cbor_uint(buf, pos, node->next_page + GARRY_PAGE_ID_BIAS);
    pos = encode_cbor_uint(buf, pos, node->prev_page + GARRY_PAGE_ID_BIAS);
    for (i = 0; i < num_children; i++) {
        pos = encode_cbor_uint(buf, pos, node->children[i] + GARRY_PAGE_ID_BIAS);
    }
    return pos;
}

void garry_decode_node_meta(const garry_byte* buf, garry_i32 elen, garry_btree_node* node)
{
    garry_i32 pos, arr_count, fb, ai, decoded, num_children, i;
    pos = 0;
    fb = (garry_i32)buf[pos];
    if (fb < 0) fb += 256;
    if (fb < 128 || fb >= 160) {
        return;
    }
    ai = fb % 32;
    if (ai < 24) {
        arr_count = ai;
        pos = pos + 1;
    } else if (ai == 24 && pos + 1 < elen) {
        arr_count = (garry_i32)buf[pos + 1];
        if (arr_count < 0) arr_count += 256;
        pos = pos + 2;
    } else {
        return;
    }
    pos = decode_cbor_uint(buf, pos, elen, &decoded);
    node->next_page = decoded - GARRY_PAGE_ID_BIAS;
    pos = decode_cbor_uint(buf, pos, elen, &decoded);
    node->prev_page = decoded - GARRY_PAGE_ID_BIAS;
    if (node->kind == GARRY_NODE_INTERNAL) {
        num_children = arr_count - 2;
    } else {
        num_children = 0;
    }
    for (i = 0; i < num_children; i++) {
        if (pos < elen) {
            pos = decode_cbor_uint(buf, pos, elen, &decoded);
            node->children[i] = decoded - GARRY_PAGE_ID_BIAS;
        }
    }
}

garry_bool garry_leaf_insert(garry_btree_node* node, const garry_byte* key, garry_i32 klen,
                             const garry_byte* value, garry_i32 vlen, garry_i32 idx)
{
    garry_i32 i;
    if (node->entry_count >= GARRY_MAX_KEYS_PER_NODE) {
        return 0;
    }
    i = node->entry_count;
    while (i > idx) {
        memcpy(node->keys[i], node->keys[i - 1], sizeof(garry_byte_array));
        node->key_lens[i] = node->key_lens[i - 1];
        memcpy(node->values[i], node->values[i - 1], sizeof(garry_byte_array));
        node->value_lens[i] = node->value_lens[i - 1];
        i = i - 1;
    }
    memcpy(node->keys[idx], key, sizeof(garry_byte_array));
    node->key_lens[idx] = klen;
    memcpy(node->values[idx], value, sizeof(garry_byte_array));
    node->value_lens[idx] = vlen;
    node->entry_count = node->entry_count + 1;
    return 1;
}

garry_i32 garry_allocate_leaf(garry_buffer_pool *pool)
{
    garry_i32 pid;
    garry_page_buffer *buf;
    pid = garry_pool_allocate(pool);
    if (pid < 0) return -1;
    buf = garry_pool_pin_page(pool, pid);
    if (!buf) return -1;
    garry_page_init(*buf, GARRY_NODE_LEAF, 0, pool->page_size);
    garry_pool_mark_dirty(pool, pid);
    garry_pool_release_page(pool, pid);
    return pid;
}

garry_i32 garry_allocate_internal(garry_buffer_pool *pool)
{
    garry_i32 pid;
    garry_page_buffer *buf;
    pid = garry_pool_allocate(pool);
    if (pid < 0) return -1;
    buf = garry_pool_pin_page(pool, pid);
    if (!buf) return -1;
    garry_page_init(*buf, GARRY_NODE_INTERNAL, 0, pool->page_size);
    garry_pool_mark_dirty(pool, pid);
    garry_pool_release_page(pool, pid);
    return pid;
}

void garry_load_node(garry_buffer_pool *pool, garry_i32 pid, garry_btree_node *node)
{
    garry_page_buffer *buf;
    garry_i32 rec_count, ptype_val, level_val;
    garry_byte rec_data[GARRY_MAX_RECORD_SIZE];
    garry_byte rec_buf[GARRY_MAX_RECORD_SIZE];
    garry_i32 rlen, j, k, entry_idx, vlen_out;
    garry_byte ckey[GARRY_MAX_KEY_SIZE];
    garry_i32 ckey_len;

    buf = garry_pool_try_pin(pool, pid);
    if (!buf) {
        buf = garry_pool_pin_page(pool, pid);
    }
    if (!buf) {
        return;
    }

    rec_count = garry_page_record_count(buf);
    ptype_val = garry_read_int32((garry_byte*)*buf, 0);
    level_val = garry_read_int32((garry_byte*)*buf, 4);

    if (ptype_val == GARRY_NODE_LEAF) {
        node->kind = GARRY_NODE_LEAF;
    } else if (ptype_val == GARRY_NODE_OVERFLOW) {
        node->kind = GARRY_NODE_OVERFLOW;
    } else {
        node->kind = GARRY_NODE_INTERNAL;
    }

    node->header.page_type = node->kind;
    node->header.page_level = level_val;
    node->next_page = -1;
    node->prev_page = -1;
    node->entry_count = 0;

    if (rec_count > 0) {
        rlen = garry_page_get(buf, GARRY_META_SLOT, rec_data, pool->page_size);
        if (rlen > 0) {
            for (k = 0; k < rlen; k++) {
                rec_buf[k] = rec_data[k];
            }
            garry_decode_node_meta(rec_buf, rlen, node);
        }
    }

    if (rec_count > 1) {
        entry_idx = 0;
        for (j = 1; j < rec_count; j++) {
            if (entry_idx < GARRY_MAX_KEYS_PER_NODE) {
                rlen = garry_page_get(buf, j, rec_data, pool->page_size);
                if (rlen > 0) {
                    for (k = 0; k < rlen; k++) {
                        rec_buf[k] = rec_data[k];
                    }
                    garry_decode_kv(rec_buf, rlen, ckey, &ckey_len,
                                    node->values[entry_idx], &vlen_out);
                    memcpy(node->keys[entry_idx], ckey, ckey_len);
                    node->key_lens[entry_idx] = ckey_len;
                    node->value_lens[entry_idx] = vlen_out;
                    entry_idx++;
                }
            }
        }
        node->entry_count = entry_idx;
    }

    garry_pool_release_page(pool, pid);
}

void garry_store_node(garry_buffer_pool *pool, garry_i32 pid, garry_btree_node *node)
{
    garry_page_buffer *buf;
    garry_byte rec_buf[GARRY_MAX_RECORD_SIZE];
    garry_byte enc_data[GARRY_MAX_RECORD_SIZE];
    garry_i32 rec_len, slot_idx, i, j;
    garry_byte meta_data[GARRY_MAX_RECORD_SIZE];
    garry_i32 meta_len;

    buf = garry_pool_pin_page(pool, pid);
    if (!buf) {
        return;
    }

    garry_page_init(*buf, node->kind, node->header.page_level, pool->page_size);
    if (node->entry_count > GARRY_MAX_KEYS_PER_NODE) {
        node->entry_count = GARRY_MAX_KEYS_PER_NODE;
    }
    meta_len = garry_encode_node_meta(node, rec_buf);
    for (j = 0; j < meta_len; j++) {
        meta_data[j] = rec_buf[j];
    }
    slot_idx = garry_page_insert(*buf, meta_data, meta_len, pool->page_size);
    if (slot_idx < 0) {
        garry_pool_release_page(pool, pid);
        return;
    }

    for (i = 0; i < node->entry_count; i++) {
        if (node->kind == GARRY_NODE_INTERNAL) {
            rec_len = garry_encode_kv(node->keys[i], node->key_lens[i],
                                      node->values[i], 0, rec_buf);
        } else {
            rec_len = garry_encode_kv(node->keys[i], node->key_lens[i],
                                      node->values[i], node->value_lens[i], rec_buf);
        }
        if (rec_len > 0) {
            for (j = 0; j < rec_len; j++) {
                enc_data[j] = rec_buf[j];
            }
            slot_idx = garry_page_insert(*buf, enc_data, rec_len, pool->page_size);
            if (slot_idx < 0) {
                garry_pool_release_page(pool, pid);
                return;
            }
        }
    }

    garry_pool_mark_dirty(pool, pid);
    garry_pool_release_page(pool, pid);
}
