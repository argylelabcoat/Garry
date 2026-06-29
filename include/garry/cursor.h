/**
 * @file cursor.h
 * @brief Cursor-based iteration for the Garry storage engine.
 *
 * Provides an opaque cursor handle that walks keys within a prefix
 * scope. Cursors support both key+value iteration and key-only
 * iteration, and are automatically positioned after the prefix on open.
 *
 * Intent: Offer efficient prefix-scoped traversal without materializing
 * the full result set into memory.
 * Rationale: Cursors stream results one at a time, making them suitable
 * for large key spaces where a full list would exhaust memory.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_CURSOR_H
#define GARRY_CURSOR_H

#include "garry/export.h"
#include "garry/types.h"

/// Forward declaration — defined in database.h.
typedef struct garry_database garry_database;
/// Opaque cursor handle for prefix-scoped iteration.
typedef struct garry_cursor garry_cursor;

/**
 * @brief Open a cursor scoped to a key prefix.
 * @param db     Database handle.
 * @param txn    Active transaction handle.
 * @param prefix  Pointer to the prefix bytes (may be @c NULL for all keys).
 * @param plen   Length of @p prefix in bytes (0 if @p prefix is @c NULL).
 * @return Cursor handle positioned before the first matching key, or
 *         @c NULL on allocation failure.
 *
 * The cursor will iterate over all keys that start with @p prefix, in
 * lexicographic order. Pass @c NULL / 0 to iterate over every key.
 */
GARRY_API garry_cursor* garry_cursor_open(garry_database *db, garry_txn txn,
                                           const garry_u8 *prefix, garry_i32 plen);

/**
 * @brief Advance the cursor to the next key and read both key and value.
 * @param cur    Cursor handle.
 * @param key    Output buffer for the key.
 * @param klen   [in] capacity of @p key; [out] actual key size.
 * @param value  Output buffer for the value.
 * @param vlen   [in] capacity of @p value; [out] actual value size.
 * @return @ref GARRY_TRUE if a key was read, @ref GARRY_FALSE if the
 *         cursor is exhausted or the prefix scope ended.
 *
 * After @ref garry_cursor_open, the cursor is positioned before the first
 * match — call this function to read the first entry.
 */
GARRY_API garry_bool garry_cursor_next(garry_cursor *cur,
                                        garry_u8 *key, garry_i32 *klen,
                                        garry_u8 *value, garry_i32 *vlen);

/**
 * @brief Advance the cursor to the next key without reading its value.
 * @param cur   Cursor handle.
 * @param key   Output buffer for the key.
 * @param klen  [in] capacity of @p key; [out] actual key size.
 * @return @ref GARRY_TRUE if a key was read, @ref GARRY_FALSE if
 *         the cursor is exhausted.
 *
 * More efficient than @ref garry_cursor_next when only the key is needed
 * (e.g. for counting, existence checks, or key-only scans).
 */
GARRY_API garry_bool garry_cursor_next_key(garry_cursor *cur,
                                            garry_u8 *key, garry_i32 *klen);

/**
 * @brief Close a cursor and release all associated resources.
 * @param cur  Cursor handle obtained from @ref garry_cursor_open.
 *
 * After this call @p cur is invalid and must not be used.
 */
GARRY_API void garry_cursor_close(garry_cursor *cur);

#endif /* GARRY_CURSOR_H */
