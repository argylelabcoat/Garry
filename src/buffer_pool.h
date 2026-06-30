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

typedef struct
{
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
    garry_i32 free_list_head;
    garry_u32 access_counter;
    garry_rwlock slot_locks[GARRY_MAX_POOL_SIZE];
} garry_buffer_pool;

/**
 * Create a new buffer pool backed by a database file.
 *
 * @param path Filesystem path of the database file.
 * @param capacity Maximum number of pages the pool can hold.
 * @param page_size Size of each page in bytes.
 * @return Buffer pool handle, or NULL on failure.
 */
garry_buffer_pool *garry_pool_create(const char *path, garry_u32 capacity, garry_u32 page_size);

/**
 * Pin a page into the pool, loading it from disk if necessary.
 *
 * Increments the pin count, preventing eviction. If the page is not
 * already loaded, evicts an unpinned LRU victim first.
 *
 * @param pool Buffer pool handle.
 * @param pid Page ID to pin.
 * @return Pointer to the page buffer, or NULL on failure.
 */
garry_page_buffer *garry_pool_pin_page(garry_buffer_pool *pool, garry_i32 pid);

/**
 * Try to pin an already-loaded page without loading from disk.
 *
 * Unlike garry_pool_pin_page, this does not load the page if it is
 * not already cached. Returns NULL if the page is not loaded.
 *
 * @param pool Buffer pool handle.
 * @param pid Page ID to try pinning.
 * @return Pointer to the page buffer, or NULL if not loaded.
 */
garry_page_buffer *garry_pool_try_pin(garry_buffer_pool *pool, garry_i32 pid);

/**
 * Release a previously pinned page.
 *
 * Decrements the pin count. The page may be evicted once its pin
 * count reaches zero.
 *
 * @param pool Buffer pool handle.
 * @param pid Page ID to release.
 */
void garry_pool_release_page(garry_buffer_pool *pool, garry_i32 pid);

/**
 * Return the current pin count of a page.
 *
 * @param pool Buffer pool handle.
 * @param pid Page ID to query.
 * @return Pin count (0 if the page is not loaded).
 */
garry_u32 garry_pool_pin_count(garry_buffer_pool *pool, garry_i32 pid);

/**
 * Mark a loaded page as dirty (modified in memory).
 *
 * @param pool Buffer pool handle.
 * @param pid Page ID to mark dirty.
 */
void garry_pool_mark_dirty(garry_buffer_pool *pool, garry_i32 pid);

/**
 * Allocate a new page from the pool.
 *
 * Reuses a freed page from the free list if available, otherwise
 * increments the next page counter.
 *
 * @param pool Buffer pool handle.
 * @return Page ID of the newly allocated page, or -1 on failure.
 */
garry_i32 garry_pool_allocate(garry_buffer_pool *pool);

/**
 * Return a page to the free list for later reuse.
 *
 * Flushes the page if dirty, evicts it from the cache, and prepends
 * it to the free list.
 *
 * @param pool Buffer pool handle.
 * @param pid Page ID to free.
 */
void garry_pool_free_page(garry_buffer_pool *pool, garry_i32 pid);

/**
 * Check whether a page is currently loaded in the pool.
 *
 * @param pool Buffer pool handle.
 * @param pid Page ID to check.
 * @return GARRY_TRUE if loaded, GARRY_FALSE otherwise.
 */
garry_bool garry_pool_is_loaded(garry_buffer_pool *pool, garry_i32 pid);

/**
 * Flush a single dirty page to disk.
 *
 * @param pool Buffer pool handle.
 * @param pid Page ID to flush.
 * @return GARRY_TRUE on success, GARRY_FALSE on I/O failure.
 */
garry_bool garry_pool_flush_page(garry_buffer_pool *pool, garry_i32 pid);

/**
 * Flush all dirty pages in the pool to disk.
 *
 * @param pool Buffer pool handle.
 */
void garry_pool_flush_all(garry_buffer_pool *pool);

/**
 * Close the pool, flushing dirty pages and releasing all resources.
 *
 * @param pool Buffer pool handle to close.
 */
void garry_pool_close(garry_buffer_pool *pool);

#endif
