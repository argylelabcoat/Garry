/*
 * Verifies the "Hierarchical Keys" example from README.md.
 */
#include "garry/api.h"
#include "garry/keysplit.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_readme_keysplit.db"

static void cleanup(void)
{
    remove(TEST_DB);
}

int main(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key;
    garry_i32 n;
    garry_u8 result[256];
    garry_i32 vlen;
    char buf[256];

    cleanup();

    /* --- README example --- */
    /* Split path into length-prefixed key */
    n = garry_key_split("users/matthew/articles/A04", '/', key);

    /* Use with set/get */
    db = garry_database_create(TEST_DB);
    txn = garry_txn_begin(db);
    garry_set(db, txn, key, n, (const garry_u8 *)"test-value", 10);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    vlen = 0;
    garry_get(db, txn, key, n, result, &vlen);
    garry_txn_rollback(db, txn);

    /* Unsplit back to string */
    garry_key_unsplit(key, n, '/', buf, sizeof(buf));
    /* buf = "users/matthew/articles/A04" */
    /* --- end README example --- */

    garry_database_close(db);

    if (n == 0)
    {
        printf("FAIL: garry_key_split returned 0\n");
        cleanup();
        return 1;
    }
    if (vlen != 10 || memcmp(result, "test-value", 10) != 0)
    {
        printf("FAIL: round-trip value mismatch\n");
        cleanup();
        return 1;
    }
    if (strcmp(buf, "users/matthew/articles/A04") != 0)
    {
        printf("FAIL: unsplit got \"%s\", expected \"users/matthew/articles/A04\"\n", buf);
        cleanup();
        return 1;
    }

    printf("test_readme_keysplit: PASSED\n");
    cleanup();
    return 0;
}
