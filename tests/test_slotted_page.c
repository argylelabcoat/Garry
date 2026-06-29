/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "slotted_page.h"
#include "key_encoding.h"
#include "test_helpers.h"

#define GARRY_LEAF_NODE GARRY_NODE_LEAF

int main(void)
{
    garry_page_buffer buf;
    garry_slot_entry s, s2;
    garry_byte_array data, out_data, src;
    garry_i32 idx, rlen, rc, i;

    s.offset = 1000;
    s.length = 256;
    garry_write_slot(buf, 0, s);
    s2 = garry_read_slot(&buf, 0);
    GARRY_CHECK(s2.offset == 1000);
    GARRY_CHECK(s2.length == 256);

    s.offset = 65535;
    s.length = 32768;
    garry_write_slot(buf, 1, s);
    s2 = garry_read_slot(&buf, 1);
    GARRY_CHECK(s2.offset == 65535);
    GARRY_CHECK(s2.length == 32768);

    s.offset = 0;
    s.length = 1;
    garry_write_slot(buf, 2, s);
    s2 = garry_read_slot(&buf, 2);
    GARRY_CHECK(s2.offset == 0);
    GARRY_CHECK(s2.length == 1);

    garry_page_init(buf, GARRY_LEAF_NODE, 0, 4096);
    rc = garry_page_record_count(&buf);
    GARRY_CHECK(rc == 0);

    memset(data, 0, sizeof(data));
    data[0] = 65;
    data[1] = 66;
    data[2] = 67;
    idx = garry_page_insert(buf, data, 3, 4096);
    GARRY_CHECK(idx == 0);

    rc = garry_page_record_count(&buf);
    GARRY_CHECK(rc == 1);

    rlen = garry_page_get(&buf, 0, out_data, 4096);
    GARRY_CHECK(rlen == 3);
    GARRY_CHECK(out_data[0] == 65);
    GARRY_CHECK(out_data[1] == 66);
    GARRY_CHECK(out_data[2] == 67);

    data[0] = 88;
    data[1] = 89;
    idx = garry_page_insert(buf, data, 2, 4096);
    GARRY_CHECK(idx == 1);

    rc = garry_page_record_count(&buf);
    GARRY_CHECK(rc == 2);

    rlen = garry_page_get(&buf, 1, out_data, 4096);
    GARRY_CHECK(rlen == 2);
    GARRY_CHECK(out_data[0] == 88);
    GARRY_CHECK(out_data[1] == 89);

    garry_page_init(buf, GARRY_LEAF_NODE, 0, 4096);
    for (i = 0; i < 10; i++)
    {
        data[0] = (garry_byte)(i + 1);
        data[1] = (garry_byte)(i + 2);
        idx = garry_page_insert(buf, data, 2, 4096);
        GARRY_CHECK(idx == i);
    }

    for (i = 0; i < 10; i++)
    {
        rlen = garry_page_get(&buf, i, out_data, 4096);
        GARRY_CHECK(rlen == 2);
        GARRY_CHECK(out_data[0] == (garry_byte)(i + 1));
        GARRY_CHECK(out_data[1] == (garry_byte)(i + 2));
    }

    garry_page_init(buf, GARRY_LEAF_NODE, 0, 512);
    memset(data, 65, sizeof(data));
    idx = garry_page_insert(buf, data, 100, 512);
    GARRY_CHECK(idx >= 0);
    idx = garry_page_insert(buf, data, 100, 512);
    GARRY_CHECK(idx >= 0);
    idx = garry_page_insert(buf, data, 100, 512);
    GARRY_CHECK(idx >= 0);
    idx = garry_page_insert(buf, data, 100, 512);
    GARRY_CHECK(idx >= 0);
    idx = garry_page_insert(buf, data, 100, 512);
    GARRY_CHECK(idx == -1);

    garry_page_init(buf, GARRY_LEAF_NODE, 0, 4096);
    src[0] = 10;
    src[1] = 20;
    src[2] = 30;
    garry_copy_bytes_in(buf, 100, src, 3);
    GARRY_CHECK(buf[100] == 10);
    GARRY_CHECK(buf[101] == 20);
    GARRY_CHECK(buf[102] == 30);

    return garry_test_failures;
}
