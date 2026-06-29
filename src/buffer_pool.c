/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file buffer_pool.c
 * @brief Fixed-size page cache with LRU eviction and pin counting.
 *
 * Maintains an array of page slots. Each slot holds a page buffer,
 * a pin count (preventing eviction of in-use pages), and a dirty flag.
 * Pages are loaded from file on first access and evicted LRU when the
 * pool is full. Dirty pages are flushed before eviction and on close.
 *
 * Threading: callers must hold the engine rwlock for read access;
 * pool allocation and flush require exclusive access.
 */

#include "buffer_pool.h"
#include <stdlib.h>
#include <string.h>

#ifndef GARRY_TRUE
#define GARRY_TRUE  1
#endif
#ifndef GARRY_FALSE
#define GARRY_FALSE 0
#endif

static garry_u32 find_slot(garry_buffer_pool *pool, garry_i32 pid) {
    garry_u32 i;
    for (i = 0; i < pool->capacity; i++) {
        if (pool->loaded[i] && pool->page_ids[i] == pid) return i;
    }
    return (garry_u32)-1;
}

static garry_u32 find_eviction_candidate(garry_buffer_pool *pool) {
    garry_u32 i;
    garry_u32 best = (garry_u32)-1;
    garry_u32 best_access = 0xFFFFFFFF;
    for (i = 0; i < pool->capacity; i++) {
        if (!pool->loaded[i]) return i;
    }
    for (i = 0; i < pool->capacity; i++) {
        if (pool->pin_counts[i] == 0 && pool->last_access[i] < best_access) {
            best_access = pool->last_access[i];
            best = i;
        }
    }
    return best;
}

static garry_u32 ensure_loaded(garry_buffer_pool *pool, garry_i32 pid) {
    garry_u32 idx, victim;
    idx = find_slot(pool, pid);
    if (idx != (garry_u32)-1) return idx;
    victim = find_eviction_candidate(pool);
    if (victim == (garry_u32)-1) return (garry_u32)-1;
    if (pool->loaded[victim] && pool->dirty[victim]) {
        garry_file_write_page(&pool->fd, pool->page_ids[victim],
                              pool->pages[victim], (garry_i32)pool->page_size);
        pool->dirty[victim] = GARRY_FALSE;
    }
    pool->page_ids[victim] = pid;
    pool->loaded[victim] = GARRY_TRUE;
    pool->dirty[victim] = GARRY_FALSE;
    pool->pin_counts[victim] = 0;
    pool->access_counter++;
    pool->last_access[victim] = pool->access_counter;
    memset(pool->pages[victim], 0, pool->page_size);
    garry_file_read_page(&pool->fd, pid, pool->pages[victim], (garry_i32)pool->page_size);
    return victim;
}

garry_buffer_pool *garry_pool_create(const char *path, garry_u32 capacity, garry_u32 page_size) {
    garry_buffer_pool *pool;
    garry_u32 i;
    if (capacity == 0 || capacity > GARRY_MAX_POOL_SIZE) return NULL;
    if (page_size < 512 || page_size > GARRY_MAX_PAGE_SIZE) return NULL;
    pool = (garry_buffer_pool *)malloc(sizeof(garry_buffer_pool));
    if (!pool) return NULL;
    pool->fd = garry_file_open(path, GARRY_O_RDWR | GARRY_O_CREAT);
    pool->capacity = capacity;
    pool->page_size = page_size;
    pool->next_page = 0;
    for (i = 0; i < GARRY_MAX_POOL_SIZE; i++) {
        pool->loaded[i] = GARRY_FALSE;
        pool->pin_counts[i] = 0;
        pool->dirty[i] = GARRY_FALSE;
        pool->page_ids[i] = 0;
        pool->last_access[i] = 0;
    }
    pool->access_counter = 0;
    for (i = 0; i < GARRY_MAX_POOL_SIZE; i++) {
        garry_rwlock_init(&pool->slot_locks[i]);
    }
    return pool;
}

garry_page_buffer *garry_pool_pin_page(garry_buffer_pool *pool, garry_i32 pid) {
    garry_u32 idx;
    if (!pool) return NULL;
    if (pid < 0) return NULL;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = ensure_loaded(pool, pid);
    if (idx == (garry_u32)-1) {
        garry_rwlock_wrunlock(&pool->slot_locks[0]);
        return NULL;
    }
    pool->pin_counts[idx]++;
    pool->access_counter++;
    pool->last_access[idx] = pool->access_counter;
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
    return &pool->pages[idx];
}

garry_page_buffer *garry_pool_try_pin(garry_buffer_pool *pool, garry_i32 pid) {
    garry_u32 idx;
    if (!pool) return NULL;
    if (pid < 0) return NULL;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx == (garry_u32)-1) {
        garry_rwlock_wrunlock(&pool->slot_locks[0]);
        return NULL;
    }
    pool->pin_counts[idx]++;
    pool->access_counter++;
    pool->last_access[idx] = pool->access_counter;
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
    return &pool->pages[idx];
}

void garry_pool_release_page(garry_buffer_pool *pool, garry_i32 pid) {
    garry_u32 idx;
    if (!pool) return;
    if (pid < 0) return;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx != (garry_u32)-1 && pool->pin_counts[idx] > 0) {
        pool->pin_counts[idx]--;
    }
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
}

garry_u32 garry_pool_pin_count(garry_buffer_pool *pool, garry_i32 pid) {
    garry_u32 idx;
    if (!pool) return 0;
    if (pid < 0) return 0;
    idx = find_slot(pool, pid);
    if (idx != (garry_u32)-1) return pool->pin_counts[idx];
    return 0;
}

void garry_pool_mark_dirty(garry_buffer_pool *pool, garry_i32 pid) {
    garry_u32 idx;
    if (!pool) return;
    if (pid < 0) return;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx != (garry_u32)-1) pool->dirty[idx] = GARRY_TRUE;
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
}

garry_i32 garry_pool_allocate(garry_buffer_pool *pool) {
    garry_u32 victim;
    garry_i32 pid;
    if (!pool) return -1;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    victim = find_eviction_candidate(pool);
    if (victim == (garry_u32)-1) {
        garry_rwlock_wrunlock(&pool->slot_locks[0]);
        return -1;
    }
    if (pool->next_page < 0) {
        garry_rwlock_wrunlock(&pool->slot_locks[0]);
        return -1;
    }
    if (pool->dirty[victim]) {
        garry_file_write_page(&pool->fd, pool->page_ids[victim],
                              pool->pages[victim], (garry_i32)pool->page_size);
    }
    pid = pool->next_page;
    pool->next_page++;
    pool->page_ids[victim] = pid;
    memset(pool->pages[victim], 0, pool->page_size);
    pool->loaded[victim] = GARRY_TRUE;
    pool->dirty[victim] = GARRY_FALSE;
    pool->pin_counts[victim] = 0;
    pool->access_counter++;
    pool->last_access[victim] = pool->access_counter;
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
    return pid;
}

garry_bool garry_pool_is_loaded(garry_buffer_pool *pool, garry_i32 pid) {
    if (!pool) return GARRY_FALSE;
    return find_slot(pool, pid) != (garry_u32)-1 ? GARRY_TRUE : GARRY_FALSE;
}

void garry_pool_flush_page(garry_buffer_pool *pool, garry_i32 pid) {
    garry_u32 idx;
    if (!pool) return;
    if (pid < 0) return;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx != (garry_u32)-1 && pool->dirty[idx]) {
        garry_file_write_page(&pool->fd, pool->page_ids[idx],
                              pool->pages[idx], (garry_i32)pool->page_size);
        pool->dirty[idx] = GARRY_FALSE;
    }
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
}

void garry_pool_flush_all(garry_buffer_pool *pool) {
    garry_u32 i;
    if (!pool) return;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    for (i = 0; i < pool->capacity; i++) {
        if (pool->loaded[i] && pool->dirty[i]) {
            garry_file_write_page(&pool->fd, pool->page_ids[i],
                                  pool->pages[i], (garry_i32)pool->page_size);
            pool->dirty[i] = GARRY_FALSE;
        }
    }
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
}

void garry_pool_close(garry_buffer_pool *pool) {
    garry_u32 i;
    if (!pool) return;
    for (i = 0; i < pool->capacity; i++) {
        if (pool->loaded[i] && pool->dirty[i]) {
            garry_file_write_page(&pool->fd, pool->page_ids[i],
                                  pool->pages[i], (garry_i32)pool->page_size);
            pool->dirty[i] = GARRY_FALSE;
        }
    }
    garry_file_close(&pool->fd);
    for (i = 0; i < GARRY_MAX_POOL_SIZE; i++) {
        garry_rwlock_destroy(&pool->slot_locks[i]);
    }
    free(pool);
}
