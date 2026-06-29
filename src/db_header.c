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
#include <string.h>

void garry_write_int32(garry_byte* buf, garry_i32 offset, garry_i32 val)
{
    buf[offset]     = (garry_byte)(val % 256);
    buf[offset + 1] = (garry_byte)((val >> 8) % 256);
    buf[offset + 2] = (garry_byte)((val >> 16) % 256);
    buf[offset + 3] = (garry_byte)((val >> 24) % 256);
}

garry_i32 garry_read_int32(garry_byte* buf, garry_i32 offset)
{
    garry_i32 b0, b1, b2, b3;
    b0 = (garry_i32)buf[offset];
    b1 = (garry_i32)buf[offset + 1];
    b2 = (garry_i32)buf[offset + 2];
    b3 = (garry_i32)buf[offset + 3];
    if (b0 < 0) b0 += 256;
    if (b1 < 0) b1 += 256;
    if (b2 < 0) b2 += 256;
    if (b3 < 0) b3 += 256;
    return b0 + (b1 << 8) + (b2 << 16) + (b3 << 24);
}

garry_db_header garry_create_db_header(garry_engine_settings* settings)
{
    garry_db_header hdr;
    if (settings->page_size >= GARRY_MIN_PAGE_SIZE &&
        settings->page_size <= GARRY_MAX_PAGE_SIZE &&
        settings->page_size % GARRY_MIN_PAGE_SIZE == 0) {
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
    } else {
        memset(&hdr, 0, sizeof(hdr));
    }
    return hdr;
}

garry_db_header garry_make_db_header_from_settings(garry_engine_settings* settings)
{
    return garry_create_db_header(settings);
}

void garry_write_db_header(garry_byte* buf, garry_db_header* hdr)
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

garry_db_header garry_read_db_header(garry_byte* buf)
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
    if (stored_cs != computed_cs) {
        hdr.magic = 0;
    }
    return hdr;
}

garry_bool garry_validate_db_header(garry_db_header* hdr)
{
    if (hdr->magic != GARRY_DB_MAGIC) return 0;
    if (hdr->version != GARRY_DB_VERSION) return 0;
    if (hdr->page_size < GARRY_MIN_PAGE_SIZE) return 0;
    if (hdr->page_size > GARRY_MAX_PAGE_SIZE) return 0;
    if (hdr->max_txns <= 0) return 0;
    if (hdr->max_versions <= 0) return 0;
    return 1;
}

garry_i32 garry_compute_header_checksum(garry_byte* buf)
{
    garry_u32 hash = GARRY_FNV1A_OFFSET_BASIS;
    garry_i32 i;
    for (i = 0; i < GARRY_HEADER_CHECKSUM_LEN; i++) {
        hash ^= (garry_u32)buf[i];
        hash *= GARRY_FNV1A_PRIME;
    }
    return (garry_i32)(hash & GARRY_CHECKSUM_MASK);
}
