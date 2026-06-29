/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file util_endian.h
 * @brief Canonical little-endian byte reading/writing utilities.
 *
 * Provides platform-agnostic LE32/LE16 read/write for on-disk formats.
 * All byte-to-integer conversions for storage go through here.
 */

#ifndef GARRY_UTIL_ENDIAN_H
#define GARRY_UTIL_ENDIAN_H

#include "garry/types.h"

/**
 * @brief Read a 32-bit little-endian unsigned integer from a byte buffer.
 *
 * Handles signed char platforms correctly by masking each byte to
 * unsigned before assembly.
 *
 * @param buf   Source buffer (must have at least offset+4 bytes valid)
 * @param offset Byte offset to read from
 * @return Decoded 32-bit value
 */
#ifdef __GNUC__
static __attribute__((unused)) garry_i32 garry_read_le32(const garry_byte *buf, garry_i32 offset)
#else
static garry_i32 garry_read_le32(const garry_byte *buf, garry_i32 offset)
#endif
{
    garry_i32 b0, b1, b2, b3;
    b0 = (garry_i32)buf[offset];     if (b0 < 0) b0 += 256;
    b1 = (garry_i32)buf[offset + 1]; if (b1 < 0) b1 += 256;
    b2 = (garry_i32)buf[offset + 2]; if (b2 < 0) b2 += 256;
    b3 = (garry_i32)buf[offset + 3]; if (b3 < 0) b3 += 256;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

/**
 * @brief Write a 32-bit integer as little-endian bytes to a buffer.
 *
 * @param buf    Destination buffer (must have at least offset+4 bytes valid)
 * @param offset Byte offset to write to
 * @param val    Value to encode
 */
#ifdef __GNUC__
static __attribute__((unused)) void garry_write_le32(garry_byte *buf, garry_i32 offset, garry_i32 val)
#else
static void garry_write_le32(garry_byte *buf, garry_i32 offset, garry_i32 val)
#endif
{
    buf[offset]     = (garry_byte)(val & 0xFF);
    buf[offset + 1] = (garry_byte)((val >> 8) & 0xFF);
    buf[offset + 2] = (garry_byte)((val >> 16) & 0xFF);
    buf[offset + 3] = (garry_byte)((val >> 24) & 0xFF);
}

#endif /* GARRY_UTIL_ENDIAN_H */
