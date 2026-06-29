/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "storage_core.h"
#include "test_helpers.h"

int main(void)
{
    garry_page_header hdr;
    GARRY_CHECK(GARRY_MAX_PAGE_SIZE == 65536);
    GARRY_CHECK(garry_page_header_size() == 32);
    GARRY_CHECK(GARRY_NODE_INTERNAL == 0);
    GARRY_CHECK(GARRY_NODE_LEAF == 1);
    GARRY_CHECK(GARRY_NODE_OVERFLOW == 2);
    GARRY_CHECK(GARRY_NODE_FREE == 3);
    GARRY_CHECK(GARRY_NODE_CHAIN == 4);
    hdr = garry_create_header(GARRY_NODE_LEAF, 0);
    GARRY_CHECK(hdr.page_type == GARRY_NODE_LEAF);
    GARRY_CHECK(hdr.page_level == 0);
    GARRY_CHECK(hdr.num_records == 0);
    GARRY_CHECK(hdr.free_start == 32);
    GARRY_CHECK(hdr.free_end == 65536);
    GARRY_CHECK(hdr.compression == 0);
    GARRY_CHECK(hdr.parent_page == 0);
    GARRY_CHECK(hdr.checksum == 0);
    return garry_test_failures;
}
