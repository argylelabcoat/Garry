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

struct garry_cursor
{
    garry_storage_cursor scur;
};

/**
 * @brief Create a new database with default settings.
 *
 * @param path Filesystem path for the database files
 * @return Database handle, or NULL on failure
 */
garry_database *garry_database_create(const char *path)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_database *db;

    if (!path)
        return NULL;

    settings = garry_default_engine_settings();

    eng = garry_storage_init(path, settings);
    if (!eng)
        return NULL;

    db = (garry_database *)malloc(sizeof(garry_database));
    if (!db)
    {
        garry_storage_close(eng);
        return NULL;
    }
    db->eng = eng;
    return db;
}

/**
 * @brief Create a new database with caller-specified configuration.
 *
 * @param path   Filesystem path for the database files
 * @param config Configuration overrides applied to default settings
 * @return Database handle, or NULL on failure
 */
garry_database *garry_database_create_with_config(const char *path, garry_config config)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_database *db;

    if (!path)
        return NULL;

    settings = garry_default_engine_settings();
    settings.pool_size = config.pool_size;
    settings.max_record_size = config.max_record_size;
    settings.page_size = config.page_size;
    settings.max_txns = config.max_txns;
    settings.max_versions = config.max_versions;
    settings.max_key_size = config.max_key_size;
    settings.max_subscripts = config.max_subscripts;
    settings.compression = config.compression;
    settings.btree_flags = config.btree_flags;

    eng = garry_storage_init(path, settings);
    if (!eng)
        return NULL;

    db = (garry_database *)malloc(sizeof(garry_database));
    if (!db)
    {
        garry_storage_close(eng);
        return NULL;
    }
    db->eng = eng;
    return db;
}

/**
 * @brief Open an existing database.
 *
 * @param path Filesystem path of the database to open
 * @return Database handle, or NULL on failure
 */
garry_database *garry_database_open(const char *path)
{
    garry_engine_handle *eng;
    garry_database *db;

    if (!path)
        return NULL;

    eng = garry_storage_open(path);
    if (!eng)
        return NULL;

    db = (garry_database *)malloc(sizeof(garry_database));
    if (!db)
    {
        garry_storage_close(eng);
        return NULL;
    }
    db->eng = eng;
    return db;
}

/**
 * @brief Close a database and release all associated resources.
 *
 * @param db Database handle to close (NULL is a no-op)
 */
void garry_database_close(garry_database *db)
{
    if (!db)
        return;
    garry_storage_close(db->eng);
    free(db);
}

/**
 * @brief Begin a new transaction.
 *
 * @param db Database handle
 * @return Transaction ID, or -1 on failure
 */
garry_txn garry_txn_begin(garry_database *db)
{
    if (!db)
        return -1;
    return garry_storage_begin(db->eng);
}

/**
 * @brief Commit an active transaction, persisting all writes.
 *
 * @param db  Database handle
 * @param txn Transaction ID to commit
 */
void garry_txn_commit(garry_database *db, garry_txn txn)
{
    if (!db)
        return;
    garry_storage_commit(db->eng, txn);
}

/**
 * @brief Roll back an active transaction, discarding all writes.
 *
 * @param db  Database handle
 * @param txn Transaction ID to roll back
 */
void garry_txn_rollback(garry_database *db, garry_txn txn)
{
    if (!db)
        return;
    garry_storage_rollback(db->eng, txn);
}

/**
 * @brief Retrieve a value by key within a transaction.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Key bytes to look up
 * @param klen  Length of the key
 * @param value Output buffer for the value
 * @param vlen  On input, capacity of value buffer; on output, actual value length
 * @return GARRY_OK on success, GARRY_ERR_NOT_FOUND if key absent
 */
garry_status_t garry_get(garry_database *db, garry_txn txn, const garry_u8 *key, garry_i32 klen,
                         garry_u8 *value, garry_i32 *vlen)
{
    garry_bool found;
    if (!db)
        return GARRY_ERR_INVALID_ARG;
    found = garry_storage_get(db->eng, txn, key, klen, value, vlen);
    if (found)
        return GARRY_OK;
    if (!garry_txn_is_active(txn, db->eng))
        return GARRY_ERR_INVALID_ARG;
    return GARRY_ERR_NOT_FOUND;
}

/**
 * @brief Store a key-value pair within a transaction.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Key bytes
 * @param klen  Length of the key
 * @param value Value bytes to store
 * @param vlen  Length of the value
 * @return GARRY_OK on success, or an error code
 */
garry_status_t garry_set(garry_database *db, garry_txn txn, const garry_u8 *key, garry_i32 klen,
                         const garry_u8 *value, garry_i32 vlen)
{
    if (!db)
        return GARRY_ERR_INVALID_ARG;
    return garry_storage_set(db->eng, txn, key, klen, value, vlen) ? GARRY_OK
                                                                   : GARRY_ERR_INVALID_ARG;
}

/**
 * @brief Delete a key-value pair within a transaction.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Key bytes to delete
 * @param klen  Length of the key
 * @return GARRY_OK on success, GARRY_ERR_NOT_FOUND if key absent
 */
garry_status_t garry_delete(garry_database *db, garry_txn txn, const garry_u8 *key, garry_i32 klen)
{
    if (!db)
        return GARRY_ERR_INVALID_ARG;
    return garry_storage_delete(db->eng, txn, key, klen) ? GARRY_OK : GARRY_ERR_NOT_FOUND;
}

/**
 * @brief Retrieve a value by key, returning a default if absent.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Key bytes to look up
 * @param klen  Length of the key
 * @param def   Default value bytes
 * @param dlen  Length of the default value
 * @param value Output buffer for the result
 * @param vlen  On input, capacity of value buffer; on output, actual length
 * @return GARRY_OK on success, or an error code
 */
garry_status_t garry_get_default(garry_database *db, garry_txn txn, const garry_u8 *key,
                                 garry_i32 klen, const garry_u8 *def, garry_i32 dlen,
                                 garry_u8 *value, garry_i32 *vlen)
{
    if (!db)
        return GARRY_ERR_INVALID_ARG;
    return garry_storage_get_default(db->eng, txn, key, klen, def, dlen, value, vlen)
               ? GARRY_OK
               : GARRY_ERR_INVALID_ARG;
}

/**
 * @brief Return the size in bytes of the value for a given key.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Key bytes
 * @param klen  Length of the key
 * @return Value size, or 0 if the key does not exist
 */
garry_i32 garry_data(garry_database *db, garry_txn txn, const garry_u8 *key, garry_i32 klen)
{
    if (!db)
        return 0;
    return garry_storage_data(db->eng, txn, key, klen);
}

/**
 * @brief Seek to the first key in the database.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Output buffer for the first key
 * @param klen  Output length of the first key
 * @return GARRY_TRUE if a first key exists, GARRY_FALSE otherwise
 */
garry_bool garry_first(garry_database *db, garry_txn txn, garry_u8 *key, garry_i32 *klen)
{
    if (!db)
        return 0;
    return garry_storage_first(db->eng, txn, key, klen);
}

/**
 * @brief Seek to the last key in the database.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Output buffer for the last key
 * @param klen  Output length of the last key
 * @return GARRY_TRUE if a last key exists, GARRY_FALSE otherwise
 */
garry_bool garry_last(garry_database *db, garry_txn txn, garry_u8 *key, garry_i32 *klen)
{
    if (!db)
        return 0;
    return garry_storage_last(db->eng, txn, key, klen);
}

/**
 * @brief Advance to the key immediately after the given key.
 *
 * @param db        Database handle
 * @param txn       Transaction ID
 * @param after     Key bytes to seek past
 * @param after_len Length of the after key
 * @param key       Output buffer for the next key
 * @param klen      Output length of the next key
 * @return GARRY_TRUE if a next key exists, GARRY_FALSE otherwise
 */
garry_bool garry_next_key(garry_database *db, garry_txn txn, const garry_u8 *after,
                          garry_i32 after_len, garry_u8 *key, garry_i32 *klen)
{
    if (!db)
        return 0;
    return garry_storage_next_key(db->eng, txn, after, after_len, key, klen);
}

/**
 * @brief Retreat to the key immediately before the given key.
 *
 * @param db         Database handle
 * @param txn        Transaction ID
 * @param before     Key bytes to seek before
 * @param before_len Length of the before key
 * @param key        Output buffer for the previous key
 * @param klen       Output length of the previous key
 * @return GARRY_TRUE if a previous key exists, GARRY_FALSE otherwise
 */
garry_bool garry_prev_key(garry_database *db, garry_txn txn, const garry_u8 *before,
                          garry_i32 before_len, garry_u8 *key, garry_i32 *klen)
{
    if (!db)
        return 0;
    return garry_storage_prev_key(db->eng, txn, before, before_len, key, klen);
}

/**
 * @brief Check whether a key exists in the database.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Key bytes
 * @param klen  Length of the key
 * @return GARRY_TRUE if the key exists, GARRY_FALSE otherwise
 */
garry_bool garry_exists(garry_database *db, garry_txn txn, const garry_u8 *key, garry_i32 klen)
{
    if (!db)
        return 0;
    return garry_storage_exists(db->eng, txn, key, klen);
}

/**
 * @brief Return the total number of key-value pairs in the database.
 *
 * @param db  Database handle
 * @param txn Transaction ID
 * @return Count of entries, or 0 on failure
 */
garry_i32 garry_count(garry_database *db, garry_txn txn)
{
    if (!db)
        return 0;
    return garry_storage_count(db->eng, txn);
}

/**
 * @brief Open a cursor for iterating over keys matching a prefix.
 *
 * @param db     Database handle
 * @param txn    Transaction ID
 * @param prefix Key prefix to match (NULL for all keys)
 * @param plen   Prefix length (0 if prefix is NULL)
 * @return Cursor handle, or NULL on failure
 */
garry_cursor *garry_cursor_open(garry_database *db, garry_txn txn, const garry_u8 *prefix,
                                garry_i32 plen)
{
    garry_cursor *cur;

    if (!db)
        return NULL;

    cur = (garry_cursor *)malloc(sizeof(garry_cursor));
    if (!cur)
        return NULL;

    cur->scur = garry_storage_cursor_open(db->eng, txn, prefix, plen);
    return cur;
}

/**
 * @brief Advance the cursor and return the next key-value pair.
 *
 * @param cur  Cursor handle
 * @param key  Output buffer for the key
 * @param klen Output length of the key
 * @param value Output buffer for the value
 * @param vlen On input, capacity of value buffer; on output, actual length
 * @return GARRY_TRUE if a next entry exists, GARRY_FALSE at end
 */
garry_bool garry_cursor_next(garry_cursor *cur, garry_u8 *key, garry_i32 *klen, garry_u8 *value,
                             garry_i32 *vlen)
{
    if (!cur)
        return 0;
    return garry_storage_cursor_next(&cur->scur, key, klen, value, vlen);
}

/**
 * @brief Advance the cursor and return only the next key.
 *
 * @param cur  Cursor handle
 * @param key  Output buffer for the key
 * @param klen Output length of the key
 * @return GARRY_TRUE if a next key exists, GARRY_FALSE at end
 */
garry_bool garry_cursor_next_key(garry_cursor *cur, garry_u8 *key, garry_i32 *klen)
{
    if (!cur)
        return 0;
    return garry_storage_cursor_next(&cur->scur, key, klen, NULL, NULL);
}

/**
 * @brief Close a cursor and release its resources.
 *
 * @param cur Cursor handle to close (NULL is a no-op)
 */
void garry_cursor_close(garry_cursor *cur)
{
    if (!cur)
        return;
    garry_storage_cursor_close(&cur->scur);
    free(cur);
}

/**
 * @brief Store a string key-value pair within a transaction.
 *
 * @param db    Database handle
 * @param txn   Transaction ID
 * @param key   Null-terminated key string
 * @param value Null-terminated value string
 * @return GARRY_OK on success, or an error code
 */
garry_status_t garry_set_str(garry_database *db, garry_txn txn, const char *key, const char *value)
{
    garry_i32 klen, vlen;

    if (!db || !key || !value)
        return GARRY_ERR_INVALID_ARG;
    klen = (garry_i32)strlen(key);
    if (klen > GARRY_MAX_KEY_SIZE)
        return GARRY_ERR_BUFFER_TOO_SMALL;
    vlen = (garry_i32)strlen(value);
    if (vlen > GARRY_MAX_RECORD_SIZE)
        return GARRY_ERR_BUFFER_TOO_SMALL;
    return garry_set(db, txn, (const garry_u8 *)key, klen, (const garry_u8 *)value, vlen);
}

/**
 * @brief Retrieve a string value by key within a transaction.
 *
 * @param db         Database handle
 * @param txn        Transaction ID
 * @param key        Null-terminated key string
 * @param value      Output buffer for the null-terminated value
 * @param value_size Size of the value buffer in bytes
 * @return GARRY_OK on success, or an error code
 */
garry_status_t garry_get_str(garry_database *db, garry_txn txn, const char *key, char *value,
                             garry_i32 value_size)
{
    garry_i32 klen, vlen;
    garry_status_t rc;

    if (!db || !key || !value || value_size <= 0)
        return GARRY_ERR_INVALID_ARG;
    klen = (garry_i32)strlen(key);
    if (klen > GARRY_MAX_KEY_SIZE)
        return GARRY_ERR_BUFFER_TOO_SMALL;
    vlen = value_size - 1;
    rc = garry_get(db, txn, (const garry_u8 *)key, klen, (garry_u8 *)value, &vlen);
    if (rc == GARRY_OK)
    {
        if (vlen >= value_size)
            vlen = value_size - 1;
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
void garry_for_each(garry_database *db, garry_txn txn, const garry_u8 *prefix, garry_i32 plen,
                    garry_visitor visitor, void *user_data)
{
    garry_cursor *cur;
    garry_u8 *key;
    garry_u8 *value;
    garry_i32 klen, vlen;

    if (!db || !visitor)
        return;

    key = (garry_u8 *)malloc(GARRY_MAX_KEY_SIZE);
    value = (garry_u8 *)malloc(GARRY_MAX_RECORD_SIZE);
    if (!key || !value)
    {
        free(key);
        free(value);
        return;
    }

    cur = garry_cursor_open(db, txn, prefix, plen);
    if (!cur)
    {
        free(key);
        free(value);
        return;
    }

    while (garry_cursor_next(cur, key, &klen, value, &vlen))
    {
        visitor(key, klen, value, vlen, user_data);
    }

    garry_cursor_close(cur);
    free(key);
    free(value);
}
