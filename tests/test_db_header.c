/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "db_header.h"
#include "engine_settings.h"
#include "test_helpers.h"

int main(void)
{
    garry_engine_settings settings;
    garry_db_header hdr, hdr2;
    garry_byte buf[GARRY_MAX_PAGE_SIZE];
    garry_bool valid;

    settings = garry_default_engine_settings();
    hdr = garry_create_db_header(&settings);
    GARRY_CHECK(hdr.magic == GARRY_DB_MAGIC);
    GARRY_CHECK(hdr.version == 2);
    GARRY_CHECK(hdr.page_size == 4096);
    GARRY_CHECK(hdr.root_page == 1);
    GARRY_CHECK(hdr.free_list_head == -1);
    GARRY_CHECK(hdr.total_pages == 1);
    GARRY_CHECK(hdr.max_txns == 4);
    GARRY_CHECK(hdr.max_versions == 64);
    GARRY_CHECK(hdr.max_key_size == 256);
    GARRY_CHECK(hdr.max_subscripts == 16);
    GARRY_CHECK(hdr.btree_flags == 0);

    valid = garry_validate_db_header(&hdr);
    GARRY_CHECK(valid == 1);

    memset(buf, 0, sizeof(buf));
    garry_write_db_header(buf, &hdr);
    hdr2 = garry_read_db_header(buf);
    GARRY_CHECK(hdr2.magic == hdr.magic);
    GARRY_CHECK(hdr2.version == hdr.version);
    GARRY_CHECK(hdr2.page_size == hdr.page_size);
    GARRY_CHECK(hdr2.max_txns == hdr.max_txns);
    GARRY_CHECK(hdr2.max_versions == hdr.max_versions);

    hdr2.magic = 0;
    valid = garry_validate_db_header(&hdr2);
    GARRY_CHECK(valid == 0);

    settings.page_size = 128;
    hdr = garry_create_db_header(&settings);
    GARRY_CHECK(hdr.magic == 0);

    return garry_test_failures;
}
