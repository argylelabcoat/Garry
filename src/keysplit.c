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

/**
 * @brief Split a delimiter-separated string into length-prefixed segments.
 *
 * Parses @p str on @p delimiter and encodes each segment as a
 * one-byte length prefix followed by the segment bytes.
 *
 * @param str        Input string to split, or NULL.
 * @param delimiter  Character used to separate segments.
 * @param out        Output buffer for encoded segments.
 * @return Total bytes written to @p out, or 0 if @p str is NULL.
 */
garry_i32 garry_key_split(const char *str, char delimiter, garry_byte_array out)
{
    garry_i32 count;
    garry_i32 seg_len;
    const char *p;
    garry_i32 pos;

    if (!str)
        return 0;

    count = 0;
    pos = 0;
    p = str;

    while (*p && count < GARRY_MAX_SUBSCRIPTS)
    {
        seg_len = 0;
        while (*p && *p != delimiter && seg_len < GARRY_MAX_SEGMENT_LEN)
        {
            p++;
            seg_len++;
        }

        if (pos + 1 + seg_len > GARRY_MAX_KEY_SIZE)
            break;

        out[pos] = (garry_byte)seg_len;
        pos++;
        memcpy(&out[pos], p - seg_len, (size_t)seg_len);
        pos += seg_len;
        count++;

        if (*p == delimiter)
            p++;
    }

    return pos;
}

/**
 * @brief Rejoin length-prefixed segments into a delimiter-separated string.
 *
 * Reads length-prefixed segments from @p key and writes them to @p out
 * separated by @p delimiter. Null-terminates the output.
 *
 * @param key        Encoded key with length-prefixed segments.
 * @param klen       Length of @p key.
 * @param delimiter  Character to insert between segments.
 * @param out        Output string buffer.
 * @param out_size   Size of the output buffer.
 * @return Number of segments rejoined, or 0 on invalid input.
 */
garry_i32 garry_key_unsplit(const garry_byte *key, garry_i32 klen, char delimiter, char *out,
                            garry_i32 out_size)
{
    garry_i32 count;
    garry_i32 pos;
    garry_i32 seg_len;
    garry_i32 out_pos;

    if (!key || klen <= 0 || !out || out_size <= 0)
        return 0;

    count = 0;
    pos = 0;
    out_pos = 0;

    while (pos < klen)
    {
        if (pos + 1 > klen)
            break;
        seg_len = (garry_i32)key[pos];
        pos++;

        if (pos + seg_len > klen)
            break;
        if (out_pos > 0)
        {
            if (out_pos >= out_size)
                break;
            out[out_pos] = delimiter;
            out_pos++;
        }
        if (out_pos + seg_len > out_size)
            break;
        memcpy(&out[out_pos], &key[pos], (size_t)seg_len);
        out_pos += seg_len;
        pos += seg_len;
        count++;
    }

    if (out_pos < out_size)
    {
        out[out_pos] = '\0';
    }

    return count;
}

/**
 * @brief Encode a single string as a length-prefixed byte sequence.
 *
 * Writes a one-byte length prefix followed by the string bytes.
 *
 * @param str  Input string to encode, or NULL.
 * @param out  Output buffer for the encoded key.
 * @return Total bytes written, or 0 if @p str is NULL.
 */
garry_i32 garry_make_key(const char *str, garry_byte_array out)
{
    garry_i32 len;
    garry_i32 pos;

    if (!str)
        return 0;

    len = 0;
    while (str[len] && len < GARRY_MAX_SEGMENT_LEN)
    {
        len++;
    }

    if (1 + len > GARRY_MAX_KEY_SIZE)
        return 0;

    pos = 0;
    out[pos] = (garry_byte)len;
    pos++;
    memcpy(&out[pos], str, (size_t)len);
    pos += len;

    return pos;
}

/**
 * @brief Encode an array of string parts as a multi-segment key.
 *
 * Each part is encoded with a one-byte length prefix followed by its bytes.
 *
 * @param parts  Array of input strings.
 * @param count  Number of parts.
 * @param out    Output buffer for the encoded key.
 * @return Total bytes written, or 0 on invalid input.
 */
garry_i32 garry_make_key_parts(const char **parts, garry_i32 count, garry_byte_array out)
{
    garry_i32 i;
    garry_i32 pos;
    garry_i32 seg_len;

    if (!parts || count <= 0)
        return 0;

    pos = 0;

    for (i = 0; i < count && i < GARRY_MAX_SUBSCRIPTS; i++)
    {
        if (!parts[i])
            return 0;

        seg_len = 0;
        while (parts[i][seg_len] && seg_len < GARRY_MAX_SEGMENT_LEN)
        {
            seg_len++;
        }

        if (pos + 1 + seg_len > GARRY_MAX_KEY_SIZE)
            break;

        out[pos] = (garry_byte)seg_len;
        pos++;
        memcpy(&out[pos], parts[i], (size_t)seg_len);
        pos += seg_len;
    }

    return pos;
}
