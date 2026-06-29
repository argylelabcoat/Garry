/**
 * @file iterator.h
 * @brief Callback-based prefix iteration for the Garry storage engine.
 *
 * Provides @ref garry_for_each — a single function that walks all
 * key-value pairs matching a prefix and invokes a user-supplied visitor
 * callback for each entry.
 *
 * Intent: Offer a functional-style iteration API as an alternative to
 * cursor-based iteration (see @ref garry_cursor).
 * Rationale: Some callers prefer a push-based callback model over
 * pull-based cursor iteration; this avoids cursor lifecycle management.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_ITERATOR_H
#define GARRY_ITERATOR_H

#include "garry/export.h"
#include "garry/types.h"

/// Forward declaration — defined in database.h.
#ifndef GARRY_DATABASE_FWD_DEFINED
#define GARRY_DATABASE_FWD_DEFINED
typedef struct garry_database garry_database;
#endif

/**
 * @brief Visitor callback invoked for each key-value pair during iteration.
 * @param key       Pointer to the key bytes (valid only for this callback).
 * @param klen      Length of @p key in bytes.
 * @param val       Pointer to the value bytes (valid only for this callback).
 * @param vlen      Length of @p val in bytes.
 * @param user_data  Opaque pointer passed through from @ref garry_for_each.
 *
 * The key and value pointers are only valid for the duration of this
 * callback. Copy them if you need them beyond the callback scope.
 */
typedef void (*garry_visitor)(const garry_u8 *key, garry_i32 klen, const garry_u8 *val,
                              garry_i32 vlen, void *user_data);

/**
 * @brief Iterate over all key-value pairs matching a prefix.
 * @param db        Database handle.
 * @param txn       Active transaction handle.
 * @param prefix    Pointer to the prefix bytes (may be @c NULL for all keys).
 * @param plen      Length of @p prefix in bytes (0 if @p prefix is @c NULL).
 * @param visitor   Callback function invoked for each matching pair.
 * @param user_data  Opaque pointer forwarded to @p visitor.
 *
 * Walks the B-tree in lexicographic order and calls @p visitor for every
 * key that starts with @p prefix. Pass @c NULL / 0 for the prefix to
 * iterate over every key in the database.
 *
 * @note Do not call @ref garry_set or @ref garry_delete from within the
 *       visitor — this may corrupt the iteration. Read-only operations
 *       (e.g. @ref garry_get) are safe.
 */
GARRY_API void garry_for_each(garry_database *db, garry_txn txn, const garry_u8 *prefix,
                              garry_i32 plen, garry_visitor visitor, void *user_data);

#endif /* GARRY_ITERATOR_H */
