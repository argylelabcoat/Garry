/*
 * Verifies the "Navigation" example from README.md.
 */
#include "garry/api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_readme_nav.db"

static void cleanup(void) { remove(TEST_DB); }

int main(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[GARRY_MAX_KEY_SIZE];
    garry_i32 klen;
    garry_i32 count;
    garry_bool found;

    cleanup();

    db = garry_database_create(TEST_DB);
    txn = garry_txn_begin(db);
    garry_set_str(db, txn, "aaa", "1");
    garry_set_str(db, txn, "bbb", "2");
    garry_set_str(db, txn, "ccc", "3");
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    klen = 0;
    found = garry_first(db, txn, key, &klen);
    printf("first: found=%d klen=%d\n", found, klen);

    memset(key, 0, sizeof(key));
    klen = 0;
    found = garry_last(db, txn, key, &klen);
    printf("last: found=%d klen=%d\n", found, klen);

    found = garry_next_key(db, txn, (const garry_u8*)"aaa", 3, key, &klen);
    printf("next(aaa): found=%d klen=%d\n", found, klen);

    found = garry_exists(db, txn, key, klen);
    printf("exists: found=%d\n", found);

    count = garry_count(db, txn);
    printf("count: %d\n", count);

    garry_txn_rollback(db, txn);
    garry_database_close(db);

    if (count != 3) {
        printf("FAIL\n");
        cleanup();
        return 1;
    }
    printf("test_readme_navigation: PASSED\n");
    cleanup();
    return 0;
}
