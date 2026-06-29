/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_cursor.h
 * @brief B-tree cursor for prefix-scoped iteration.
 */

#ifndef GARRY_STORAGE_CURSOR_H
#define GARRY_STORAGE_CURSOR_H

#include "transaction.h"
#include "btree_cursor.h"

typedef struct
{
    garry_btree_cursor_handle btree_cur;
    garry_engine_handle *eng;
    garry_txn_id txn;
} garry_storage_cursor;

garry_storage_cursor garry_storage_cursor_open(garry_engine_handle *eng, garry_txn_id txn,
                                               const garry_byte *prefix, garry_i32 plen);
garry_bool garry_storage_cursor_next(garry_storage_cursor *cur, garry_byte *key, garry_i32 *klen,
                                     garry_byte *value, garry_i32 *vlen);
garry_bool garry_storage_cursor_peek(garry_storage_cursor *cur, garry_byte *key, garry_i32 *klen);
void garry_storage_cursor_close(garry_storage_cursor *cur);

#endif /* GARRY_STORAGE_CURSOR_H */
