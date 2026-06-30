/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file db_header.c
 * @brief Database file header creation, serialization, and validation.
 *
 * The header occupies the first page of the database file and stores
 * magic number, format version, page size, root page ID, free page
 * count, and a CRC32 checksum for corruption detection.
 */

#include "db_header.h"
#include "util_endian.h"
#include <string.h>

/**
 * @brief Write a 32-bit little-endian integer to a buffer.
 *
 * @param buf    Destination buffer.
 * @param offset Byte offset to write at.
 * @param val    32-bit value to write.
 */
void garry_write_int32(garry_byte *buf, garry_i32 offset, garry_i32 val)
{
    garry_write_le32(buf, offset, val);
}

/**
 * @brief Read a 32-bit little-endian integer from a buffer.
 *
 * @param buf    Source buffer.
 * @param offset Byte offset to read from.
 *
 * @return The decoded 32-bit integer.
 */
garry_i32 garry_read_int32(garry_byte *buf, garry_i32 offset)
{
    return garry_read_le32((const garry_byte *)buf, offset);
}

/**
 * @brief Create a new database header from engine settings.
 *
 * Populates header fields with magic, version, page size, and limits
 * from the settings. Returns a zeroed header if page size is invalid.
 *
 * @param settings Engine settings specifying database configuration.
 *
 * @return Initialized database header.
 */
garry_db_header garry_create_db_header(garry_engine_settings *settings)
{
    garry_db_header hdr;
    if (settings->page_size >= GARRY_MIN_PAGE_SIZE && settings->page_size <= GARRY_MAX_PAGE_SIZE &&
        settings->page_size % GARRY_MIN_PAGE_SIZE == 0)
    {
        hdr.magic = GARRY_DB_MAGIC;
        hdr.version = GARRY_DB_VERSION;
        hdr.page_size = settings->page_size;
        hdr.compression = settings->compression;
        hdr.root_page = 1;
        hdr.free_list_head = -1;
        hdr.total_pages = 1;
        hdr.max_txns = settings->max_txns;
        hdr.max_versions = settings->max_versions;
        hdr.max_key_size = settings->max_key_size;
        hdr.max_subscripts = settings->max_subscripts;
        hdr.btree_flags = 0;
        hdr.checksum = 0;
    }
    else
    {
        memset(&hdr, 0, sizeof(hdr));
    }
    return hdr;
}

/**
 * @brief Create a database header from settings (alias for garry_create_db_header).
 *
 * @param settings Engine settings specifying database configuration.
 *
 * @return Initialized database header.
 */
garry_db_header garry_make_db_header_from_settings(garry_engine_settings *settings)
{
    return garry_create_db_header(settings);
}

/**
 * @brief Serialize a database header to a byte buffer.
 *
 * Writes all header fields as little-endian 32-bit integers and
 * computes and appends the FNV-1a checksum.
 *
 * @param buf Destination buffer for the serialized header.
 * @param hdr Header struct to serialize.
 */
void garry_write_db_header(garry_byte *buf, garry_db_header *hdr)
{
    garry_i32 cs;
    garry_write_int32(buf, GARRY_HDR_OFF_MAGIC, hdr->magic);
    garry_write_int32(buf, GARRY_HDR_OFF_VERSION, hdr->version);
    garry_write_int32(buf, GARRY_HDR_OFF_PAGE_SIZE, hdr->page_size);
    garry_write_int32(buf, GARRY_HDR_OFF_COMPRESSION, hdr->compression);
    garry_write_int32(buf, GARRY_HDR_OFF_ROOT_PAGE, hdr->root_page);
    garry_write_int32(buf, GARRY_HDR_OFF_FREE_LIST, hdr->free_list_head);
    garry_write_int32(buf, GARRY_HDR_OFF_TOTAL_PAGES, hdr->total_pages);
    garry_write_int32(buf, GARRY_HDR_OFF_MAX_TXNS, hdr->max_txns);
    garry_write_int32(buf, GARRY_HDR_OFF_MAX_VERSIONS, hdr->max_versions);
    garry_write_int32(buf, GARRY_HDR_OFF_MAX_KEY_SIZE, hdr->max_key_size);
    garry_write_int32(buf, GARRY_HDR_OFF_MAX_SUBSCRIPTS, hdr->max_subscripts);
    garry_write_int32(buf, GARRY_HDR_OFF_BTREE_FLAGS, hdr->btree_flags);
    cs = garry_compute_header_checksum(buf);
    garry_write_int32(buf, GARRY_HDR_OFF_CHECKSUM, cs);
}

/**
 * @brief Deserialize a database header from a byte buffer.
 *
 * Reads all header fields and validates the checksum. If the checksum
 * does not match, the magic field is set to 0 to signal corruption.
 *
 * @param buf Source buffer containing the serialized header.
 *
 * @return Deserialized header struct.
 */
garry_db_header garry_read_db_header(garry_byte *buf)
{
    garry_db_header hdr;
    garry_i32 stored_cs, computed_cs;
    hdr.magic = garry_read_int32(buf, GARRY_HDR_OFF_MAGIC);
    hdr.version = garry_read_int32(buf, GARRY_HDR_OFF_VERSION);
    hdr.page_size = garry_read_int32(buf, GARRY_HDR_OFF_PAGE_SIZE);
    hdr.compression = garry_read_int32(buf, GARRY_HDR_OFF_COMPRESSION);
    hdr.root_page = garry_read_int32(buf, GARRY_HDR_OFF_ROOT_PAGE);
    hdr.free_list_head = garry_read_int32(buf, GARRY_HDR_OFF_FREE_LIST);
    hdr.total_pages = garry_read_int32(buf, GARRY_HDR_OFF_TOTAL_PAGES);
    hdr.max_txns = garry_read_int32(buf, GARRY_HDR_OFF_MAX_TXNS);
    hdr.max_versions = garry_read_int32(buf, GARRY_HDR_OFF_MAX_VERSIONS);
    hdr.max_key_size = garry_read_int32(buf, GARRY_HDR_OFF_MAX_KEY_SIZE);
    hdr.max_subscripts = garry_read_int32(buf, GARRY_HDR_OFF_MAX_SUBSCRIPTS);
    hdr.btree_flags = garry_read_int32(buf, GARRY_HDR_OFF_BTREE_FLAGS);
    hdr.checksum = garry_read_int32(buf, GARRY_HDR_OFF_CHECKSUM);
    stored_cs = hdr.checksum;
    computed_cs = garry_compute_header_checksum(buf);
    if (stored_cs != computed_cs)
    {
        hdr.magic = 0;
    }
    return hdr;
}

/**
 * @brief Validate a database header.
 *
 * Checks magic number, version, page size bounds, and positive
 * limits for max_txns and max_versions.
 *
 * @param hdr Header struct to validate.
 *
 * @return 1 if valid, 0 if any check fails.
 */
garry_bool garry_validate_db_header(garry_db_header *hdr)
{
    if (hdr->magic != GARRY_DB_MAGIC)
        return 0;
    if (hdr->version != GARRY_DB_VERSION)
        return 0;
    if (hdr->page_size < GARRY_MIN_PAGE_SIZE)
        return 0;
    if (hdr->page_size > GARRY_MAX_PAGE_SIZE)
        return 0;
    if (hdr->max_txns <= 0)
        return 0;
    if (hdr->max_versions <= 0)
        return 0;
    return 1;
}

/**
 * @brief Compute the FNV-1a checksum of the database header.
 *
 * Hashes the first GARRY_HEADER_CHECKSUM_LEN bytes of the buffer
 * using FNV-1a and masks to GARRY_CHECKSUM_MASK.
 *
 * @param buf Buffer containing the serialized header.
 *
 * @return 32-bit checksum value.
 */
garry_i32 garry_compute_header_checksum(garry_byte *buf)
{
    garry_u32 hash = GARRY_FNV1A_OFFSET_BASIS;
    garry_i32 i;
    for (i = 0; i < GARRY_HEADER_CHECKSUM_LEN; i++)
    {
        hash ^= (garry_u32)buf[i];
        hash *= GARRY_FNV1A_PRIME;
    }
    return (garry_i32)(hash & GARRY_CHECKSUM_MASK);
}
