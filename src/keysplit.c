/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file keysplit.c
 * @brief Delimiter-based key splitting and rejoining.
 *
 * Splits human-readable string keys on a delimiter character and
 * encodes each component as a length-prefixed byte string for the
 * tuple key encoding. Also provides inverse operations for display
 * and debugging.
 */

#include "garry/keysplit.h"
#include "garry/config.h"
#include <string.h>

garry_i32 garry_key_split(const char *str, char delimiter,
                          garry_byte_array out)
{
    garry_i32 count;
    garry_i32 seg_len;
    const char *p;
    garry_i32 pos;

    if (!str) return 0;

    count = 0;
    pos = 0;
    p = str;

    while (*p && count < GARRY_MAX_SUBSCRIPTS) {
        seg_len = 0;
        while (*p && *p != delimiter && seg_len < 255) {
            p++;
            seg_len++;
        }

        if (pos + 1 + seg_len > GARRY_MAX_KEY_SIZE) break;

        out[pos] = (garry_byte)seg_len;
        pos++;
        memcpy(&out[pos], p - seg_len, (size_t)seg_len);
        pos += seg_len;
        count++;

        if (*p == delimiter) p++;
    }

    return pos;
}

garry_i32 garry_key_unsplit(const garry_byte *key, garry_i32 klen,
                            char delimiter, char *out, garry_i32 out_size)
{
    garry_i32 count;
    garry_i32 pos;
    garry_i32 seg_len;
    garry_i32 out_pos;

    if (!key || klen <= 0 || !out || out_size <= 0) return 0;

    count = 0;
    pos = 0;
    out_pos = 0;

    while (pos < klen) {
        if (pos + 1 > klen) break;
        seg_len = (garry_i32)key[pos];
        pos++;

        if (pos + seg_len > klen) break;
        if (out_pos > 0) {
            if (out_pos >= out_size) break;
            out[out_pos] = delimiter;
            out_pos++;
        }
        if (out_pos + seg_len > out_size) break;
        memcpy(&out[out_pos], &key[pos], (size_t)seg_len);
        out_pos += seg_len;
        pos += seg_len;
        count++;
    }

    if (out_pos < out_size) {
        out[out_pos] = '\0';
    }

    return count;
}

garry_i32 garry_make_key(const char *str, garry_byte_array out)
{
    garry_i32 len;
    garry_i32 pos;

    if (!str) return 0;

    len = 0;
    while (str[len] && len < 255) {
        len++;
    }

    if (1 + len > GARRY_MAX_KEY_SIZE) return 0;

    pos = 0;
    out[pos] = (garry_byte)len;
    pos++;
    memcpy(&out[pos], str, (size_t)len);
    pos += len;

    return 1;
}

garry_i32 garry_make_key_parts(const char **parts, garry_i32 count,
                               garry_byte_array out)
{
    garry_i32 i;
    garry_i32 pos;
    garry_i32 seg_len;

    if (!parts || count <= 0) return 0;

    pos = 0;

    for (i = 0; i < count && i < GARRY_MAX_SUBSCRIPTS; i++) {
        if (!parts[i]) return 0;

        seg_len = 0;
        while (parts[i][seg_len] && seg_len < 255) {
            seg_len++;
        }

        if (pos + 1 + seg_len > GARRY_MAX_KEY_SIZE) break;

        out[pos] = (garry_byte)seg_len;
        pos++;
        memcpy(&out[pos], parts[i], (size_t)seg_len);
        pos += seg_len;
    }

    return pos;
}
