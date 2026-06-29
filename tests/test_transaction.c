/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "transaction.h"
#include "test_helpers.h"
#include <string.h>
#include <stdio.h>

#ifndef GARRY_TRUE
#define GARRY_TRUE 1
#endif

static void test_chain_id_roundtrip(void)
{
    garry_byte buf[4];
    garry_i32 encoded;
    garry_i32 decoded;

    encoded = garry_chain_id_encode(42, buf);
    GARRY_CHECK(encoded == 4);
    decoded = garry_chain_id_decode(buf);
    GARRY_CHECK(decoded == 42);
}

static void test_mvcc_begin_increments_txid(void)
{
    garry_engine_settings settings;
    garry_engine_handle *eng;
    garry_txn_id t1, t2;

    settings = garry_default_engine_settings();
    eng = garry_engine_init("/tmp/garry_txn_test_1", settings);
    GARRY_CHECK(eng != NULL);

    t1 = garry_mvcc_begin(eng);
    t2 = garry_mvcc_begin(eng);

    GARRY_CHECK(t1 == 1);
    GARRY_CHECK(t2 == 2);

    garry_mvcc_rollback(eng, t2);
    garry_mvcc_rollback(eng, t1);
    garry_engine_close(eng);
    remove("/tmp/garry_txn_test_1");
    remove("/tmp/garry_txn_test_1.wal");
    remove("/tmp/garry_txn_test_1.ckpt");
}

static void test_mvcc_commit_marks_state(void)
{
    garry_engine_settings settings;
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_i32 slot;

    settings = garry_default_engine_settings();
    eng = garry_engine_init("/tmp/garry_txn_test_2", settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_mvcc_begin(eng);
    GARRY_CHECK(txn == 1);

    slot = -1;
    {
        garry_i32 i;
        for (i = 0; i < eng->active_count; i++) {
            if (eng->active_txns[i] == txn) slot = i;
        }
    }
    GARRY_CHECK(slot >= 0);
    GARRY_CHECK(eng->txn_states[slot].state == GARRY_TXN_ACTIVE);

    garry_mvcc_commit(eng, txn);

    GARRY_CHECK(garry_txn_is_active(txn, eng) == 0);

    garry_engine_close(eng);
    remove("/tmp/garry_txn_test_2");
    remove("/tmp/garry_txn_test_2.wal");
    remove("/tmp/garry_txn_test_2.ckpt");
}

static void test_mvcc_rollback_marks_state(void)
{
    garry_engine_settings settings;
    garry_engine_handle *eng;
    garry_txn_id txn;

    settings = garry_default_engine_settings();
    eng = garry_engine_init("/tmp/garry_txn_test_3", settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_mvcc_begin(eng);
    GARRY_CHECK(txn == 1);

    garry_mvcc_rollback(eng, txn);

    GARRY_CHECK(garry_txn_is_active(txn, eng) == 0);

    garry_engine_close(eng);
    remove("/tmp/garry_txn_test_3");
    remove("/tmp/garry_txn_test_3.wal");
    remove("/tmp/garry_txn_test_3.ckpt");
}

static void test_txn_is_active_check(void)
{
    garry_engine_settings settings;
    garry_engine_handle *eng;
    garry_txn_id txn;

    settings = garry_default_engine_settings();
    eng = garry_engine_init("/tmp/garry_txn_test_4", settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_mvcc_begin(eng);
    GARRY_CHECK(garry_txn_is_active(txn, eng) == GARRY_TRUE);

    garry_mvcc_commit(eng, txn);
    GARRY_CHECK(garry_txn_is_active(txn, eng) == 0);

    garry_engine_close(eng);
    remove("/tmp/garry_txn_test_4");
    remove("/tmp/garry_txn_test_4.wal");
    remove("/tmp/garry_txn_test_4.ckpt");
}

int main(void)
{
    test_chain_id_roundtrip();
    test_mvcc_begin_increments_txid();
    test_mvcc_commit_marks_state();
    test_mvcc_rollback_marks_state();
    test_txn_is_active_check();

    if (garry_test_failures == 0) printf("test_transaction: ALL PASSED\n");
    return garry_test_failures;
}
