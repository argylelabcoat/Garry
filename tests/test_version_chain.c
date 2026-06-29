/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "version_chain.h"
#include "storage_core.h"
#include "slotted_page.h"
#include "buffer_pool.h"
#include "test_helpers.h"
#include <stdlib.h>
#include <string.h>

#ifndef GARRY_TRUE
#define GARRY_TRUE 1
#endif

static void test_empty_page_returns_nil(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *result;
    garry_txn_id active[4];

    garry_chain_page_init(buf, 4096);
    active[0] = 10;
    result = garry_chain_page_find_visible(NULL, buf, 4096, 5, active, 1, &vlen);
    GARRY_CHECK(result == NULL);
}

static void test_append_visible_to_later_txn(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *result;
    garry_txn_id active[4];
    garry_bool ok;

    garry_chain_page_init(buf, 4096);
    ok = garry_chain_page_append(buf, 4096, 1, "hello", 5);
    GARRY_CHECK(ok == GARRY_TRUE);

    active[0] = 10;
    result = garry_chain_page_find_visible(NULL, buf, 4096, 2, active, 1, &vlen);
    GARRY_CHECK(result != NULL);
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(result[0] == 'h');
    GARRY_CHECK(result[1] == 'e');
    GARRY_CHECK(result[2] == 'l');
    GARRY_CHECK(result[3] == 'l');
    GARRY_CHECK(result[4] == 'o');
    free(result);
}

static void test_not_visible_to_earlier_txn(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *result;
    garry_txn_id active[4];

    garry_chain_page_init(buf, 4096);
    garry_chain_page_append(buf, 4096, 3, "secret", 6);

    active[0] = 10;
    result = garry_chain_page_find_visible(NULL, buf, 4096, 2, active, 1, &vlen);
    GARRY_CHECK(result == NULL);
}

static void test_not_visible_when_txn_is_active(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *result;
    garry_txn_id active[4];

    garry_chain_page_init(buf, 4096);
    garry_chain_page_append(buf, 4096, 1, "uncommitted", 11);

    active[0] = 1;
    active[1] = 5;
    result = garry_chain_page_find_visible(NULL, buf, 4096, 2, active, 2, &vlen);
    GARRY_CHECK(result == NULL);
}

static void test_tombstone_hides_value(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *result;
    garry_txn_id active[4];
    garry_bool ok;

    garry_chain_page_init(buf, 4096);
    ok = garry_chain_page_append(buf, 4096, 1, "visible", 7);
    GARRY_CHECK(ok == GARRY_TRUE);
    ok = garry_chain_page_append_tombstone(buf, 4096, 2);
    GARRY_CHECK(ok == GARRY_TRUE);

    active[0] = 10;
    result = garry_chain_page_find_visible(NULL, buf, 4096, 3, active, 1, &vlen);
    GARRY_CHECK(result == NULL);
}

static void test_tombstone_does_not_affect_earlier_snapshots(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *result;
    garry_txn_id active[4];

    garry_chain_page_init(buf, 4096);
    garry_chain_page_append(buf, 4096, 1, "old_value", 9);
    garry_chain_page_append_tombstone(buf, 4096, 3);

    active[0] = 10;
    result = garry_chain_page_find_visible(NULL, buf, 4096, 2, active, 1, &vlen);
    GARRY_CHECK(result != NULL);
    GARRY_CHECK(vlen == 9);
    GARRY_CHECK(result[0] == 'o');
    GARRY_CHECK(result[8] == 'e');
    free(result);
}

static void test_page_holds_66_records(void)
{
    garry_page_buffer buf;
    garry_i32 idx;
    garry_i32 count;
    garry_i32 i;
    garry_byte data[512];

    garry_chain_page_init(buf, 4096);
    memset(data, 'A', sizeof(data));

    for (i = 0; i < 66; i++) {
        data[0] = (garry_byte)(i & 0xFF);
        idx = garry_chain_page_append(buf, 4096, (garry_txn_id)(i + 1),
                                      (const char*)data, 10);
        GARRY_CHECK(idx == GARRY_TRUE);
    }

    count = garry_page_record_count(&buf);
    GARRY_CHECK(count == 66);
}

int main(void)
{
    test_empty_page_returns_nil();
    test_append_visible_to_later_txn();
    test_not_visible_to_earlier_txn();
    test_not_visible_when_txn_is_active();
    test_tombstone_hides_value();
    test_tombstone_does_not_affect_earlier_snapshots();
    test_page_holds_66_records();

    if (garry_test_failures == 0) printf("test_version_chain: ALL PASSED\n");
    return garry_test_failures;
}
