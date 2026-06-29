/*
 * Verifies the "Cursor Iteration" example from README.md.
 */
#include "garry/api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_readme_cursor.db"

static void cleanup(void) { remove(TEST_DB); }

int main(void)
{
    garry_database *db;
    garry_txn txn;
    garry_cursor *cur;
    garry_u8 key[GARRY_MAX_KEY_SIZE], val[GARRY_MAX_KEY_SIZE];
    garry_i32 klen, vlen;
    int count_keys = 0;
    int count_keysonly = 0;

    cleanup();

    /* Seed some data */
    db = garry_database_create(TEST_DB);
    txn = garry_txn_begin(db);
    garry_set_str(db, txn, "a/1", "alpha");
    garry_set_str(db, txn, "a/2", "bravo");
    garry_set_str(db, txn, "b/1", "charlie");
    garry_txn_commit(db, txn);

    /* --- README example (cursor_next) --- */
    txn = garry_txn_begin(db);
    cur = garry_cursor_open(db, txn, NULL, 0);
    while (garry_cursor_next(cur, key, &klen, val, &vlen)) {
        count_keys++;
    }
    garry_cursor_close(cur);
    /* --- end README example --- */

    /* --- README example (cursor_next_key) --- */
    cur = garry_cursor_open(db, txn, NULL, 0);
    while (garry_cursor_next_key(cur, key, &klen)) {
        count_keysonly++;
    }
    garry_cursor_close(cur);
    /* --- end README example --- */

    garry_txn_rollback(db, txn);
    garry_database_close(db);

    if (count_keys != 3) {
        printf("FAIL: cursor_next saw %d keys, expected 3\n", count_keys);
        cleanup();
        return 1;
    }
    if (count_keysonly != 3) {
        printf("FAIL: cursor_next_key saw %d keys, expected 3\n", count_keysonly);
        cleanup();
        return 1;
    }

    printf("test_readme_cursor: PASSED\n");
    cleanup();
    return 0;
}
