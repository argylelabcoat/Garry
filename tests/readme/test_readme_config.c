/*
 * Verifies the "Custom Configuration" example from README.md.
 */
#include "garry/api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_readme_config.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

int main(void)
{
    garry_config cfg;
    garry_database *db;
    garry_txn txn;
    char result[256];

    cleanup();

    /* --- README example --- */
    cfg = garry_config_default();
    cfg.pool_size = 4;
    cfg.compression = GARRY_COMPRESS_LZ4;
    db = garry_database_create_with_config(TEST_DB, cfg);
    /* --- end README example --- */

    /* Verify the database works with custom config */
    txn = garry_txn_begin(db);
    garry_set_str(db, txn, "key", "value");
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    garry_get_str(db, txn, "key", result, sizeof(result));
    garry_txn_rollback(db, txn);

    garry_database_close(db);

    if (strcmp(result, "value") != 0) {
        printf("FAIL: expected \"value\", got \"%s\"\n", result);
        cleanup();
        return 1;
    }

    printf("test_readme_config: PASSED\n");
    cleanup();
    return 0;
}
