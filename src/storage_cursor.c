/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_cursor.c
 * @brief Prefix-scoped cursor for iterating over B-tree entries.
 *
 * Opens a cursor on the first key matching a prefix, then steps through
 * consecutive matching keys. Each next() call resolves MVCC visibility
 * so only committed values are returned to the caller.
 */

#include "storage_cursor.h"
#include "storage_ops.h"
#include "btree_search.h"
#include "version_chain.h"
#include <string.h>
#include <stdlib.h>

garry_storage_cursor garry_storage_cursor_open(garry_engine_handle *eng, garry_txn_id txn,
                                               const garry_byte *prefix, garry_i32 plen)
{
    garry_storage_cursor cur;
    garry_byte_array pfx;
    garry_u32 uplen;

    memset(&cur, 0, sizeof(cur));
    memset(pfx, 0, sizeof(pfx));
    if (prefix && plen > 0) {
        memcpy(pfx, prefix, (size_t)plen);
    }
    uplen = (garry_u32)plen;
    cur.btree_cur = garry_btree_cursor_open(eng->pool, eng->btree_root, &pfx, uplen);
    cur.eng = eng;
    cur.txn = txn;
    return cur;
}

garry_bool garry_storage_cursor_next(garry_storage_cursor *cur, garry_byte *key, garry_i32 *klen,
                                     garry_byte *value, garry_i32 *vlen)
{
    garry_byte_array bkey;
    garry_u32 bklen;
    garry_i32 cid;
    char *val;
    garry_i32 mv_len;

    if (!cur || cur->btree_cur.exhausted) return 0;

    for (;;) {
        if (!garry_btree_cursor_next(cur->eng->pool, &cur->btree_cur, &bkey, &bklen)) {
            return 0;
        }

        if (!cur->btree_cur.node_loaded) continue;

        {
            garry_btree_node *node = &cur->btree_cur.current_node;
            garry_i32 idx = (garry_i32)cur->btree_cur.current_slot - 1;
            garry_byte lookup[GARRY_LOOKUP_BUF_SIZE];
            garry_i32 lookup_len;

            if (idx < 0 || idx >= node->entry_count) continue;

            lookup_len = node->value_lens[idx];
            if (lookup_len <= 0 || lookup_len > GARRY_LOOKUP_BUF_SIZE) continue;
            memcpy(lookup, node->values[idx], (size_t)lookup_len);

            cid = garry_decode_cid_from_descriptor(lookup);
            if (cid < 0) continue;

            val = garry_mvcc_get(cur->eng, cur->txn, cid, &mv_len);
            if (!val) continue;

            if (key) {
                memcpy(key, bkey, sizeof(garry_byte_array));
            }
            if (klen) {
                *klen = (garry_i32)bklen;
            }
            if (value && vlen) {
                memcpy(value, val, (size_t)mv_len);
                *vlen = mv_len;
            } else if (value && !vlen) {
                free(val);
                return 0;
            } else if (vlen) {
                *vlen = 0;
            }
            free(val);
            return 1;
        }
    }
}

garry_bool garry_storage_cursor_peek(const garry_storage_cursor *cur,
                                     garry_byte *key, garry_i32 *klen)
{
    garry_byte_array bkey;
    garry_u32 bklen;

    if (!cur || cur->btree_cur.exhausted) return 0;
    if (!garry_btree_cursor_peek(cur->eng->pool, &cur->btree_cur, &bkey, &bklen)) {
        return 0;
    }
    if (key) {
        memcpy(key, bkey, sizeof(garry_byte_array));
    }
    if (klen) {
        *klen = (garry_i32)bklen;
    }
    return 1;
}

garry_bool garry_storage_cursor_skip_prefix(garry_storage_cursor *cur,
                                            const garry_byte *skip_prefix, garry_i32 skip_plen)
{
    garry_byte key[GARRY_MAX_KEY_SIZE];
    garry_i32 klen;
    garry_bool match;
    garry_i32 i;

    if (!cur || cur->btree_cur.exhausted) return 0;

    for (;;) {
        if (!garry_storage_cursor_next(cur, key, &klen, NULL, NULL)) {
            return 0;
        }
        match = 1;
        if (klen < skip_plen) {
            match = 0;
        } else {
            for (i = 0; i < skip_plen; i++) {
                if (key[i] != skip_prefix[i]) {
                    match = 0;
                    break;
                }
            }
        }
        if (!match) {
            if (klen <= (garry_i32)sizeof(garry_byte_array)) {
                memcpy(cur->btree_cur.saved_key, key, (size_t)klen);
            }
            cur->btree_cur.saved_klen = (garry_u32)klen;
            cur->btree_cur.has_saved = 1;
            return 1;
        }
    }
}

void garry_storage_cursor_close(garry_storage_cursor *cur)
{
    if (!cur) return;
    garry_btree_cursor_close(&cur->btree_cur);
}
