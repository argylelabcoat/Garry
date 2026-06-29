/*
 * Verifies the "Basic Key-Value Operations" example from README.md.
 */
#include "garry/api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_readme_basic.db"

static void cleanup(void)
{
    remove(TEST_DB);
}

int main(void)
{
    garry_database *db;
    garry_txn txn;
    char result[256];

    cleanup();

    /* --- README example (adapted to C89) --- */
    db = garry_database_create(TEST_DB);
    txn = garry_txn_begin(db);

    garry_set_str(db, txn, "hello", "world");
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    garry_get_str(db, txn, "hello", result, sizeof(result));
    /* result = "world" */
    garry_txn_rollback(db, txn);
    garry_database_close(db);
    /* --- end README example --- */

    if (strcmp(result, "world") != 0)
    {
        printf("FAIL: expected \"world\", got \"%s\"\n", result);
        cleanup();
        return 1;
    }

    printf("test_readme_basic: PASSED\n");
    cleanup();
    return 0;
}
