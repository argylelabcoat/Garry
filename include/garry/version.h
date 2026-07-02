/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

#ifndef GARRY_VERSION_H
#define GARRY_VERSION_H

#include "garry/export.h"
#include "garry/types.h"
#include "garry/transaction.h"

#ifndef GARRY_DATABASE_FWD_DEFINED
#define GARRY_DATABASE_FWD_DEFINED
typedef struct garry_database garry_database;
#endif

typedef struct garry_version_iter garry_version_iter;

GARRY_API garry_version_iter *garry_version_iter_open(
    garry_database *db, garry_txn txn,
    const garry_u8 *key, garry_i32 klen);

GARRY_API garry_bool garry_version_iter_next(
    garry_version_iter *iter,
    garry_i32 *txid,
    garry_u8 **value,
    garry_i32 *vlen,
    garry_bool *is_tombstone);

GARRY_API void garry_version_iter_close(garry_version_iter *iter);

#endif /* GARRY_VERSION_H */
