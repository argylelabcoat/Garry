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

/* MAKE_UPDATE_RECORD(txn, key, klen, new_val, vlen) RET wal_record_opt */
garry_wal_record *garry_make_update_record(garry_txn_id txn, const garry_byte *key, garry_i32 klen,
                                           const garry_byte *new_val, garry_i32 vlen);

/* MAKE_COMMIT_RECORD(txn) RET wal_record_opt */
garry_wal_record *garry_make_commit_record(garry_txn_id txn);

/* MAKE_ABORT_RECORD(txn) RET wal_record_opt */
garry_wal_record *garry_make_abort_record(garry_txn_id txn);

/* MAKE_CHECKPOINT_RECORD(txn) RET wal_record_opt */
garry_wal_record *garry_make_checkpoint_record(garry_txn_id txn);

/* Free a heap-allocated WAL record. */
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
