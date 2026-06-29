/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file test_blog_storage.c
 * @brief Integration test storing all 9 blog articles with full metadata
 *        and complete content using hierarchical keysplit keys.
 */

#include "garry/api.h"
#include "garry/keysplit.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_blog_test.db"

#ifndef GARRY_TEST_CONTENT_DIR
#define GARRY_TEST_CONTENT_DIR "content"
#endif

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static const char *slugs[] = {
    "acoa-trauma-forgiveness-and-self-love",
    "baking-soda-non-toxic-household-hacks",
    "benefits-of-wool-wellness-clothing",
    "cooking-from-scratch-guide-ditching-processed-foods",
    "keynesian-economics-daily-life-mental-health",
    "philosophy-of-fountain-pens-analog-writing",
    "sarah-connor-vs-trad-wife-2030",
    "walking-10km-a-day-second-heart",
    "zen-and-the-algorithmic-garden"
};
#define NUM_ARTICLES 9

/* Read entire file into buf, return bytes read or -1 on error. */
static long read_full_file(const char *path, char *buf, long max_len)
{
    FILE *f;
    long n;
    f = fopen(path, "rb");
    if (!f) return -1;
    n = (long)fread(buf, 1, (size_t)max_len, f);
    fclose(f);
    return n;
}

/* Parse title from YAML front matter: title: "..." */
static int parse_title(const char *content, char *title, int title_size)
{
    const char *start;
    const char *end;
    int len;
    start = strstr(content, "title: \"");
    if (!start) return 0;
    start += 8;
    end = strchr(start, '"');
    if (!end) return 0;
    len = (int)(end - start);
    if (len >= title_size) len = title_size - 1;
    memcpy(title, start, (size_t)len);
    title[len] = '\0';
    return len;
}

/* Parse date from YAML front matter: date: YYYY-MM-DD */
static int parse_date(const char *content, char *date, int date_size)
{
    const char *start;
    const char *end;
    int len;
    start = strstr(content, "date: ");
    if (!start) return 0;
    start += 6;
    end = start;
    while (*end && *end != '\n' && *end != '\r') end++;
    len = (int)(end - start);
    if (len >= date_size) len = date_size - 1;
    memcpy(date, start, (size_t)len);
    date[len] = '\0';
    return len;
}

static void test_store_all_articles(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key;
    garry_u8 *content;
    garry_u8 *result;
    garry_i32 klen, vlen;
    garry_bool ok;
    char fpath[512];
    char title[256];
    char date[32];
    long content_len;
    int title_len, date_len;
    int i;
    int stored = 0;

    printf("test_store_all_articles\n");
    cleanup();

    content = (garry_u8*)malloc(GARRY_MAX_RECORD_SIZE);
    result = (garry_u8*)malloc(GARRY_MAX_RECORD_SIZE);
    GARRY_CHECK(content != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    /* Store all 9 articles */
    txn = garry_txn_begin(db);
    for (i = 0; i < NUM_ARTICLES; i++) {
        /* Read full file */
        sprintf(fpath, "%s/%s.md", GARRY_TEST_CONTENT_DIR, slugs[i]);
        content_len = read_full_file(fpath, (char*)content, GARRY_MAX_RECORD_SIZE);
        GARRY_CHECK(content_len > 0);

        /* Parse metadata from front matter */
        title_len = parse_title((const char*)content, title, sizeof(title));
        date_len = parse_date((const char*)content, date, sizeof(date));
        GARRY_CHECK(title_len > 0);
        GARRY_CHECK(date_len > 0);

        /* Store title */
        sprintf(fpath, "post/%s/title", slugs[i]);
        klen = garry_key_split(fpath, '/', key);
        ok = garry_set(db, txn, key, klen, (const garry_u8*)title, (garry_i32)title_len);
        GARRY_CHECK(ok == GARRY_OK);

        /* Store author */
        sprintf(fpath, "post/%s/author", slugs[i]);
        klen = garry_key_split(fpath, '/', key);
        ok = garry_set(db, txn, key, klen,
                       (const garry_u8*)"Garry the Arborist", 18);
        GARRY_CHECK(ok == GARRY_OK);

        /* Store date */
        sprintf(fpath, "post/%s/date", slugs[i]);
        klen = garry_key_split(fpath, '/', key);
        ok = garry_set(db, txn, key, klen,
                       (const garry_u8*)date, (garry_i32)date_len);
        GARRY_CHECK(ok == GARRY_OK);

        /* Store full content */
        sprintf(fpath, "post/%s/content", slugs[i]);
        klen = garry_key_split(fpath, '/', key);
        ok = garry_set(db, txn, key, klen, content, (garry_i32)content_len);
        GARRY_CHECK(ok == GARRY_OK);

        stored++;
    }
    garry_txn_commit(db, txn);
    printf("  stored %d articles\n", stored);

    /* Verify all articles can be retrieved with correct content */
    txn = garry_txn_begin(db);
    for (i = 0; i < NUM_ARTICLES; i++) {
        /* Read original file for comparison */
        sprintf(fpath, "%s/%s.md", GARRY_TEST_CONTENT_DIR, slugs[i]);
        content_len = read_full_file(fpath, (char*)content, GARRY_MAX_RECORD_SIZE);
        GARRY_CHECK(content_len > 0);

        title_len = parse_title((const char*)content, title, sizeof(title));
        date_len = parse_date((const char*)content, date, sizeof(date));

        /* Verify title */
        sprintf(fpath, "post/%s/title", slugs[i]);
        klen = garry_key_split(fpath, '/', key);
        memset(result, 0, GARRY_MAX_RECORD_SIZE);
        vlen = 0;
        ok = garry_get(db, txn, key, klen, result, &vlen);
        GARRY_CHECK(ok == GARRY_OK);
        GARRY_CHECK(vlen == title_len);
        GARRY_CHECK(memcmp(result, title, (size_t)title_len) == 0);

        /* Verify author */
        sprintf(fpath, "post/%s/author", slugs[i]);
        klen = garry_key_split(fpath, '/', key);
        memset(result, 0, GARRY_MAX_RECORD_SIZE);
        vlen = 0;
        ok = garry_get(db, txn, key, klen, result, &vlen);
        GARRY_CHECK(ok == GARRY_OK);
        GARRY_CHECK(vlen == 18);
        GARRY_CHECK(memcmp(result, "Garry the Arborist", 18) == 0);

        /* Verify date */
        sprintf(fpath, "post/%s/date", slugs[i]);
        klen = garry_key_split(fpath, '/', key);
        memset(result, 0, GARRY_MAX_RECORD_SIZE);
        vlen = 0;
        ok = garry_get(db, txn, key, klen, result, &vlen);
        GARRY_CHECK(ok == GARRY_OK);
        GARRY_CHECK(vlen == date_len);
        GARRY_CHECK(memcmp(result, date, (size_t)date_len) == 0);

        /* Verify full content */
        sprintf(fpath, "post/%s/content", slugs[i]);
        klen = garry_key_split(fpath, '/', key);
        memset(result, 0, GARRY_MAX_RECORD_SIZE);
        vlen = 0;
        ok = garry_get(db, txn, key, klen, result, &vlen);
        GARRY_CHECK(ok == GARRY_OK);
        GARRY_CHECK(vlen == (garry_i32)content_len);
        GARRY_CHECK(memcmp(result, content, (size_t)content_len) == 0);
    }
    garry_txn_rollback(db, txn);

    garry_database_close(db);
    free(content);
    free(result);
    cleanup();
}

int main(void)
{
    test_store_all_articles();
    if (garry_test_failures == 0) printf("test_blog_storage: ALL PASSED\n");
    return garry_test_failures;
}
