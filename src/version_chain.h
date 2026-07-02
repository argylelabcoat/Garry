/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file version_chain.h
 * @brief Per-key version chain for MVCC storage.
 */

#ifndef GARRY_VERSION_CHAIN_H
#define GARRY_VERSION_CHAIN_H

#include "garry/config.h"
#include "storage_types.h"
#include "storage_core.h"
#include "buffer_pool.h"

typedef garry_txn_id *garry_txn_id_ptr;

/**
 * Initialise a version chain page header.
 *
 * @param buf Page buffer to initialise.
 * @param page_size Size of the page in bytes.
 */
void garry_chain_page_init(garry_page_buffer buf, garry_u32 page_size);

/**
 * Append an inline version entry to a chain page.
 *
 * @param buf Page buffer of the version chain.
 * @param page_size Size of the page in bytes.
 * @param txn Transaction ID that created this version.
 * @param value Value bytes to store inline.
 * @param vlen Length of the value in bytes.
 * @return GARRY_TRUE on success, GARRY_FALSE if the value exceeds inline capacity.
 */
garry_bool garry_chain_page_append(garry_page_buffer buf, garry_u32 page_size, garry_txn_id txn,
                                   const char *value, garry_i32 vlen);

/**
 * Append a tombstone entry to a chain page.
 *
 * @param buf Page buffer of the version chain.
 * @param page_size Size of the page in bytes.
 * @param txn Transaction ID that created the tombstone.
 * @return GARRY_TRUE on success, GARRY_FALSE on failure.
 */
garry_bool garry_chain_page_append_tombstone(garry_page_buffer buf, garry_u32 page_size,
                                             garry_txn_id txn);

/**
 * Append an overflow version entry to a chain page.
 *
 * @param buf Page buffer of the version chain.
 * @param page_size Size of the page in bytes.
 * @param txn Transaction ID that created this version.
 * @param total_len Total length of the overflow value in bytes.
 * @param head Page ID of the first overflow chunk.
 * @return GARRY_TRUE on success, GARRY_FALSE on failure.
 */
garry_bool garry_chain_page_append_overflow(garry_page_buffer buf, garry_u32 page_size,
                                            garry_txn_id txn, garry_i32 total_len, garry_i32 head);

/**
 * Return the number of value bytes that fit in a single overflow chunk.
 *
 * @param page_size Size of the page in bytes.
 * @return Usable bytes per overflow chunk.
 */
garry_i32 garry_overflow_chunk_size(garry_u32 page_size);

/**
 * Return the maximum inline value length for a version chain entry.
 *
 * @param page_size Size of the page in bytes.
 * @return Maximum inline value length in bytes.
 */
garry_i32 garry_chain_inline_capacity(garry_u32 page_size);

/**
 * Write a value to a chain of overflow pages.
 *
 * @param pool Buffer pool to allocate pages from.
 * @param value Value bytes to write.
 * @param vlen Length of the value in bytes.
 * @return Head page ID of the overflow chain, or -1 on failure.
 */
garry_i32 garry_overflow_write(garry_buffer_pool *pool, const char *value, garry_i32 vlen);

/**
 * Read a value from a chain of overflow pages.
 *
 * @param pool Buffer pool to read pages from.
 * @param head Page ID of the first overflow chunk.
 * @param total_len Total number of bytes to read.
 * @param out_buf Destination buffer (must be at least total_len bytes).
 * @return GARRY_TRUE if all bytes were read, GARRY_FALSE on failure.
 */
garry_bool garry_overflow_read(garry_buffer_pool *pool, garry_i32 head, garry_i32 total_len,
                               char *out_buf);
/**
 * Find the most recent version visible to a given snapshot.
 *
 * Scans the chain backward (newest to oldest). Returns a malloc'd
 * copy of the value, or NULL if the latest visible version is a
 * tombstone or no version exists. Caller must free() the result.
 *
 * @param pool Buffer pool (used to read overflow pages).
 * @param buf Page buffer of the version chain.
 * @param page_size Size of the page in bytes.
 * @param snap_txid Transaction ID acting as the snapshot.
 * @param active Array of active transaction IDs.
 * @param active_count Number of entries in the active array.
 * @param vlen Output parameter receiving the value length in bytes.
 * @return Malloc'd visible value, or NULL if not found or deleted.
 */
char *garry_chain_page_find_visible(garry_buffer_pool *pool, garry_page_buffer buf,
                                    garry_u32 page_size, garry_txn_id snap_txid,
                                    garry_txn_id_ptr active, garry_i32 active_count,
                                    garry_i32 *vlen);

/**
 * Lightweight check for a visible non-tombstone entry.
 *
 * Like garry_chain_page_find_visible but does not allocate or copy
 * the value. Returns GARRY_TRUE if a visible, non-tombstone entry
 * exists on the chain page.
 */
garry_bool garry_chain_page_has_visible(garry_buffer_pool *pool, garry_page_buffer buf,
                                        garry_u32 page_size, garry_txn_id snap_txid,
                                        garry_txn_id_ptr active, garry_i32 active_count);
/**
 * Free all overflow pages in a chain.
 *
 * Walks the linked list starting at head and releases each page back
 * to the buffer pool.
 *
 * @param pool Buffer pool owning the overflow pages.
 * @param head Page ID of the first overflow chunk, or -1 for none.
 */
void garry_overflow_free(garry_buffer_pool *pool, garry_i32 head);
/**
 * Prune old versions invisible to all active transactions.
 *
 * Keeps the latest version unconditionally. Copies surviving entries
 * into a fresh page buffer and overwrites the original. Frees overflow
 * pages from discarded entries.
 *
 * @param pool Buffer pool (used to free overflow pages). May be NULL to skip freeing.
 * @param buf Page buffer of the version chain to prune (modified in place).
 * @param page_size Size of the page in bytes.
 * @param active Array of active transaction IDs for visibility checks.
 * @param active_count Number of entries in the active array.
 */
void garry_chain_page_prune(garry_buffer_pool *pool, garry_page_buffer buf, garry_u32 page_size,
                            garry_txn_id_ptr active, garry_i32 active_count);

/**
 * Check whether any version in the chain was created by a given transaction.
 *
 * @param buf Page buffer of the version chain.
 * @param page_size Size of the page in bytes.
 * @param txn Transaction ID to search for.
 * @return GARRY_TRUE if a matching version exists, GARRY_FALSE otherwise.
 */
garry_bool garry_chain_page_has_version(garry_page_buffer buf, garry_u32 page_size,
                                        garry_txn_id txn);

#endif /* GARRY_VERSION_CHAIN_H */
