/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "lock_table.h"
#include "test_helpers.h"

int main(void)
{
    garry_lock_manager mgr;
    garry_txn_id txn1, txn2;
    garry_bool ok;
    garry_byte_array key;

    ENCODE_KEY(key, "k");
    mgr = garry_create_lock_manager();
    txn1 = 1;
    txn2 = 2;

    /* Shared + shared should succeed. */
    garry_lock_acquire(&mgr, txn1, key, 1, GARRY_LOCK_SHARED, &ok);
    GARRY_CHECK(ok == 1);

    garry_lock_acquire(&mgr, txn2, key, 1, GARRY_LOCK_SHARED, &ok);
    GARRY_CHECK(ok == 1);

    /* Reset. Exclusive blocks shared. */
    mgr = garry_create_lock_manager();

    garry_lock_acquire(&mgr, txn1, key, 1, GARRY_LOCK_EXCLUSIVE, &ok);
    GARRY_CHECK(ok == 1);

    garry_lock_acquire(&mgr, txn2, key, 1, GARRY_LOCK_SHARED, &ok);
    GARRY_CHECK(ok == 0);

    /* Release exclusive, shared should now succeed. */
    garry_lock_release(&mgr, txn1);

    garry_lock_acquire(&mgr, txn2, key, 1, GARRY_LOCK_SHARED, &ok);
    GARRY_CHECK(ok == 1);

    return garry_test_failures;
}
