/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_engine.c
 * @brief Storage engine teardown and flush-on-close logic.
 *
 * Handles orderly shutdown: flushes dirty buffer pool pages, syncs
 * the WAL log, and releases all engine resources.
 */

#include "storage_engine.h"
#include "db_header.h"
#include "file_io.h"
#include "buffer_pool.h"
#include <string.h>

garry_engine_handle* garry_storage_init(const char *path, garry_engine_settings settings)
{
    garry_engine_handle *eng;

    if (!path) return NULL;

    eng = garry_engine_init(path, settings);
    return eng;
}

garry_engine_handle* garry_storage_open(const char *path)
{
    if (!path) return NULL;
    return garry_engine_open(path);
}

void garry_storage_close(garry_engine_handle *eng)
{
    garry_page_buffer *hdr_buf;

    if (!eng) return;

    eng->header.root_page = eng->btree_root;
    eng->header.total_pages = eng->pool->next_page;

    hdr_buf = garry_pool_pin_page(eng->pool, GARRY_HEADER_PAGE);
    if (hdr_buf) {
        garry_write_db_header((garry_byte*)*hdr_buf, &eng->header);
        garry_pool_mark_dirty(eng->pool, GARRY_HEADER_PAGE);
        garry_pool_release_page(eng->pool, GARRY_HEADER_PAGE);
    }

    garry_wal_log_flush(&eng->wal);
    garry_engine_close(eng);
}
