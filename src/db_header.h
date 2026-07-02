/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file db_header.h
 * @brief Database file header: magic, version, page size, root pointer.
 */

#ifndef GARRY_DB_HEADER_H
#define GARRY_DB_HEADER_H

#include "garry/types.h"
#include "storage_core.h"
#include "engine_settings.h"

#define GARRY_DB_MAGIC           0x52435350
#define GARRY_DB_VERSION         2
#define GARRY_HEADER_PAGE        0
#define GARRY_HEADER_FIELD_COUNT 14
#define GARRY_HEADER_SIZE        56
#define GARRY_CHECKSUM_OFFSET    52

typedef struct
{
    garry_i32 magic;
    garry_i32 version;
    garry_i32 page_size;
    garry_i32 compression;
    garry_i32 root_page;
    garry_i32 free_list_head;
    garry_i32 total_pages;
    garry_i32 max_txns;
    garry_i32 max_versions;
    garry_i32 max_key_size;
    garry_i32 max_subscripts;
    garry_i32 btree_flags;
    garry_i32 next_txid;
    garry_i32 checksum;
} garry_db_header;

/**
 * @brief Write a 32-bit integer to a buffer in little-endian byte order.
 *
 * @param buf     Output buffer
 * @param offset  Byte offset where integer is written
 * @param val     32-bit integer value to write
 */
void garry_write_int32(garry_byte *buf, garry_i32 offset, garry_i32 val);

/**
 * @brief Read a 32-bit integer from a buffer in little-endian byte order.
 *
 * @param buf     Input buffer
 * @param offset  Byte offset where integer is stored
 * @return Decoded 32-bit integer value
 */
garry_i32 garry_read_int32(garry_byte *buf, garry_i32 offset);

/**
 * @brief Create a database header from engine settings.
 *
 * Populates header fields (magic, version, page size, etc.) from the
 * given settings. Computes the header checksum.
 *
 * @param settings  Engine settings to populate header from
 * @return Initialized database header
 */
garry_db_header garry_create_db_header(garry_engine_settings *settings);

/**
 * @brief Create a database header from engine settings (alias).
 *
 * Equivalent to garry_create_db_header. Provided for naming consistency.
 *
 * @param settings  Engine settings to populate header from
 * @return Initialized database header
 */
garry_db_header garry_make_db_header_from_settings(garry_engine_settings *settings);

/**
 * @brief Serialize a database header to a byte buffer.
 *
 * Writes all header fields in big-endian order at fixed offsets.
 * The header occupies GARRY_HEADER_SIZE bytes (52 bytes).
 *
 * @param buf  Output buffer (must be at least GARRY_HEADER_SIZE bytes)
 * @param hdr  Header struct to serialize
 */
void garry_write_db_header(garry_byte *buf, garry_db_header *hdr);

/**
 * @brief Deserialize a database header from a byte buffer.
 *
 * Reads all header fields from their fixed offsets. Does not validate
 * the header — use garry_validate_db_header after reading.
 *
 * @param buf  Input buffer containing serialized header
 * @return Deserialized header struct
 */
garry_db_header garry_read_db_header(garry_byte *buf);

/**
 * @brief Validate a database header's magic, version, and checksum.
 *
 * Checks that the magic number matches GARRY_DB_MAGIC, version is
 * supported, and the checksum matches the computed value.
 *
 * @param hdr  Header to validate
 * @return GARRY_TRUE if header is valid
 */
garry_bool garry_validate_db_header(garry_db_header *hdr);

/**
 * @brief Compute the checksum for the first GARRY_CHECKSUM_OFFSET bytes.
 *
 * Uses FNV-1a hash truncated to 31 bits over all bytes before the
 * checksum field. The checksum is stored at GARRY_CHECKSUM_OFFSET (byte 48).
 *
 * @param buf  Buffer containing the header (first 48 bytes used)
 * @return Computed checksum value
 */
garry_i32 garry_compute_header_checksum(garry_byte *buf);

#endif /* GARRY_DB_HEADER_H */
