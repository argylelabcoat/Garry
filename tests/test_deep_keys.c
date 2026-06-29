/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "garry/keysplit.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

static void test_5_level_deep_key(void)
{
    garry_byte_array k;
    garry_i32 klen;
    char result[256];
    garry_i32 nsubs;

    printf("test_5_level_deep_key\n");

    memset(k, 0, sizeof(k));
    klen = garry_key_split("a/b/c/d/e", '/', k);
    GARRY_CHECK(klen > 0);

    nsubs = garry_key_unsplit(k, klen, '/', result, sizeof(result));
    GARRY_CHECK(nsubs == 5);
    GARRY_CHECK(strcmp(result, "a/b/c/d/e") == 0);
}

static void test_8_level_deep_key(void)
{
    garry_byte_array k;
    garry_i32 klen;

    printf("test_8_level_deep_key\n");

    klen = garry_key_split("a/b/c/d/e/f/g/h", '/', k);
    GARRY_CHECK(klen > 0);
}

static void test_key_split_multi_char_delimiter(void)
{
    garry_byte_array k;
    garry_i32 klen;

    printf("test_key_split_multi_char_delimiter\n");

    klen = garry_key_split("part1::part2::part3", ':', k);
    GARRY_CHECK(klen > 0);

    klen = garry_key_split("a-b-c-d", '-', k);
    GARRY_CHECK(klen > 0);

    klen = garry_key_split("com.example.www", '.', k);
    GARRY_CHECK(klen > 0);
}

static void test_key_unsplit_round_trip(void)
{
    garry_byte_array k;
    char result[256];
    garry_i32 klen;
    garry_i32 nsubs;

    printf("test_key_unsplit_round_trip\n");

    memset(k, 0, sizeof(k));
    klen = garry_key_split("users/matthew/articles", '/', k);
    nsubs = garry_key_unsplit(k, klen, '/', result, sizeof(result));
    GARRY_CHECK(nsubs == 3);
    GARRY_CHECK(strcmp(result, "users/matthew/articles") == 0);

    memset(k, 0, sizeof(k));
    klen = garry_key_split("com.example.method", '.', k);
    nsubs = garry_key_unsplit(k, klen, '.', result, sizeof(result));
    GARRY_CHECK(nsubs == 3);
    GARRY_CHECK(strcmp(result, "com.example.method") == 0);
}

static void test_key_empty_subscripts(void)
{
    garry_byte_array k;
    garry_i32 klen;
    char result[256];
    garry_i32 nsubs;

    printf("test_key_empty_subscripts\n");

    klen = garry_key_split("a//b", '/', k);
    GARRY_CHECK(klen > 0);

    nsubs = garry_key_unsplit(k, klen, '/', result, sizeof(result));
    GARRY_CHECK(nsubs == 3);
    GARRY_CHECK(strcmp(result, "a//b") == 0);
}

static void test_key_long_subscripts(void)
{
    garry_byte_array k;
    garry_i32 klen;
    char result[512];
    garry_i32 nsubs;
    const char *parts[2];

    printf("test_key_long_subscripts\n");

    memset(k, 0, sizeof(k));
    parts[0] = "short";
    parts[1] =
        "this_is_a_very_long_subscript_that_exceeds_128_bytes_"
        "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    klen = garry_make_key_parts(parts, 2, k);
    GARRY_CHECK(klen > 0);

    nsubs = garry_key_unsplit(k, klen, '/', result, sizeof(result));
    GARRY_CHECK(nsubs == 2);
}

static void test_make_key_parts_mixed_lengths(void)
{
    garry_byte_array k;
    garry_i32 klen;
    const char *parts[4];

    printf("test_make_key_parts_mixed_lengths\n");

    parts[0] = "a";
    parts[1] = "this_is_a_medium_length_part";
    parts[2] = "x";
    parts[3] = "another_long_part_that_has_more_characters_than_the_others";

    klen = garry_make_key_parts(parts, 4, k);
    GARRY_CHECK(klen > 0);
}

int main(void)
{
    test_5_level_deep_key();
    test_8_level_deep_key();
    test_key_split_multi_char_delimiter();
    test_key_unsplit_round_trip();
    test_key_empty_subscripts();
    test_key_long_subscripts();
    test_make_key_parts_mixed_lengths();
    if (garry_test_failures == 0)
        printf("test_deep_keys: ALL PASSED\n");
    return garry_test_failures;
}
