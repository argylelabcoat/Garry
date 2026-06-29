/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file buffer_pool.h
 * @brief Buffer pool manager for fixed-size page caching.
 */

#ifndef GARRY_BUFFER_POOL_H
#define GARRY_BUFFER_POOL_H

#include "garry/config.h"
#include "storage_types.h"
#include "storage_core.h"
#include "file_io.h"
#include "garry_threading.h"

typedef struct {
    garry_u32 capacity;
    garry_u32 page_size;
    garry_page_buffer pages[GARRY_MAX_POOL_SIZE];
    garry_i32 page_ids[GARRY_MAX_POOL_SIZE];
    garry_u32 pin_counts[GARRY_MAX_POOL_SIZE];
    garry_bool dirty[GARRY_MAX_POOL_SIZE];
    garry_bool loaded[GARRY_MAX_POOL_SIZE];
    garry_u32 last_access[GARRY_MAX_POOL_SIZE];
    garry_file_descriptor fd;
    garry_i32 next_page;
    garry_u32 access_counter;
    garry_rwlock slot_locks[GARRY_MAX_POOL_SIZE];
} garry_buffer_pool;

garry_buffer_pool *garry_pool_create(const char *path, garry_u32 capacity, garry_u32 page_size);
garry_page_buffer *garry_pool_pin_page(garry_buffer_pool *pool, garry_i32 pid);
garry_page_buffer *garry_pool_try_pin(garry_buffer_pool *pool, garry_i32 pid);
void garry_pool_release_page(garry_buffer_pool *pool, garry_i32 pid);
garry_u32 garry_pool_pin_count(garry_buffer_pool *pool, garry_i32 pid);
void garry_pool_mark_dirty(garry_buffer_pool *pool, garry_i32 pid);
garry_i32 garry_pool_allocate(garry_buffer_pool *pool);
garry_bool garry_pool_is_loaded(garry_buffer_pool *pool, garry_i32 pid);
garry_bool garry_pool_flush_page(garry_buffer_pool *pool, garry_i32 pid);
void garry_pool_flush_all(garry_buffer_pool *pool);
void garry_pool_close(garry_buffer_pool *pool);

#endif
