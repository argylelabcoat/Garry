/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file test_lz4_compression.c
 * @brief Round-trip, ratio, and edge-case tests for the LZ4 compressor.
 *        Uses real markdown content from the content/ directory.
 */

#include "lz4.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef GARRY_TEST_CONTENT_DIR
#define GARRY_TEST_CONTENT_DIR "content"
#endif

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { \
        printf("  FAIL: %s (line %d)\n", msg, __LINE__); \
    } else { \
        tests_passed++; \
    } \
} while (0)

static char* read_file(const char *path, long *out_len)
{
    FILE *f;
    long len;
    char *buf;

    f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (char*)malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    if ((long)fread(buf, 1, len, f) != len) {
        free(buf); fclose(f); return NULL;
    }
    buf[len] = '\0';
    fclose(f);
    *out_len = len;
    return buf;
}

static char* content_path(char *buf, size_t bufsz, const char *filename)
{
    snprintf(buf, bufsz, "%s/%s", GARRY_TEST_CONTENT_DIR, filename);
    return buf;
}

static void test_round_trip_basic(void)
{
    const char *input = "Hello, LZ4 compression test string!";
    size_t in_len = strlen(input);
    size_t compressed_len = 0;
    char *compressed;
    size_t decompressed_len = 0;
    char *decompressed;

    printf("test_round_trip_basic\n");
    compressed = lz4_compress(input, in_len, &compressed_len);
    ASSERT(compressed != NULL, "compress returns non-NULL");
    ASSERT(compressed_len > 0, "compressed length > 0");

    decompressed = lz4_decompress(compressed, compressed_len, in_len * 2, &decompressed_len);
    ASSERT(decompressed != NULL, "decompress returns non-NULL");
    ASSERT(decompressed_len == in_len, "decompressed length matches original");
    ASSERT(memcmp(decompressed, input, in_len) == 0, "decompressed data matches original");

    lz4_free(compressed);
    lz4_free(decompressed);
}

static void test_round_trip_repeated(void)
{
    const char *input = "ABCABCABCABCABCABCABCABCABCABC";
    size_t in_len = strlen(input);
    size_t compressed_len = 0;
    char *compressed;
    size_t decompressed_len = 0;
    char *decompressed;

    printf("test_round_trip_repeated\n");
    compressed = lz4_compress(input, in_len, &compressed_len);
    ASSERT(compressed != NULL, "compress returns non-NULL");
    ASSERT(compressed_len > 0, "compressed length > 0");

    decompressed = lz4_decompress(compressed, compressed_len, in_len * 2, &decompressed_len);
    ASSERT(decompressed != NULL, "decompress returns non-NULL");
    ASSERT(decompressed_len == in_len, "decompressed length matches");
    ASSERT(memcmp(decompressed, input, in_len) == 0, "data matches after round-trip");

    lz4_free(compressed);
    lz4_free(decompressed);
}

static void test_compression_ratio(void)
{
    char input[1024];
    size_t in_len;
    size_t compressed_len = 0;
    char *compressed;
    size_t decompressed_len = 0;
    char *decompressed;
    size_t i;

    printf("test_compression_ratio\n");

    for (i = 0; i < sizeof(input); i++) {
        input[i] = (char)('A' + (i % 26));
    }
    in_len = sizeof(input);

    compressed = lz4_compress(input, in_len, &compressed_len);
    ASSERT(compressed != NULL, "compress returns non-NULL");
    ASSERT(compressed_len < in_len, "compressed < original for compressible data");

    decompressed = lz4_decompress(compressed, compressed_len, in_len * 2, &decompressed_len);
    ASSERT(decompressed != NULL, "decompress returns non-NULL");
    ASSERT(decompressed_len == in_len, "length matches");
    ASSERT(memcmp(decompressed, input, in_len) == 0, "data matches");

    lz4_free(compressed);
    lz4_free(decompressed);
}

static void test_empty_input(void)
{
    size_t compressed_len = 999;
    char *compressed;
    size_t decompressed_len = 999;

    printf("test_empty_input\n");
    compressed = lz4_compress("x", 0, &compressed_len);
    ASSERT(compressed == NULL, "compress(NULL/empty) returns NULL");
    ASSERT(compressed_len == 0, "out_len set to 0 on empty input");

    compressed = lz4_decompress("x", 0, 100, &decompressed_len);
    ASSERT(compressed == NULL, "decompress(empty src) returns NULL");
    ASSERT(decompressed_len == 0, "decompress out_len=0 on empty input");
}

static void test_single_byte(void)
{
    const char input = 'Z';
    size_t compressed_len = 0;
    char *compressed;
    size_t decompressed_len = 0;
    char *decompressed;

    printf("test_single_byte\n");
    compressed = lz4_compress(&input, 1, &compressed_len);
    ASSERT(compressed != NULL, "compress single byte returns non-NULL");

    decompressed = lz4_decompress(compressed, compressed_len, 64, &decompressed_len);
    ASSERT(decompressed != NULL, "decompress returns non-NULL");
    ASSERT(decompressed_len == 1, "decompressed length is 1");
    ASSERT(decompressed[0] == 'Z', "single byte matches");

    lz4_free(compressed);
    lz4_free(decompressed);
}

static void test_max_input_size(void)
{
    char input[1024];
    size_t in_len = sizeof(input);
    size_t compressed_len = 0;
    char *compressed;
    size_t decompressed_len = 0;
    char *decompressed;
    size_t i;

    printf("test_max_input_size\n");

    for (i = 0; i < in_len; i++) {
        input[i] = (char)(i & 0xFF);
    }

    compressed = lz4_compress(input, in_len, &compressed_len);
    ASSERT(compressed != NULL, "compress large size returns non-NULL");
    ASSERT(compressed_len > 0, "compressed length > 0");

    decompressed = lz4_decompress(compressed, compressed_len, in_len * 2, &decompressed_len);
    ASSERT(decompressed != NULL, "decompress returns non-NULL");
    ASSERT(decompressed_len == in_len, "decompressed length matches");
    ASSERT(memcmp(decompressed, input, in_len) == 0, "data matches for large input");

    lz4_free(compressed);
    lz4_free(decompressed);
}

static void test_corrupt_decompress(void)
{
    const char garbage[] = {0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA};
    size_t decompressed_len = 0;
    char *result;

    printf("test_corrupt_decompress\n");
    result = lz4_decompress(garbage, sizeof(garbage), 1024, &decompressed_len);
    ASSERT(1, "corrupt decompress did not crash");
    if (result) {
        lz4_free(result);
    }
}

static void test_compress_bound(void)
{
    printf("test_compress_bound\n");
    ASSERT(lz4_compress_bound(0) > 0, "bound(0) > 0");
    ASSERT(lz4_compress_bound(1) > 1, "bound(1) > 1");
    ASSERT(lz4_compress_bound(1000) > 1000, "bound(1000) > 1000");
    ASSERT(lz4_compress_bound(65536) > 0, "bound(65536) > 0");
    ASSERT(lz4_compress_bound(65537) == 0, "bound(>max) returns 0");
}

static void test_null_inputs(void)
{
    size_t len = 0;

    printf("test_null_inputs\n");
    ASSERT(lz4_compress(NULL, 10, &len) == NULL, "compress(NULL src) returns NULL");
    ASSERT(lz4_decompress(NULL, 10, 100, &len) == NULL, "decompress(NULL src) returns NULL");
}

static void test_real_file_round_trip(void)
{
    long file_len;
    char *file_data;
    size_t in_len;
    size_t compressed_len = 0;
    char *compressed;
    size_t decompressed_len = 0;
    char *decompressed;
    char path[256];

    printf("test_real_file_round_trip\n");

    content_path(path, sizeof(path), "keynesian-economics-daily-life-mental-health.md");
    file_data = read_file(path, &file_len);
    ASSERT(file_data != NULL, "file read succeeds");
    if (!file_data) return;

    in_len = (size_t)file_len;

    compressed = lz4_compress(file_data, in_len, &compressed_len);
    ASSERT(compressed != NULL, "compress real file returns non-NULL");
    ASSERT(compressed_len > 0, "compressed length > 0");
    ASSERT(compressed_len < in_len, "compressed < original for real content");

    decompressed = lz4_decompress(compressed, compressed_len, in_len * 2, &decompressed_len);
    ASSERT(decompressed != NULL, "decompress returns non-NULL");
    ASSERT(decompressed_len == in_len, "decompressed length matches original");
    ASSERT(memcmp(decompressed, file_data, in_len) == 0, "decompressed data matches file");

    free(file_data);
    lz4_free(compressed);
    lz4_free(decompressed);
}

static void test_multiple_compress_decompress_cycles(void)
{
    long file_len;
    char *file_data;
    size_t in_len;
    int cycle;
    size_t compressed_len = 0;
    char *compressed;
    size_t decompressed_len = 0;
    char *decompressed;
    char path[256];

    printf("test_multiple_compress_decompress_cycles\n");

    content_path(path, sizeof(path), "zen-and-the-algorithmic-garden.md");
    file_data = read_file(path, &file_len);
    ASSERT(file_data != NULL, "file read succeeds");
    if (!file_data) return;

    in_len = (size_t)file_len;

    for (cycle = 0; cycle < 5; cycle++) {
        compressed = lz4_compress(file_data, in_len, &compressed_len);
        ASSERT(compressed != NULL, "compress cycle succeeds");

        decompressed = lz4_decompress(compressed, compressed_len, in_len * 2, &decompressed_len);
        ASSERT(decompressed != NULL, "decompress cycle succeeds");
        ASSERT(decompressed_len == in_len, "length matches each cycle");
        ASSERT(memcmp(decompressed, file_data, in_len) == 0, "data matches each cycle");

        lz4_free(compressed);
        lz4_free(decompressed);
    }

    free(file_data);
}

static void test_compress_bound_real_file(void)
{
    long file_len;
    char *file_data;
    size_t in_len;
    size_t bound;
    size_t compressed_len = 0;
    char *compressed;
    char path[256];

    printf("test_compress_bound_real_file\n");

    content_path(path, sizeof(path), "sarah-connor-vs-trad-wife-2030.md");
    file_data = read_file(path, &file_len);
    ASSERT(file_data != NULL, "file read succeeds");
    if (!file_data) return;

    in_len = (size_t)file_len;
    bound = lz4_compress_bound(in_len);
    ASSERT(bound > in_len, "bound > original for real file");

    compressed = lz4_compress(file_data, in_len, &compressed_len);
    ASSERT(compressed != NULL, "compress returns non-NULL");
    ASSERT(compressed_len <= bound, "compressed <= bound");

    free(file_data);
    lz4_free(compressed);
}

int main(void)
{
    test_round_trip_basic();
    test_round_trip_repeated();
    test_compression_ratio();
    test_empty_input();
    test_single_byte();
    test_max_input_size();
    test_corrupt_decompress();
    test_compress_bound();
    test_null_inputs();
    test_real_file_round_trip();
    test_multiple_compress_decompress_cycles();
    test_compress_bound_real_file();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
