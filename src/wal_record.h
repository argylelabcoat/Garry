/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file wal_record.h
 * @brief WAL record types and structures.
 */

#ifndef GARRY_WAL_RECORD_H
#define GARRY_WAL_RECORD_H

#include "garry/types.h"
#include "storage_types.h"
#include "buffer_pool.h"

typedef enum
{
    GARRY_WAL_UPDATE = 0,
    GARRY_WAL_COMMIT = 1,
    GARRY_WAL_ABORT = 2,
    GARRY_WAL_CHECKPOINT = 3,
    GARRY_WAL_DELETE = 4
} garry_wal_record_kind;

typedef struct
{
    garry_wal_record_kind kind;
    garry_txn_id txid;
    garry_byte_array key;
    garry_i32 key_len;
    garry_i32 value_len;
    garry_byte_array old_data;
    garry_byte_array new_data;
    /* When new_is_overflow is set, the real value (longer than a single
     * garry_byte_array) was written to an overflow page chain via
     * garry_overflow_write(); new_data is unused and new_overflow_head
     * is the chain's head page ID. Recovery must reassemble the value
     * via garry_overflow_read() before applying it. */
    garry_bool new_is_overflow;
    garry_i32 new_overflow_head;
} garry_wal_record;

/**
 * @brief Create an update WAL record.
 *
 * Heap-allocates a WAL record of kind GARRY_WAL_UPDATE, copying the
 * key and new value into the record's internal buffers. Values larger
 * than a single garry_byte_array are written to an overflow page chain
 * (see version_chain.c) instead of being inlined; the record stores a
 * reference to that chain.
 *
 * @param pool    Buffer pool to allocate overflow pages from, if needed
 * @param txn     Transaction ID that owns this update
 * @param key     Key bytes for the update
 * @param klen    Key length in bytes
 * @param new_val New value bytes
 * @param vlen    New value length in bytes
 * @return Heap-allocated WAL record, or NULL on allocation failure
 */
garry_wal_record *garry_make_update_record(garry_buffer_pool *pool, garry_txn_id txn,
                                           const garry_byte *key, garry_i32 klen,
                                           const garry_byte *new_val, garry_i32 vlen);

/**
 * @brief Create a delete WAL record.
 *
 * Heap-allocates a WAL record of kind GARRY_WAL_DELETE, copying the
 * key into the record's internal buffer. garry_storage_delete()
 * previously applied deletes only to the in-memory/buffer-pool state
 * with no WAL record at all, so a deleted key silently reappeared on
 * WAL-recovered crash restart (recovery would replay the older
 * GARRY_WAL_UPDATE record for that key with no way to know it had
 * since been deleted).
 *
 * @param txn   Transaction ID performing the delete
 * @param key   Key bytes to delete
 * @param klen  Key length in bytes
 * @return Heap-allocated WAL record, or NULL on allocation failure
 */
garry_wal_record *garry_make_delete_record(garry_txn_id txn, const garry_byte *key,
                                           garry_i32 klen);

/**
 * @brief Create a commit WAL record.
 *
 * Heap-allocates a WAL record of kind GARRY_WAL_COMMIT for the given
 * transaction.
 *
 * @param txn  Transaction ID being committed
 * @return Heap-allocated WAL record, or NULL on allocation failure
 */
garry_wal_record *garry_make_commit_record(garry_txn_id txn);

/**
 * @brief Create an abort WAL record.
 *
 * Heap-allocates a WAL record of kind GARRY_WAL_ABORT for the given
 * transaction.
 *
 * @param txn  Transaction ID being aborted
 * @return Heap-allocated WAL record, or NULL on allocation failure
 */
garry_wal_record *garry_make_abort_record(garry_txn_id txn);

/**
 * @brief Create a checkpoint WAL record.
 *
 * Heap-allocates a WAL record of kind GARRY_WAL_CHECKPOINT for the
 * given transaction.
 *
 * @param txn  Transaction ID associated with the checkpoint
 * @return Heap-allocated WAL record, or NULL on allocation failure
 */
garry_wal_record *garry_make_checkpoint_record(garry_txn_id txn);

/**
 * @brief Free a heap-allocated WAL record.
 *
 * @param rec  WAL record to free (may be NULL)
 */
void garry_wal_record_free(garry_wal_record *rec);

/* WAL record on-disk format constants. */
#define GARRY_WAL_RECORD_SIZE 792
#define WAL_REC_KIND_OFF      0
#define WAL_REC_TXID_OFF      4
#define WAL_REC_KLEN_OFF      8
#define WAL_REC_VLEN_OFF      12
#define WAL_REC_KEY_OFF       16
#define WAL_REC_OLD_OFF       272
#define WAL_REC_NEW_OFF       528
#define WAL_REC_NEW_OVERFLOW_FLAG_OFF 784
#define WAL_REC_NEW_OVERFLOW_HEAD_OFF 788

#endif /* GARRY_WAL_RECORD_H */
