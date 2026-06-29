/**
 * @file config.h
 * @brief Compile-time constants for the Garry storage engine.
 *
 * Centralizes every numeric limit, page layout constant, and version-chain
 * tuning parameter. Changes here affect the on-disk format and must be
 * coordinated with migration logic.
 *
 * Intent: Provide a single place to tune storage-engine limits without
 * touching application code.
 * Rationale: Grouping constants avoids magic numbers scattered across the
 * codebase and makes it clear which values are architecturally significant.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_CONFIG_H
#define GARRY_CONFIG_H

/* ---- Pool / slot limits ---- */

/// Maximum number of pages in the in-memory pool.
#define GARRY_MAX_POOL_SIZE          8
/// Maximum keys a single B-tree node may hold before splitting.
#define GARRY_MAX_KEYS_PER_NODE     3
/// Reserved slot index for page metadata (never used for user data).
#define GARRY_META_SLOT              0
/// Bias added to physical page IDs to distinguish them from logical IDs.
#define GARRY_PAGE_ID_BIAS      100000
/// Maximum size of a user-supplied key in bytes.
#define GARRY_MAX_KEY_SIZE         256
/// Maximum number of subscripts (path components) in a composite key.
#define GARRY_MAX_SUBSCRIPTS        16
/// Maximum size of a single record (key + value) in bytes.
#define GARRY_MAX_RECORD_SIZE     8192
/// Maximum page size in bytes (must be a power of two).
#define GARRY_MAX_PAGE_SIZE     65536
/// Size of the header region at the start of every page (in bytes).
#define GARRY_PAGE_HEADER_SIZE      32
/// Maximum number of slots per page.
#define GARRY_MAX_SLOTS            128
/// Size of a single slot entry in bytes.
#define GARRY_SLOT_SIZE              4

/* ---- Version chain constants ---- */

/// Size of a version-chain entry header in bytes.
#define GARRY_CHAIN_ENTRY_HEADER_SIZE 14
/// Flag indicating a version-chain entry points to an overflow page.
#define GARRY_CHAIN_FLAG_OVERFLOW     1
/// Maximum number of concurrently active version-chain entries.
#define GARRY_CHAIN_MAX_ACTIVE        4

/* ---- garry_data() return values ---- */

/// @ref garry_data returned this when the key was not found.
#define GARRY_DATA_NOT_FOUND          0
/// @ref garry_data returned this when the key has child nodes but no value.
#define GARRY_DATA_HAS_CHILDREN       1
/// @ref garry_data returned this when the key has a value but no children.
#define GARRY_DATA_HAS_VALUE         10
/// @ref garry_data returned this when the key has both a value and children.
#define GARRY_DATA_HAS_BOTH          11

#endif /* GARRY_CONFIG_H */
