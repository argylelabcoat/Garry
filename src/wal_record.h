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

typedef enum
{
    GARRY_WAL_UPDATE = 0,
    GARRY_WAL_COMMIT = 1,
    GARRY_WAL_ABORT = 2,
    GARRY_WAL_CHECKPOINT = 3
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
} garry_wal_record;

/**
 * @brief Create an update WAL record.
 *
 * Heap-allocates a WAL record of kind GARRY_WAL_UPDATE, copying the
 * key and new value into the record's internal buffers.
 *
 * @param txn     Transaction ID that owns this update
 * @param key     Key bytes for the update
 * @param klen    Key length in bytes
 * @param new_val New value bytes
 * @param vlen    New value length in bytes
 * @return Heap-allocated WAL record, or NULL on allocation failure
 */
garry_wal_record *garry_make_update_record(garry_txn_id txn, const garry_byte *key, garry_i32 klen,
                                           const garry_byte *new_val, garry_i32 vlen);

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
#define GARRY_WAL_RECORD_SIZE 784
#define WAL_REC_KIND_OFF      0
#define WAL_REC_TXID_OFF      4
#define WAL_REC_KLEN_OFF      8
#define WAL_REC_VLEN_OFF      12
#define WAL_REC_KEY_OFF       16
#define WAL_REC_OLD_OFF       272
#define WAL_REC_NEW_OFF       528

#endif /* GARRY_WAL_RECORD_H */
