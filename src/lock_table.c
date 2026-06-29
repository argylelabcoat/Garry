/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file lock_table.c
 * @brief Key-level lock manager with shared/exclusive modes.
 *
 * Maintains a linked list of lock nodes per transaction. Each node
 * records the locked key and its mode (shared or exclusive). Conflict
 * detection checks whether a requested lock would conflict with locks
 * held by other active transactions on the same key.
 *
 * Thread safety: the lock manager is not internally synchronized;
 * callers must hold the engine mutex when acquiring or releasing locks.
 */

#include "lock_table.h"
#include <stdlib.h>
#include <string.h>

garry_lock_manager garry_create_lock_manager(void)
{
    garry_lock_manager m;
    m.head = NULL;
    m.count = 0;
    return m;
}

garry_bool garry_lock_held(garry_lock_manager* mgr, garry_txn_id txn,
                           const garry_byte* key, garry_i32 klen)
{
    garry_lock_node* p;
    garry_i32 j;
    garry_bool same;
    p = mgr->head;
    while (p != NULL) {
        if (p->txn == txn && p->key_len == klen) {
            same = 1;
            j = 0;
            while (j < klen && same) {
                if (p->key[j] != key[j]) {
                    same = 0;
                }
                j = j + 1;
            }
            if (same) {
                return 1;
            }
        }
        p = p->next;
    }
    return 0;
}

garry_bool garry_has_conflict(garry_lock_manager* mgr, garry_txn_id txn,
                              const garry_byte* key, garry_i32 klen,
                              garry_lock_mode mode)
{
    garry_lock_node* p;
    garry_i32 j;
    garry_bool same_key;
    garry_bool conflict;
    garry_bool found;
    conflict = 0;
    found = 0;
    p = mgr->head;
    while (p != NULL && !found) {
        if (p->key_len == klen) {
            same_key = 1;
            j = 0;
            while (j < klen && same_key) {
                if (p->key[j] != key[j]) {
                    same_key = 0;
                }
                j = j + 1;
            }
            if (same_key && p->txn != txn) {
                if (p->mode == GARRY_LOCK_EXCLUSIVE || mode == GARRY_LOCK_EXCLUSIVE) {
                    conflict = 1;
                    found = 1;
                }
            }
        }
        p = p->next;
    }
    return conflict;
}

void garry_lock_acquire(garry_lock_manager* mgr, garry_txn_id txn,
                        const garry_byte* key, garry_i32 klen,
                        garry_lock_mode mode, garry_bool* ok)
{
    garry_lock_node* n;
    garry_bool conflict;
    if (garry_lock_held(mgr, txn, key, klen)) {
        *ok = 1;
    } else {
        conflict = garry_has_conflict(mgr, txn, key, klen, mode);
        if (conflict) {
            *ok = 0;
        } else {
            n = (garry_lock_node*)malloc(sizeof(garry_lock_node));
            if (!n) {
                *ok = 0;
                return;
            }
            memcpy(n->key, key, sizeof(garry_byte_array));
            n->key_len = klen;
            n->txn = txn;
            n->mode = mode;
            n->next = mgr->head;
            mgr->head = n;
            mgr->count = mgr->count + 1;
            *ok = 1;
        }
    }
}

void garry_lock_release(garry_lock_manager* mgr, garry_txn_id txn)
{
    garry_lock_node* pp;
    /* Release from head first (matches original's head-pass removal). */
    while (mgr->head != NULL && mgr->head->txn == txn) {
        garry_lock_node* tmp = mgr->head;
        mgr->head = mgr->head->next;
        mgr->count = mgr->count - 1;
        free(tmp);
    }
    /* Then remove from middle/tail. */
    pp = mgr->head;
    while (pp != NULL && pp->next != NULL) {
        if (pp->next->txn == txn) {
            garry_lock_node* tmp = pp->next;
            pp->next = pp->next->next;
            mgr->count = mgr->count - 1;
            free(tmp);
            /* Don't advance — re-check current position for consecutive nodes. */
        } else {
            pp = pp->next;
        }
    }
}
