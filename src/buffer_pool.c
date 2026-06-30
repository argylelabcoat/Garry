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
#include "db_header.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Find the slot index holding a given page ID.
 *
 * @param pool  Buffer pool to search.
 * @param pid   Page ID to find.
 * @return Slot index, or GARRY_INVALID_SLOT if not found.
 */
static garry_u32 find_slot(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 i;
    for (i = 0; i < pool->capacity; i++)
    {
        if (pool->loaded[i] && pool->page_ids[i] == pid)
            return i;
    }
    return GARRY_INVALID_SLOT;
}

/**
 * @brief Find a slot suitable for eviction.
 *
 * Returns an unloaded slot immediately if one exists. Otherwise returns
 * the least-recently-accessed slot with zero pins. Returns
 * GARRY_INVALID_SLOT if all loaded slots are pinned (pool exhausted).
 *
 * @param pool  Buffer pool to search.
 * @return Slot index to evict, or GARRY_INVALID_SLOT if none available.
 */
static garry_u32 find_eviction_candidate(garry_buffer_pool *pool)
{
    garry_u32 i;
    garry_u32 best = GARRY_INVALID_SLOT;
    garry_u32 best_access = 0xFFFFFFFF;
    for (i = 0; i < pool->capacity; i++)
    {
        if (!pool->loaded[i])
            return i;
    }
    for (i = 0; i < pool->capacity; i++)
    {
        if (pool->pin_counts[i] == 0 && pool->last_access[i] < best_access)
        {
            best_access = pool->last_access[i];
            best = i;
        }
    }
    return best;
}

/**
 * @brief Ensure a page is loaded into the pool, evicting if necessary.
 *
 * If the page is already cached, returns its slot index. Otherwise
 * evicts a candidate slot (flushing it if dirty) and loads the page.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to ensure is loaded.
 * @return Slot index of the loaded page, or GARRY_INVALID_SLOT on failure.
 */
static garry_u32 ensure_loaded(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 idx, victim;
    idx = find_slot(pool, pid);
    if (idx != GARRY_INVALID_SLOT)
        return idx;
    victim = find_eviction_candidate(pool);
    if (victim == GARRY_INVALID_SLOT)
        return GARRY_INVALID_SLOT;
    if (pool->loaded[victim] && pool->dirty[victim])
    {
        if (!garry_file_write_page(&pool->fd, pool->page_ids[victim], pool->pages[victim],
                                   (garry_i32)pool->page_size))
        {
            return GARRY_INVALID_SLOT;
        }
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

/**
 * @brief Create and initialize a new buffer pool.
 *
 * Opens (or creates) the database file and allocates slot arrays for
 * the given capacity and page size.
 *
 * @param path      Path to the database file.
 * @param capacity  Maximum number of pages to cache.
 * @param page_size Size of each page in bytes.
 * @return Pointer to the new pool, or NULL on failure.
 */
garry_buffer_pool *garry_pool_create(const char *path, garry_u32 capacity, garry_u32 page_size)
{
    garry_buffer_pool *pool;
    garry_u32 i;
    if (capacity == 0 || capacity > GARRY_MAX_POOL_SIZE)
        return NULL;
    if (page_size < GARRY_MIN_PAGE_SIZE || page_size > GARRY_MAX_PAGE_SIZE)
        return NULL;
    pool = (garry_buffer_pool *)malloc(sizeof(garry_buffer_pool));
    if (!pool)
        return NULL;
    pool->fd = garry_file_open(path, GARRY_O_RDWR | GARRY_O_CREAT);
    pool->capacity = capacity;
    pool->page_size = page_size;
    pool->next_page = 0;
    pool->free_list_head = -1;
    for (i = 0; i < capacity; i++)
    {
        pool->loaded[i] = GARRY_FALSE;
        pool->pin_counts[i] = 0;
        pool->dirty[i] = GARRY_FALSE;
        pool->page_ids[i] = 0;
        pool->last_access[i] = 0;
    }
    pool->access_counter = 0;
    for (i = 0; i < capacity; i++)
    {
        garry_rwlock_init(&pool->slot_locks[i]);
    }
    return pool;
}

/**
 * @brief Pin a page, loading it if necessary.
 *
 * Loads the page into the pool (evicting if needed), increments its
 * pin count, and returns a pointer to the page buffer. The caller
 * must call garry_pool_release_page when done.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to pin.
 * @return Pointer to the page buffer, or NULL on failure.
 */
garry_page_buffer *garry_pool_pin_page(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 idx;
    if (!pool)
        return NULL;
    if (pid < 0)
        return NULL;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = ensure_loaded(pool, pid);
    if (idx == GARRY_INVALID_SLOT)
    {
        garry_rwlock_wrunlock(&pool->slot_locks[0]);
        return NULL;
    }
    pool->pin_counts[idx]++;
    pool->access_counter++;
    pool->last_access[idx] = pool->access_counter;
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
    return &pool->pages[idx];
}

/**
 * @brief Try to pin an already-loaded page without loading from disk.
 *
 * Returns a pointer to the page buffer only if the page is already
 * cached in the pool. Does not trigger I/O on cache miss.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to try pinning.
 * @return Pointer to the page buffer, or NULL if not loaded.
 */
garry_page_buffer *garry_pool_try_pin(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 idx;
    if (!pool)
        return NULL;
    if (pid < 0)
        return NULL;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx == GARRY_INVALID_SLOT)
    {
        garry_rwlock_wrunlock(&pool->slot_locks[0]);
        return NULL;
    }
    pool->pin_counts[idx]++;
    pool->access_counter++;
    pool->last_access[idx] = pool->access_counter;
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
    return &pool->pages[idx];
}

/**
 * @brief Release a pin on a page.
 *
 * Decrements the pin count for the given page. The page becomes
 * eligible for eviction once its pin count reaches zero.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to release.
 */
void garry_pool_release_page(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 idx;
    if (!pool)
        return;
    if (pid < 0)
        return;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx != GARRY_INVALID_SLOT && pool->pin_counts[idx] > 0)
    {
        pool->pin_counts[idx]--;
    }
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
}

/**
 * @brief Get the current pin count for a page.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to query.
 * @return Current pin count, or 0 if the page is not loaded.
 */
garry_u32 garry_pool_pin_count(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 idx;
    garry_u32 result;
    if (!pool)
        return 0;
    if (pid < 0)
        return 0;
    garry_rwlock_rdlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx != GARRY_INVALID_SLOT)
    {
        result = pool->pin_counts[idx];
    }
    else
    {
        result = 0;
    }
    garry_rwlock_rdunlock(&pool->slot_locks[0]);
    return result;
}

/**
 * @brief Mark a loaded page as dirty.
 *
 * The page will be flushed to disk on eviction or pool close.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to mark dirty.
 */
void garry_pool_mark_dirty(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 idx;
    if (!pool)
        return;
    if (pid < 0)
        return;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx != GARRY_INVALID_SLOT)
        pool->dirty[idx] = GARRY_TRUE;
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
}

/**
 * @brief Allocate a new page from the pool.
 *
 * First reuses a page from the free list if available. Otherwise
 * increments next_page to create a fresh page. Evicts a victim slot
 * if necessary, flushing dirty pages first.
 *
 * @param pool  Buffer pool.
 * @return Page ID of the newly allocated page, or -1 on failure.
 */
garry_i32 garry_pool_allocate(garry_buffer_pool *pool)
{
    garry_u32 victim;
    garry_i32 pid;
    if (!pool)
        return -1;
    garry_rwlock_wrlock(&pool->slot_locks[0]);

    /* Check free list first */
    if (pool->free_list_head >= 0)
    {
        garry_i32 free_pid;
        garry_u32 free_idx;
        free_pid = pool->free_list_head;
        /* Read next pointer directly from the loaded page (no re-pin needed) */
        free_idx = find_slot(pool, free_pid);
        if (free_idx != GARRY_INVALID_SLOT)
        {
            pool->free_list_head = garry_read_int32((garry_byte *)pool->pages[free_idx], 0);
        }
        else
        {
            pool->free_list_head = -1;
        }
        /* Reuse the slot that was holding this page (or find a new one) */
        victim = find_eviction_candidate(pool);
        if (victim == GARRY_INVALID_SLOT)
        {
            garry_rwlock_wrunlock(&pool->slot_locks[0]);
            return -1;
        }
        if (pool->dirty[victim])
        {
            if (!garry_file_write_page(&pool->fd, pool->page_ids[victim], pool->pages[victim],
                                       (garry_i32)pool->page_size))
            {
                garry_rwlock_wrunlock(&pool->slot_locks[0]);
                return -1;
            }
        }
        pid = free_pid;
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

    /* No free pages — allocate a new one */
    victim = find_eviction_candidate(pool);
    if (victim == GARRY_INVALID_SLOT)
    {
        garry_rwlock_wrunlock(&pool->slot_locks[0]);
        return -1;
    }
    if (pool->next_page < 0)
    {
        garry_rwlock_wrunlock(&pool->slot_locks[0]);
        return -1;
    }
    if (pool->dirty[victim])
    {
        if (!garry_file_write_page(&pool->fd, pool->page_ids[victim], pool->pages[victim],
                                   (garry_i32)pool->page_size))
        {
            garry_rwlock_wrunlock(&pool->slot_locks[0]);
            return -1;
        }
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

/**
 * @brief Return a page to the free list for later reuse.
 *
 * Flushes the page if dirty, evicts it from the cache, then writes
 * the free-list forward pointer into the page data so
 * garry_pool_allocate can traverse the chain.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to free.
 */
void garry_pool_free_page(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 idx;
    if (!pool)
        return;
    if (pid < 0)
        return;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    /* If the page is loaded, evict it first */
    idx = find_slot(pool, pid);
    if (idx != GARRY_INVALID_SLOT)
    {
        if (pool->dirty[idx])
        {
            /* Best-effort flush. On failure the in-memory page data is
             * abandoned — this is a data loss path. Callers should ensure
             * critical pages are flushed via garry_pool_flush_page before
             * calling free_page. */
            (void)garry_file_write_page(&pool->fd, pool->page_ids[idx], pool->pages[idx],
                                        (garry_i32)pool->page_size);
        }
        pool->loaded[idx] = GARRY_FALSE;
        pool->pin_counts[idx] = 0;
        pool->dirty[idx] = GARRY_FALSE;
    }
    /* Load the page to write the free-list pointer into it */
    idx = ensure_loaded(pool, pid);
    if (idx != GARRY_INVALID_SLOT)
    {
        garry_write_int32((garry_byte *)pool->pages[idx], 0, pool->free_list_head);
        pool->dirty[idx] = GARRY_TRUE;
        /* Keep the page loaded so garry_pool_allocate can read the next pointer */
    }
    pool->free_list_head = pid;
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
}

/**
 * @brief Check whether a page is currently loaded in the pool.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to check.
 * @return GARRY_TRUE if loaded, GARRY_FALSE otherwise.
 */
garry_bool garry_pool_is_loaded(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_bool result;
    if (!pool)
        return GARRY_FALSE;
    garry_rwlock_rdlock(&pool->slot_locks[0]);
    result = find_slot(pool, pid) != GARRY_INVALID_SLOT ? GARRY_TRUE : GARRY_FALSE;
    garry_rwlock_rdunlock(&pool->slot_locks[0]);
    return result;
}

/**
 * @brief Flush a single dirty page to disk.
 *
 * @param pool  Buffer pool.
 * @param pid   Page ID to flush.
 * @return GARRY_TRUE on success, GARRY_FALSE on failure.
 */
garry_bool garry_pool_flush_page(garry_buffer_pool *pool, garry_i32 pid)
{
    garry_u32 idx;
    if (!pool)
        return GARRY_FALSE;
    if (pid < 0)
        return GARRY_FALSE;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    idx = find_slot(pool, pid);
    if (idx != GARRY_INVALID_SLOT && pool->dirty[idx])
    {
        if (!garry_file_write_page(&pool->fd, pool->page_ids[idx], pool->pages[idx],
                                   (garry_i32)pool->page_size))
        {
            garry_rwlock_wrunlock(&pool->slot_locks[0]);
            return GARRY_FALSE;
        }
        pool->dirty[idx] = GARRY_FALSE;
    }
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
    return GARRY_TRUE;
}

/**
 * @brief Flush all dirty pages in the pool to disk.
 *
 * @param pool  Buffer pool.
 */
void garry_pool_flush_all(garry_buffer_pool *pool)
{
    garry_u32 i;
    if (!pool)
        return;
    garry_rwlock_wrlock(&pool->slot_locks[0]);
    for (i = 0; i < pool->capacity; i++)
    {
        if (pool->loaded[i] && pool->dirty[i])
        {
            if (!garry_file_write_page(&pool->fd, pool->page_ids[i], pool->pages[i],
                                       (garry_i32)pool->page_size))
            {
                garry_rwlock_wrunlock(&pool->slot_locks[0]);
                return;
            }
            pool->dirty[i] = GARRY_FALSE;
        }
    }
    garry_rwlock_wrunlock(&pool->slot_locks[0]);
}

/**
 * @brief Flush all dirty pages, close the file, and free the pool.
 *
 * @param pool  Buffer pool to close.
 */
void garry_pool_close(garry_buffer_pool *pool)
{
    garry_u32 i;
    if (!pool)
        return;
    for (i = 0; i < pool->capacity; i++)
    {
        if (pool->loaded[i] && pool->dirty[i])
        {
            (void)garry_file_write_page(&pool->fd, pool->page_ids[i], pool->pages[i],
                                        (garry_i32)pool->page_size);
            pool->dirty[i] = GARRY_FALSE;
        }
    }
    garry_file_close(&pool->fd);
    for (i = 0; i < pool->capacity; i++)
    {
        garry_rwlock_destroy(&pool->slot_locks[i]);
    }
    free(pool);
}
