/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "hierarchical.h"
#include "test_helpers.h"

int main(void)
{
    garry_byte_array k;
    garry_byte_array sub;
    garry_byte_array parent;
    garry_byte_array child_buf;
    garry_i32 v0;
    garry_i32 sub_len;
    garry_i32 parent_len;
    garry_i32 child_len;

    /* Key: accounts/user1/balance (3 subscripts). */
    { garry_key_tuple t3 = garry_make_key_3("accounts", "user1", "balance");
      garry_encode_key_tuple(&t3, k); }

    v0 = garry_subscript_count(k, 1 * 3 + 8 + 5 + 7);
    GARRY_CHECK(v0 == 3);

    v0 = garry_get_subscript_offset(k, 1 * 3 + 8 + 5 + 7, 0);
    GARRY_CHECK(v0 == 0);

    v0 = garry_decode_length_prefix(k, 0);
    GARRY_CHECK(v0 == 8);

    v0 = garry_decode_length_prefix(k, 9);
    GARRY_CHECK(v0 == 5);

    v0 = garry_decode_length_prefix(k, 15);
    GARRY_CHECK(v0 == 7);

    /* Key: accounts/user1/txns/1 (4 subscripts). */
    { garry_key_tuple t4 = garry_make_key_4("accounts", "user1", "txns", "1");
      garry_encode_key_tuple(&t4, k); }

    v0 = garry_subscript_count(k, 1 * 4 + 8 + 5 + 4 + 1);
    GARRY_CHECK(v0 == 4);

    v0 = garry_length_prefix_size(8);
    GARRY_CHECK(v0 == 1);

    v0 = garry_length_prefix_size(200);
    GARRY_CHECK(v0 == 3);

    /* Key: a/b (2 subscripts, single-byte each). */
    { garry_key_tuple t2 = garry_make_key_2("a", "b");
      garry_encode_key_tuple(&t2, k); }

    sub_len = 0;
    garry_extract_subscript(k, 1 + 1 + 1 + 1, 0, sub, &sub_len);
    GARRY_CHECK(sub_len == 1);
    GARRY_CHECK((garry_i32)sub[0] == 97);

    garry_extract_subscript(k, 1 + 1 + 1 + 1, 1, sub, &sub_len);
    GARRY_CHECK(sub_len == 1);
    GARRY_CHECK((garry_i32)sub[0] == 98);

    /* CONCAT_PREFIX: parent=[4], sub="b" -> [4, len(1), 98]. */
    memset(parent, 0, sizeof(parent));
    parent[0] = (garry_byte)4;
    parent_len = 1;
    child_len = garry_concat_prefix(parent, parent_len, sub, sub_len, child_buf, sizeof(child_buf));
    GARRY_CHECK(child_len == 3);
    GARRY_CHECK((garry_i32)child_buf[0] == 4);
    GARRY_CHECK((garry_i32)child_buf[1] == 1);
    GARRY_CHECK((garry_i32)child_buf[2] == 98);

    /* Key: acct/usr1/bal — extract subscript 1 ("usr1"). */
    { garry_key_tuple t3b = garry_make_key_3("acct", "usr1", "bal");
      garry_encode_key_tuple(&t3b, k); }

    garry_extract_subscript(k, 1 + 4 + 1 + 4 + 1 + 3, 1, sub, &sub_len);
    GARRY_CHECK(sub_len == 4);
    GARRY_CHECK((garry_i32)sub[0] == 117);

    return garry_test_failures;
}
