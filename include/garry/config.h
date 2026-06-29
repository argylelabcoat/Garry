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
#define GARRY_MAX_POOL_SIZE 8
/// Maximum keys a single B-tree node may hold before splitting.
#define GARRY_MAX_KEYS_PER_NODE 3
/// Minimum keys a B-tree node may hold (underflow threshold).
#define GARRY_BTREE_MIN_KEYS 1
/// Reserved slot index for page metadata (never used for user data).
#define GARRY_META_SLOT 0
/// Bias added to physical page IDs to distinguish them from logical IDs.
#define GARRY_PAGE_ID_BIAS 100000
/// First usable page ID (after header page).
#define GARRY_FIRST_PAGE_ID 2
/// Maximum size of a user-supplied key in bytes.
#define GARRY_MAX_KEY_SIZE 256
/// Maximum length of a single key-segment in bytes.
#define GARRY_MAX_SEGMENT_LEN 255
/// Maximum number of subscripts (path components) in a composite key.
#define GARRY_MAX_SUBSCRIPTS 16
/// Maximum size of a single record (key + value) in bytes.
#define GARRY_MAX_RECORD_SIZE 16384
/// Size of an overflow next-page pointer in bytes.
#define GARRY_OVERFLOW_PTR_SIZE 4

/* ---- Page size limits ---- */

/// Minimum page size in bytes.
#define GARRY_MIN_PAGE_SIZE 512
/// Maximum page size in bytes (must be a power of two).
#define GARRY_MAX_PAGE_SIZE 65536
/// Default page size in bytes.
#define GARRY_DEFAULT_PAGE_SIZE 4096
/// Size of the header region at the start of every page (in bytes).
#define GARRY_PAGE_HEADER_SIZE 32
/// Maximum number of slots per page.
#define GARRY_MAX_SLOTS 128
/// Size of a single slot entry in bytes.
#define GARRY_SLOT_SIZE 4

/* ---- Page header field offsets ---- */

/// Byte offset of the page type field in the page header.
#define GARRY_PAGE_HDR_OFF_TYPE 0
/// Byte offset of the B-tree level field.
#define GARRY_PAGE_HDR_OFF_LEVEL 4
/// Byte offset of the record count field.
#define GARRY_PAGE_HDR_OFF_COUNT 8
/// Byte offset of the free-start pointer.
#define GARRY_PAGE_HDR_OFF_FREE_START 12
/// Byte offset of the free-end pointer.
#define GARRY_PAGE_HDR_OFF_FREE_END 16
/// Byte offset of the compression field.
#define GARRY_PAGE_HDR_OFF_COMPRESSION 20
/// Byte offset of the parent page pointer.
#define GARRY_PAGE_HDR_OFF_PARENT 24
/// Byte offset of the checksum field.
#define GARRY_PAGE_HDR_OFF_CHECKSUM 28

/* ---- Engine defaults ---- */

/// Default number of pages in the in-memory pool.
#define GARRY_DEFAULT_POOL_SIZE 8
/// Default maximum concurrent transactions.
#define GARRY_DEFAULT_MAX_TXNS 4
/// Default maximum version-chain depth per key.
#define GARRY_DEFAULT_MAX_VERSIONS 64
/// Default maximum size of a user-supplied key in bytes.
#define GARRY_DEFAULT_MAX_KEY_SIZE 256
/// Default maximum number of subscripts in a composite key.
#define GARRY_DEFAULT_MAX_SUBSCRIPTS 16
/// Default maximum size of a single record in bytes.
#define GARRY_DEFAULT_MAX_RECORD_SIZE 16384

/* ---- Buffer sizes ---- */

/// Size of a stack-allocated B-tree lookup buffer.
#define GARRY_LOOKUP_BUF_SIZE 512
/// Size of a stack-allocated descriptor encode/decode buffer.
#define GARRY_DESC_BUF_SIZE 64
/// Size of a version-chain entry stack buffer.
#define GARRY_CHAIN_ENTRY_BUF_SIZE 512
/// Sentinel value for "no slot found" in buffer pool.
#define GARRY_INVALID_SLOT ((garry_u32) - 1)

/* ---- Version chain constants ---- */

/// Size of a version-chain entry header in bytes.
#define GARRY_CHAIN_ENTRY_HEADER_SIZE 14
/// Flag indicating a version-chain entry points to an overflow page.
#define GARRY_CHAIN_FLAG_OVERFLOW 1
/// Maximum number of concurrently active version-chain entries.
#define GARRY_CHAIN_MAX_ACTIVE 4
/// Maximum number of versions that fit on a single chain page.
#define GARRY_MAX_VERSIONS_PER_PAGE 128
/// Minimum record length for a version-chain header.
#define GARRY_CHAIN_PRUNE_MIN_LEN 8

/* ---- Length-prefix encoding ---- */

/// Maximum length that fits in a 1-byte length prefix.
#define GARRY_LEN_PREFIX_INLINE_MAX 128
/// Marker byte for long-form (3-byte) length prefix.
#define GARRY_LEN_PREFIX_LONG_MARKER 255

/* ---- garry_data() return values ---- */

/// @ref garry_data returned this when the key was not found.
#define GARRY_DATA_NOT_FOUND 0
/// @ref garry_data returned this when the key has child nodes but no value.
#define GARRY_DATA_HAS_CHILDREN 1
/// @ref garry_data returned this when the key has a value but no children.
#define GARRY_DATA_HAS_VALUE 10
/// @ref garry_data returned this when the key has both a value and children.
#define GARRY_DATA_HAS_BOTH 11

/* ---- DB header layout ---- */

/// Byte offset of the magic field in the database header.
#define GARRY_HDR_OFF_MAGIC 0
/// Byte offset of the version field.
#define GARRY_HDR_OFF_VERSION 4
/// Byte offset of the page size field.
#define GARRY_HDR_OFF_PAGE_SIZE 8
/// Byte offset of the compression field.
#define GARRY_HDR_OFF_COMPRESSION 12
/// Byte offset of the root page field.
#define GARRY_HDR_OFF_ROOT_PAGE 16
/// Byte offset of the free list head field.
#define GARRY_HDR_OFF_FREE_LIST 20
/// Byte offset of the total pages field.
#define GARRY_HDR_OFF_TOTAL_PAGES 24
/// Byte offset of the max_txns field.
#define GARRY_HDR_OFF_MAX_TXNS 28
/// Byte offset of the max_versions field.
#define GARRY_HDR_OFF_MAX_VERSIONS 32
/// Byte offset of the max_key_size field.
#define GARRY_HDR_OFF_MAX_KEY_SIZE 36
/// Byte offset of the max_subscripts field.
#define GARRY_HDR_OFF_MAX_SUBSCRIPTS 40
/// Byte offset of the btree_flags field.
#define GARRY_HDR_OFF_BTREE_FLAGS 44
/// Byte offset of the checksum field.
#define GARRY_HDR_OFF_CHECKSUM 48
/// Number of bytes covered by the header checksum.
#define GARRY_HEADER_CHECKSUM_LEN 48

/* ---- FNV-1a hash constants ---- */

/// FNV-1a offset basis for header checksum.
#define GARRY_FNV1A_OFFSET_BASIS 2166136261u
/// FNV-1a prime for header checksum.
#define GARRY_FNV1A_PRIME 16777619u
/// Mask to ensure checksum is a positive i32.
#define GARRY_CHECKSUM_MASK 0x7FFFFFFFu

#endif /* GARRY_CONFIG_H */
