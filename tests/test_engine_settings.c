/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "engine_settings.h"
#include "test_helpers.h"

int main(void)
{
    garry_engine_settings s;
    s = garry_default_engine_settings();
    GARRY_CHECK(s.page_size == 4096);
    GARRY_CHECK(s.compression == 0);
    GARRY_CHECK(s.max_txns == 4);
    GARRY_CHECK(s.max_versions == 64);
    GARRY_CHECK(s.max_key_size == 256);
    GARRY_CHECK(s.max_subscripts == 16);
    GARRY_CHECK(s.btree_flags == 0);
    GARRY_CHECK(GARRY_MAX_TXNS_CAPACITY == 64);
    GARRY_CHECK(GARRY_MAX_VERSIONS_CAPACITY == 256);
    GARRY_CHECK(GARRY_MAX_KEY_SIZE_CAPACITY == 256);
    GARRY_CHECK(GARRY_MAX_SUBSCRIPTS_CAPACITY == 32);
    GARRY_CHECK(GARRY_COMPRESS_NONE == 0);
    GARRY_CHECK(GARRY_COMPRESS_LZ4 == 1);
    GARRY_CHECK(GARRY_BTREE_FLAG_COMPRESS_KEYS == 1);
    return garry_test_failures;
}
