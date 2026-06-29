/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file gc.c
 * @brief MVCC garbage collection.
 *
 * Determines the oldest active transaction and prunes version chain
 * entries that are no longer visible to any active transaction. This
 * reclaims space in version chain pages and prevents unbounded growth.
 */

#include "gc.h"
#include "version_chain.h"
#include "slotted_page.h"
#include "garry_threading.h"
#include <string.h>

garry_txn_id garry_oldest_active_txid(garry_engine_handle *eng)
{
    garry_i32 i;
    garry_txn_id oldest;

    if (eng->active_count == 0) return eng->next_txid;

    oldest = eng->active_txns[0];
    for (i = 1; i < eng->active_count; i++) {
        if (eng->active_txns[i] < oldest) {
            oldest = eng->active_txns[i];
        }
    }
    return oldest;
}

void garry_mvcc_gc(garry_engine_handle *eng)
{
    garry_i32 pid;
    garry_page_buffer *buf;
    garry_txn_id snapshot[GARRY_CHAIN_MAX_ACTIVE];
    garry_i32 snapshot_count;
    garry_i32 i;

    garry_mutex_lock(&eng->txn_slot_mutex);
    snapshot_count = eng->active_count;
    for (i = 0; i < snapshot_count; i++) snapshot[i] = eng->active_txns[i];
    garry_mutex_unlock(&eng->txn_slot_mutex);

    for (pid = 1; pid < eng->header.total_pages; pid++) {
        buf = garry_pool_try_pin(eng->pool, pid);
        if (buf != NULL) {
            garry_page_type pt;
            pt = ((garry_page_header*)*buf)->page_type;
            if (pt == GARRY_NODE_CHAIN) {
                garry_rwlock_wrlock(&eng->root_lock);
                garry_chain_page_prune(eng->pool, *buf, (garry_u32)eng->page_size,
                                       snapshot, snapshot_count);
                garry_pool_mark_dirty(eng->pool, pid);
                garry_rwlock_wrunlock(&eng->root_lock);
            }
            garry_pool_release_page(eng->pool, pid);
        }
    }
}
