/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file transaction.c
 * @brief MVCC transaction lifecycle and version chain management.
 *
 * Implements multi-version concurrency control: transaction begin allocates
 * a slot, commit finalizes writes, and rollback reverts them. Writes are
 * appended to per-key version chains that are resolved at read time by
 * visibility rules.
 *
 * Threading: the engine handle's mutex serializes slot allocation and
 * version chain appends; the rwlock allows concurrent reads.
 */

#include "transaction.h"
#include "slotted_page.h"
#include "db_header.h"
#include "btree_node.h"
#include "version_chain.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifndef GARRY_TRUE
#define GARRY_TRUE 1
#endif
#ifndef GARRY_FALSE
#define GARRY_FALSE 0
#endif

garry_i32 garry_chain_id_encode(garry_i32 chain_id, garry_byte *out)
{
    out[0] = (garry_byte)(chain_id & 0xFF);
    out[1] = (garry_byte)((chain_id >> 8) & 0xFF);
    out[2] = (garry_byte)((chain_id >> 16) & 0xFF);
    out[3] = (garry_byte)((chain_id >> 24) & 0xFF);
    return 4;
}

garry_i32 garry_chain_id_decode(const garry_byte *encoded)
{
    garry_i32 b0, b1, b2, b3;
    b0 = (garry_i32)encoded[0];
    b1 = (garry_i32)encoded[1];
    b2 = (garry_i32)encoded[2];
    b3 = (garry_i32)encoded[3];
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

static void remove_active_txn(garry_engine_handle *eng, garry_txn_id txn)
{
    garry_i32 i, j;
    for (i = 0; i < eng->active_count; i++) {
        if (eng->active_txns[i] == txn) {
            for (j = i; j < eng->active_count - 1; j++) {
                eng->active_txns[j] = eng->active_txns[j + 1];
                eng->txn_states[j] = eng->txn_states[j + 1];
            }
            eng->active_count--;
            return;
        }
    }
}

static garry_i32 find_txn_slot(garry_engine_handle *eng, garry_txn_id txn)
{
    garry_i32 i;
    for (i = 0; i < eng->active_count; i++) {
        if (eng->active_txns[i] == txn) return i;
    }
    return -1;
}

garry_engine_handle* garry_engine_init(const char *path, garry_engine_settings settings)
{
    garry_engine_handle *eng;
    garry_page_buffer *hdr_buf;
    garry_byte wal_path[PATH_MAX];
    garry_byte ckpt_path[PATH_MAX];
    garry_i32 path_len;

    eng = (garry_engine_handle*)malloc(sizeof(garry_engine_handle));
    if (!eng) return NULL;
    memset(eng, 0, sizeof(garry_engine_handle));

    eng->settings = settings;
    eng->page_size = settings.page_size;
    eng->compression = settings.compression;
    eng->max_txns = settings.max_txns;
    eng->next_txid = 1;
    eng->active_count = 0;

    eng->active_txns = (garry_txn_id*)malloc(sizeof(garry_txn_id) * settings.max_txns);
    eng->txn_states = (garry_txn_info*)malloc(sizeof(garry_txn_info) * settings.max_txns);
    if (!eng->active_txns || !eng->txn_states) {
        free(eng->active_txns);
        free(eng->txn_states);
        free(eng);
        return NULL;
    }
    memset(eng->active_txns, 0, sizeof(garry_txn_id) * settings.max_txns);
    memset(eng->txn_states, 0, sizeof(garry_txn_info) * settings.max_txns);

    eng->pool = garry_pool_create(path, (garry_u32)settings.pool_size, (garry_u32)settings.page_size);
    if (!eng->pool) {
        free(eng->active_txns);
        free(eng->txn_states);
        free(eng);
        return NULL;
    }
    eng->pool->next_page = GARRY_FIRST_PAGE_ID;

    path_len = (garry_i32)strlen(path);
    if (path_len > PATH_MAX - 6) path_len = PATH_MAX - 6;
    memcpy(wal_path, path, (size_t)path_len);
    wal_path[path_len] = '-'; wal_path[path_len + 1] = 'w';
    wal_path[path_len + 2] = 'a'; wal_path[path_len + 3] = 'l';
    wal_path[path_len + 4] = '\0';
    memcpy(ckpt_path, path, (size_t)path_len);
    ckpt_path[path_len] = '-'; ckpt_path[path_len + 1] = 'c';
    ckpt_path[path_len + 2] = 'k'; ckpt_path[path_len + 3] = 'p';
    ckpt_path[path_len + 4] = 't'; ckpt_path[path_len + 5] = '\0';
    if (garry_wal_log_init(&eng->wal, (const char*)wal_path, (const char*)ckpt_path) != GARRY_OK) {
        garry_pool_close(eng->pool);
        free(eng->active_txns);
        free(eng->txn_states);
        free(eng);
        return NULL;
    }

    eng->lock_mgr = garry_create_lock_manager();
    garry_rwlock_init(&eng->root_lock);
    garry_mutex_init(&eng->txn_slot_mutex);

    eng->header = garry_create_db_header(&settings);
    eng->btree_root = eng->header.root_page;

    hdr_buf = garry_pool_pin_page(eng->pool, GARRY_HEADER_PAGE);
    if (hdr_buf != NULL) {
        garry_write_db_header((garry_byte*)*hdr_buf, &eng->header);
        garry_pool_mark_dirty(eng->pool, GARRY_HEADER_PAGE);
        garry_pool_release_page(eng->pool, GARRY_HEADER_PAGE);
    }

    {
        garry_btree_node root_node;
        garry_page_buffer *root_buf;
        root_buf = garry_pool_pin_page(eng->pool, eng->btree_root);
        if (root_buf != NULL) {
            garry_load_node(eng->pool, eng->btree_root, &root_node);
            if (root_node.entry_count == 0 && root_node.kind != GARRY_NODE_LEAF) {
                root_node.kind = GARRY_NODE_LEAF;
                root_node.header = garry_create_header(GARRY_NODE_LEAF, 0);
                root_node.entry_count = 0;
                root_node.next_page = -1;
                root_node.prev_page = -1;
                garry_store_node(eng->pool, eng->btree_root, &root_node);
                garry_pool_mark_dirty(eng->pool, eng->btree_root);
            }
            garry_pool_release_page(eng->pool, eng->btree_root);
        }
    }

    return eng;
}

garry_engine_handle* garry_engine_open(const char *path)
{
    garry_engine_handle *eng;
    garry_page_buffer *hdr_buf;
    garry_byte wal_path[PATH_MAX];
    garry_byte ckpt_path[PATH_MAX];
    garry_i32 path_len;
    garry_file_descriptor raw_fd;
    garry_byte hdr_raw[GARRY_MAX_PAGE_SIZE];
    garry_i32 page_size;
    garry_db_header tmp_hdr;

    eng = (garry_engine_handle*)malloc(sizeof(garry_engine_handle));
    if (!eng) return NULL;
    memset(eng, 0, sizeof(garry_engine_handle));

    raw_fd = garry_file_open(path, GARRY_O_RDONLY);
    if (!raw_fd.is_open) {
        free(eng);
        return NULL;
    }
    memset(hdr_raw, 0, sizeof(hdr_raw));
    garry_file_read_page(&raw_fd, 0, hdr_raw, GARRY_MAX_PAGE_SIZE);
    tmp_hdr = garry_read_db_header(hdr_raw);
    page_size = tmp_hdr.page_size;
    if (page_size < GARRY_MIN_PAGE_SIZE || page_size > GARRY_MAX_PAGE_SIZE) {
        page_size = GARRY_DEFAULT_PAGE_SIZE;
    }
    garry_file_close(&raw_fd);

    eng->pool = garry_pool_create(path, (garry_u32)garry_default_engine_settings().pool_size, (garry_u32)page_size);
    if (!eng->pool) {
        free(eng);
        return NULL;
    }
    eng->pool->next_page = GARRY_FIRST_PAGE_ID;

    hdr_buf = garry_pool_pin_page(eng->pool, GARRY_HEADER_PAGE);
    if (hdr_buf != NULL) {
        eng->header = garry_read_db_header((garry_byte*)*hdr_buf);
        garry_pool_release_page(eng->pool, GARRY_HEADER_PAGE);
    } else {
        free(eng->pool);
        free(eng);
        return NULL;
    }

    eng->page_size = eng->header.page_size;
    eng->compression = eng->header.compression;
    eng->max_txns = eng->header.max_txns;
    eng->btree_root = eng->header.root_page;
    eng->next_txid = 1;
    eng->active_count = 0;

    eng->settings.page_size = eng->header.page_size;
    eng->settings.compression = eng->header.compression;
    eng->settings.max_txns = eng->header.max_txns;
    eng->settings.max_versions = eng->header.max_versions;
    eng->settings.max_key_size = eng->header.max_key_size;
    eng->settings.max_subscripts = eng->header.max_subscripts;
    eng->settings.btree_flags = eng->header.btree_flags;

    eng->active_txns = (garry_txn_id*)malloc(sizeof(garry_txn_id) * eng->max_txns);
    eng->txn_states = (garry_txn_info*)malloc(sizeof(garry_txn_info) * eng->max_txns);
    if (!eng->active_txns || !eng->txn_states) {
        free(eng->active_txns);
        free(eng->txn_states);
        free(eng->pool);
        free(eng);
        return NULL;
    }
    memset(eng->active_txns, 0, sizeof(garry_txn_id) * eng->max_txns);
    memset(eng->txn_states, 0, sizeof(garry_txn_info) * eng->max_txns);

    path_len = (garry_i32)strlen(path);
    if (path_len > PATH_MAX - 6) path_len = PATH_MAX - 6;
    memcpy(wal_path, path, (size_t)path_len);
    wal_path[path_len] = '-'; wal_path[path_len + 1] = 'w';
    wal_path[path_len + 2] = 'a'; wal_path[path_len + 3] = 'l';
    wal_path[path_len + 4] = '\0';
    memcpy(ckpt_path, path, (size_t)path_len);
    ckpt_path[path_len] = '-'; ckpt_path[path_len + 1] = 'c';
    ckpt_path[path_len + 2] = 'k'; ckpt_path[path_len + 3] = 'p';
    ckpt_path[path_len + 4] = 't'; ckpt_path[path_len + 5] = '\0';
    if (garry_wal_log_init(&eng->wal, (const char*)wal_path, (const char*)ckpt_path) != GARRY_OK) {
        garry_pool_close(eng->pool);
        free(eng->active_txns);
        free(eng->txn_states);
        free(eng);
        return NULL;
    }

    eng->lock_mgr = garry_create_lock_manager();
    garry_rwlock_init(&eng->root_lock);
    garry_mutex_init(&eng->txn_slot_mutex);

    return eng;
}

void garry_engine_close(garry_engine_handle *eng)
{
    garry_lock_node *node, *next;
    if (!eng) return;
    garry_pool_flush_all(eng->pool);
    garry_wal_log_close(&eng->wal);
    garry_pool_close(eng->pool);
    garry_rwlock_destroy(&eng->root_lock);
    garry_mutex_destroy(&eng->txn_slot_mutex);
    node = eng->lock_mgr.head;
    while (node != NULL) {
        next = node->next;
        free(node);
        node = next;
    }
    free(eng->active_txns);
    free(eng->txn_states);
    free(eng);
}

garry_i32 garry_chain_allocate(garry_engine_handle *eng, const garry_byte *key, garry_i32 klen)
{
    garry_i32 pid;
    garry_page_buffer *buf;
    (void)key;
    (void)klen;

    pid = garry_pool_allocate(eng->pool);
    if (pid < 0) return -1;

    buf = garry_pool_pin_page(eng->pool, pid);
    if (!buf) return -1;

    garry_chain_page_init(*buf, (garry_u32)eng->page_size);
    garry_pool_mark_dirty(eng->pool, pid);
    garry_pool_release_page(eng->pool, pid);

    return pid;
}

garry_txn_id garry_mvcc_begin(garry_engine_handle *eng)
{
    garry_txn_id txn;
    garry_i32 slot;

    garry_mutex_lock(&eng->txn_slot_mutex);
    if (eng->active_count >= eng->max_txns) {
        garry_mutex_unlock(&eng->txn_slot_mutex);
        return -1;
    }

    txn = eng->next_txid;
    eng->next_txid++;

    slot = eng->active_count;
    eng->active_txns[slot] = txn;
    eng->txn_states[slot].txid = txn;
    eng->txn_states[slot].state = GARRY_TXN_ACTIVE;
    eng->txn_states[slot].modified_count = 0;
    eng->active_count++;
    garry_mutex_unlock(&eng->txn_slot_mutex);

    return txn;
}

void garry_mvcc_commit(garry_engine_handle *eng, garry_txn_id txn)
{
    garry_i32 slot;
    garry_i32 i, count;
    garry_i32 pages[GARRY_MAX_MODIFIED_PAGES];

    garry_mutex_lock(&eng->txn_slot_mutex);
    slot = find_txn_slot(eng, txn);
    if (slot >= 0) {
        count = eng->txn_states[slot].modified_count;
        if (count > GARRY_MAX_MODIFIED_PAGES) count = GARRY_MAX_MODIFIED_PAGES;
        for (i = 0; i < count; i++) {
            pages[i] = eng->txn_states[slot].modified_pages[i];
        }
        eng->txn_states[slot].state = GARRY_TXN_COMMITTED;
        eng->txn_states[slot].modified_count = 0;
        remove_active_txn(eng, txn);
    } else {
        count = 0;
    }
    garry_mutex_unlock(&eng->txn_slot_mutex);

    for (i = 0; i < count; i++) {
        garry_pool_flush_page(eng->pool, pages[i]);
    }
}

void garry_mvcc_rollback(garry_engine_handle *eng, garry_txn_id txn)
{
    garry_i32 slot;
    garry_mutex_lock(&eng->txn_slot_mutex);
    slot = find_txn_slot(eng, txn);
    if (slot >= 0) {
        eng->txn_states[slot].state = GARRY_TXN_ROLLED_BACK;
        remove_active_txn(eng, txn);
    }
    garry_mutex_unlock(&eng->txn_slot_mutex);
}

garry_bool garry_txn_is_active(garry_txn_id txn, garry_engine_handle *eng)
{
    garry_bool result;
    garry_mutex_lock(&eng->txn_slot_mutex);
    result = find_txn_slot(eng, txn) >= 0 ? GARRY_TRUE : GARRY_FALSE;
    garry_mutex_unlock(&eng->txn_slot_mutex);
    return result;
}

char* garry_mvcc_get(garry_engine_handle *eng, garry_txn_id txn,
                     garry_i32 chain_page_id, garry_i32 *vlen)
{
    garry_page_buffer *buf;
    char *result;

    buf = garry_pool_pin_page(eng->pool, chain_page_id);
    if (!buf) return NULL;

    garry_mutex_lock(&eng->txn_slot_mutex);
    result = garry_chain_page_find_visible(
        eng->pool, *buf, (garry_u32)eng->page_size,
        txn, eng->active_txns, eng->active_count, vlen);
    garry_mutex_unlock(&eng->txn_slot_mutex);

    garry_pool_release_page(eng->pool, chain_page_id);
    return result;
}

garry_bool garry_mvcc_set(garry_engine_handle *eng, garry_txn_id txn,
                           garry_i32 chain_page_id, const char *value, garry_i32 vlen)
{
    garry_page_buffer *buf;
    garry_bool ok;
    garry_i32 slot;
    garry_i32 inline_cap;

    buf = garry_pool_pin_page(eng->pool, chain_page_id);
    if (!buf) return GARRY_FALSE;

    inline_cap = garry_chain_inline_capacity((garry_u32)eng->page_size);

    if (vlen > inline_cap) {
        garry_i32 head;
        head = garry_overflow_write(eng->pool, value, vlen);
        if (head < 0) {
            garry_pool_release_page(eng->pool, chain_page_id);
            return GARRY_FALSE;
        }
        ok = garry_chain_page_append_overflow(*buf, (garry_u32)eng->page_size,
                                              txn, vlen, head);
    } else {
        ok = garry_chain_page_append(*buf, (garry_u32)eng->page_size,
                                     txn, value, vlen);
    }

    if (ok) {
        garry_pool_mark_dirty(eng->pool, chain_page_id);
        garry_mutex_lock(&eng->txn_slot_mutex);
        slot = find_txn_slot(eng, txn);
        if (slot >= 0) {
            garry_i32 mc = eng->txn_states[slot].modified_count;
            if (mc < GARRY_MAX_MODIFIED_PAGES) {
                eng->txn_states[slot].modified_pages[mc] = chain_page_id;
            }
            eng->txn_states[slot].modified_count = mc + 1;
        }
        garry_mutex_unlock(&eng->txn_slot_mutex);
    }

    garry_pool_release_page(eng->pool, chain_page_id);
    return ok;
}

garry_bool garry_mvcc_delete(garry_engine_handle *eng, garry_txn_id txn,
                             garry_i32 chain_page_id)
{
    garry_page_buffer *buf;
    garry_bool ok;
    garry_i32 slot;

    buf = garry_pool_pin_page(eng->pool, chain_page_id);
    if (!buf) return GARRY_FALSE;

    ok = garry_chain_page_append_tombstone(*buf, (garry_u32)eng->page_size, txn);

    if (ok) {
        garry_pool_mark_dirty(eng->pool, chain_page_id);
        garry_mutex_lock(&eng->txn_slot_mutex);
        slot = find_txn_slot(eng, txn);
        if (slot >= 0) {
            garry_i32 mc = eng->txn_states[slot].modified_count;
            if (mc < GARRY_MAX_MODIFIED_PAGES) {
                eng->txn_states[slot].modified_pages[mc] = chain_page_id;
            }
            eng->txn_states[slot].modified_count = mc + 1;
        }
        garry_mutex_unlock(&eng->txn_slot_mutex);
    }

    garry_pool_release_page(eng->pool, chain_page_id);
    return ok;
}

garry_i32 garry_mvcc_chain_overflow(garry_engine_handle *eng, garry_txn_id txn,
                                    const char *value, garry_i32 vlen)
{
    garry_i32 head;
    head = garry_overflow_write(eng->pool, value, vlen);
    return head;
}

garry_bool garry_mvcc_recovery_apply(garry_engine_handle *eng, garry_i32 chain_page_id,
                                     garry_txn_id txn, const char *value, garry_i32 vlen)
{
    return garry_mvcc_set(eng, txn, chain_page_id, value, vlen);
}
