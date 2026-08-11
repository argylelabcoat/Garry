/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file wal_record.c
 * @brief WAL record lifecycle (allocation and deallocation).
 *
 * Provides heap allocation for WAL records that own their value
 * buffers, and a destructor that frees the owned data.
 */

#include "wal_record.h"
#include "version_chain.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a WAL UPDATE record.
 *
 * Allocates and initializes a WAL record with kind GARRY_WAL_UPDATE,
 * copying the key and new value into the record's fixed-size buffers.
 *
 * @param txn     Transaction ID performing the update.
 * @param key     Pointer to the key bytes.
 * @param klen    Length of the key in bytes.
 * @param new_val Pointer to the new value bytes.
 * @param vlen    Length of the new value in bytes.
 *
 * @return Pointer to the allocated record, or NULL on allocation failure.
 */
garry_wal_record *garry_make_update_record(garry_buffer_pool *pool, garry_txn_id txn,
                                           const garry_byte *key, garry_i32 klen,
                                           const garry_byte *new_val, garry_i32 vlen)
{
    garry_wal_record *rec = (garry_wal_record *)malloc(sizeof(garry_wal_record));
    if (rec == NULL)
        return NULL;
    memset(rec, 0, sizeof(*rec));
    rec->kind = GARRY_WAL_UPDATE;
    rec->txid = txn;
    if (klen > 0 && key != NULL)
    {
        memcpy(rec->key, key, (size_t)klen);
    }
    rec->key_len = klen;
    rec->value_len = vlen;
    if (vlen > 0 && new_val != NULL)
    {
        if ((size_t)vlen <= sizeof(garry_byte_array))
        {
            memcpy(rec->new_data, new_val, (size_t)vlen);
        }
        else
        {
            garry_i32 head = garry_overflow_write(pool, (const char *)new_val, vlen);
            if (head < 0)
            {
                free(rec);
                return NULL;
            }
            rec->new_is_overflow = GARRY_TRUE;
            rec->new_overflow_head = head;
        }
    }
    return rec;
}

/**
 * @brief Create a WAL DELETE record.
 *
 * @param txn   Transaction ID performing the delete.
 * @param key   Pointer to the key bytes.
 * @param klen  Length of the key in bytes.
 *
 * @return Pointer to the allocated record, or NULL on allocation failure.
 */
garry_wal_record *garry_make_delete_record(garry_txn_id txn, const garry_byte *key,
                                           garry_i32 klen)
{
    garry_wal_record *rec = (garry_wal_record *)malloc(sizeof(garry_wal_record));
    if (rec == NULL)
        return NULL;
    memset(rec, 0, sizeof(*rec));
    rec->kind = GARRY_WAL_DELETE;
    rec->txid = txn;
    if (klen > 0 && key != NULL)
    {
        memcpy(rec->key, key, (size_t)klen);
    }
    rec->key_len = klen;
    rec->value_len = 0;
    return rec;
}

/**
 * @brief Create a WAL COMMIT record.
 *
 * @param txn Transaction ID being committed.
 *
 * @return Pointer to the allocated record, or NULL on allocation failure.
 */
garry_wal_record *garry_make_commit_record(garry_txn_id txn)
{
    garry_wal_record *rec = (garry_wal_record *)malloc(sizeof(garry_wal_record));
    if (rec == NULL)
        return NULL;
    memset(rec, 0, sizeof(*rec));
    rec->kind = GARRY_WAL_COMMIT;
    rec->txid = txn;
    return rec;
}

/**
 * @brief Create a WAL ABORT record.
 *
 * @param txn Transaction ID being aborted.
 *
 * @return Pointer to the allocated record, or NULL on allocation failure.
 */
garry_wal_record *garry_make_abort_record(garry_txn_id txn)
{
    garry_wal_record *rec = (garry_wal_record *)malloc(sizeof(garry_wal_record));
    if (rec == NULL)
        return NULL;
    memset(rec, 0, sizeof(*rec));
    rec->kind = GARRY_WAL_ABORT;
    rec->txid = txn;
    return rec;
}

/**
 * @brief Create a WAL CHECKPOINT record.
 *
 * @param txn Transaction ID associated with the checkpoint.
 *
 * @return Pointer to the allocated record, or NULL on allocation failure.
 */
garry_wal_record *garry_make_checkpoint_record(garry_txn_id txn)
{
    garry_wal_record *rec = (garry_wal_record *)malloc(sizeof(garry_wal_record));
    if (rec == NULL)
        return NULL;
    memset(rec, 0, sizeof(*rec));
    rec->kind = GARRY_WAL_CHECKPOINT;
    rec->txid = txn;
    return rec;
}

/**
 * @brief Free a WAL record.
 *
 * @param rec Pointer to the record to free (may be NULL).
 */
void garry_wal_record_free(garry_wal_record *rec)
{
    free(rec);
}
