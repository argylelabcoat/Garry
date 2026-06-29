/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file garry_api.c
 * @brief Public API implementation for the Garry key-value storage engine.
 *
 * This is the primary entry point for callers. It translates high-level
 * operations (get, set, delete, cursor, transaction) into internal storage
 * engine calls, handling string-to-binary key conversion and cursor lifecycle.
 */

#include "garry/database.h"
#include "garry/transaction.h"
#include "garry/operations.h"
#include "garry/cursor.h"
#include "garry/navigation.h"
#include "garry/garry_string.h"
#include "garry/iterator.h"
#include "garry_internal.h"
#include "storage_engine.h"
#include "storage_txn.h"
#include "storage_ops.h"
#include "storage_cursor.h"
#include "storage_navigation.h"
#include "engine_settings.h"
#include <stdlib.h>
#include <string.h>

struct garry_cursor {
    garry_storage_cursor scur;
};

garry_database* garry_database_create(const char *path)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_database *db;

    if (!path) return NULL;

    settings = garry_default_engine_settings();

    eng = garry_storage_init(path, settings);
    if (!eng) return NULL;

    db = (garry_database*)malloc(sizeof(garry_database));
    if (!db) {
        garry_storage_close(eng);
        return NULL;
    }
    db->eng = eng;
    return db;
}

garry_database* garry_database_create_with_config(const char *path, garry_config config)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_database *db;

    if (!path) return NULL;

    settings = garry_default_engine_settings();
    settings.pool_size       = config.pool_size;
    settings.max_record_size = config.max_record_size;
    settings.page_size       = config.page_size;
    settings.max_txns        = config.max_txns;
    settings.max_versions    = config.max_versions;
    settings.compression     = config.compression;
    settings.btree_flags     = config.btree_flags;

    eng = garry_storage_init(path, settings);
    if (!eng) return NULL;

    db = (garry_database*)malloc(sizeof(garry_database));
    if (!db) {
        garry_storage_close(eng);
        return NULL;
    }
    db->eng = eng;
    return db;
}

garry_database* garry_database_open(const char *path)
{
    garry_engine_handle *eng;
    garry_database *db;

    if (!path) return NULL;

    eng = garry_storage_open(path);
    if (!eng) return NULL;

    db = (garry_database*)malloc(sizeof(garry_database));
    if (!db) {
        garry_storage_close(eng);
        return NULL;
    }
    db->eng = eng;
    return db;
}

void garry_database_close(garry_database *db)
{
    if (!db) return;
    garry_storage_close(db->eng);
    free(db);
}

garry_txn garry_txn_begin(garry_database *db)
{
    if (!db) return -1;
    return garry_storage_begin(db->eng);
}

void garry_txn_commit(garry_database *db, garry_txn txn)
{
    if (!db) return;
    garry_storage_commit(db->eng, txn);
}

void garry_txn_rollback(garry_database *db, garry_txn txn)
{
    if (!db) return;
    garry_storage_rollback(db->eng, txn);
}

garry_status_t garry_get(garry_database *db, garry_txn txn,
                         const garry_u8 *key, garry_i32 klen,
                         garry_u8 *value, garry_i32 *vlen)
{
    if (!db) return GARRY_ERR_INVALID_ARG;
    return garry_storage_get(db->eng, txn, key, klen, value, vlen)
        ? GARRY_OK : GARRY_ERR_NOT_FOUND;
}

garry_status_t garry_set(garry_database *db, garry_txn txn,
                         const garry_u8 *key, garry_i32 klen,
                         const garry_u8 *value, garry_i32 vlen)
{
    if (!db) return GARRY_ERR_INVALID_ARG;
    return garry_storage_set(db->eng, txn, key, klen, value, vlen)
        ? GARRY_OK : GARRY_ERR_INVALID_ARG;
}

garry_status_t garry_delete(garry_database *db, garry_txn txn,
                            const garry_u8 *key, garry_i32 klen)
{
    if (!db) return GARRY_ERR_INVALID_ARG;
    return garry_storage_delete(db->eng, txn, key, klen)
        ? GARRY_OK : GARRY_ERR_NOT_FOUND;
}

garry_status_t garry_get_default(garry_database *db, garry_txn txn,
                                 const garry_u8 *key, garry_i32 klen,
                                 const garry_u8 *def, garry_i32 dlen,
                                 garry_u8 *value, garry_i32 *vlen)
{
    if (!db) return GARRY_ERR_INVALID_ARG;
    return garry_storage_get_default(db->eng, txn, key, klen, def, dlen, value, vlen)
        ? GARRY_OK : GARRY_ERR_INVALID_ARG;
}

garry_i32 garry_data(garry_database *db, garry_txn txn,
                     const garry_u8 *key, garry_i32 klen)
{
    if (!db) return 0;
    return garry_storage_data(db->eng, txn, key, klen);
}

garry_bool garry_first(garry_database *db, garry_txn txn,
                       garry_u8 *key, garry_i32 *klen)
{
    if (!db) return 0;
    return garry_storage_first(db->eng, txn, key, klen);
}

garry_bool garry_last(garry_database *db, garry_txn txn,
                      garry_u8 *key, garry_i32 *klen)
{
    if (!db) return 0;
    return garry_storage_last(db->eng, txn, key, klen);
}

garry_bool garry_next_key(garry_database *db, garry_txn txn,
                          const garry_u8 *after, garry_i32 after_len,
                          garry_u8 *key, garry_i32 *klen)
{
    if (!db) return 0;
    return garry_storage_next_key(db->eng, txn, after, after_len, key, klen);
}

garry_bool garry_prev_key(garry_database *db, garry_txn txn,
                          const garry_u8 *before, garry_i32 before_len,
                          garry_u8 *key, garry_i32 *klen)
{
    if (!db) return 0;
    return garry_storage_prev_key(db->eng, txn, before, before_len, key, klen);
}

garry_bool garry_exists(garry_database *db, garry_txn txn,
                        const garry_u8 *key, garry_i32 klen)
{
    if (!db) return 0;
    return garry_storage_exists(db->eng, txn, key, klen);
}

garry_i32 garry_count(garry_database *db, garry_txn txn)
{
    if (!db) return 0;
    return garry_storage_count(db->eng, txn);
}

garry_cursor* garry_cursor_open(garry_database *db, garry_txn txn,
                                const garry_u8 *prefix, garry_i32 plen)
{
    garry_cursor *cur;

    if (!db) return NULL;

    cur = (garry_cursor*)malloc(sizeof(garry_cursor));
    if (!cur) return NULL;

    cur->scur = garry_storage_cursor_open(db->eng, txn, prefix, plen);
    return cur;
}

garry_bool garry_cursor_next(garry_cursor *cur,
                             garry_u8 *key, garry_i32 *klen,
                             garry_u8 *value, garry_i32 *vlen)
{
    if (!cur) return 0;
    return garry_storage_cursor_next(&cur->scur, key, klen, value, vlen);
}

garry_bool garry_cursor_next_key(garry_cursor *cur,
                                 garry_u8 *key, garry_i32 *klen)
{
    if (!cur) return 0;
    return garry_storage_cursor_next(&cur->scur, key, klen, NULL, NULL);
}

void garry_cursor_close(garry_cursor *cur)
{
    if (!cur) return;
    garry_storage_cursor_close(&cur->scur);
    free(cur);
}

garry_status_t garry_set_str(garry_database *db, garry_txn txn,
                             const char *key, const char *value)
{
    garry_i32 klen, vlen;

    if (!db || !key || !value) return GARRY_ERR_INVALID_ARG;
    klen = (garry_i32)strlen(key);
    if (klen > GARRY_MAX_KEY_SIZE) return GARRY_ERR_BUFFER_TOO_SMALL;
    vlen = (garry_i32)strlen(value);
    return garry_set(db, txn, (const garry_u8*)key, klen,
                     (const garry_u8*)value, vlen);
}

garry_status_t garry_get_str(garry_database *db, garry_txn txn,
                             const char *key, char *value, garry_i32 value_size)
{
    garry_i32 klen, vlen;
    garry_status_t rc;

    if (!db || !key || !value || value_size <= 0) return GARRY_ERR_INVALID_ARG;
    klen = (garry_i32)strlen(key);
    if (klen > GARRY_MAX_KEY_SIZE) return GARRY_ERR_BUFFER_TOO_SMALL;
    vlen = value_size - 1;
    rc = garry_get(db, txn, (const garry_u8*)key, klen,
                   (garry_u8*)value, &vlen);
    if (rc == GARRY_OK) {
        value[vlen] = '\0';
    }
    return rc;
}

/**
 * @brief Iterate over all key-value pairs matching a prefix.
 *
 * Convenience wrapper that opens a cursor, calls the visitor for each
 * matching entry, and closes the cursor. Allocates and frees a cursor
 * per call — callers needing high-throughput scans should open a
 * cursor directly via garry_cursor_open / garry_cursor_next /
 * garry_cursor_close to amortize the allocation cost.
 *
 * @param db        Database handle
 * @param txn       Active transaction ID
 * @param prefix    Key prefix to match (NULL for all keys)
 * @param plen      Prefix length (0 if prefix is NULL)
 * @param visitor   Callback invoked for each matching key-value pair
 * @param user_data Opaque pointer forwarded to the visitor
 */
void garry_for_each(garry_database *db, garry_txn txn,
                    const garry_u8 *prefix, garry_i32 plen,
                    garry_visitor visitor, void *user_data)
{
    garry_cursor *cur;
    garry_u8 key[GARRY_MAX_KEY_SIZE];
    garry_u8 value[GARRY_MAX_RECORD_SIZE];
    garry_i32 klen, vlen;

    if (!db || !visitor) return;

    cur = garry_cursor_open(db, txn, prefix, plen);
    if (!cur) return;

    while (garry_cursor_next(cur, key, &klen, value, &vlen)) {
        visitor(key, klen, value, vlen, user_data);
    }

    garry_cursor_close(cur);
}
