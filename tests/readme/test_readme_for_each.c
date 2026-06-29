/*
 * Verifies the "Callback Iteration" example from README.md.
 */
#include "garry/api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_readme_foreach.db"

static void cleanup(void)
{
    remove(TEST_DB);
}

static int visit_count;

/* --- README example --- */
static void my_visitor(const garry_u8 *key, garry_i32 klen, const garry_u8 *val, garry_i32 vlen,
                       void *ud)
{
    (void)key;
    (void)klen;
    (void)val;
    (void)vlen;
    (void)ud;
    visit_count++;
}
/* --- end README example --- */

int main(void)
{
    garry_database *db;
    garry_txn txn;

    cleanup();
    visit_count = 0;

    /* Seed data */
    db = garry_database_create(TEST_DB);
    txn = garry_txn_begin(db);
    garry_set_str(db, txn, "x", "1");
    garry_set_str(db, txn, "y", "2");
    garry_set_str(db, txn, "z", "3");
    garry_txn_commit(db, txn);

    /* --- README example --- */
    txn = garry_txn_begin(db);
    garry_for_each(db, txn, NULL, 0, my_visitor, NULL);
    garry_txn_rollback(db, txn);
    /* --- end README example --- */

    garry_database_close(db);

    if (visit_count != 3)
    {
        printf("FAIL: visitor called %d times, expected 3\n", visit_count);
        cleanup();
        return 1;
    }

    printf("test_readme_for_each: PASSED\n");
    cleanup();
    return 0;
}
