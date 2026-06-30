/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 *
 * Original LZ4 implementation written for SPARK by Kimi-2.6,
 * ported to C for the Garry storage engine.
 */

/**
 * @file lz4.c
 * @brief LZ4 block compression and decompression.
 *
 * Self-contained LZ4-compatible compressor/decompressor. Produces output
 * compatible with the LZ4 block format. Uses a hash table for LZ77 match
 * finding with a minimum match length of 4 bytes.
 */

#include "lz4.h"
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE 32768
#define MIN_MATCH       4
#define MAX_MATCH       65536

/**
 * @brief Convert a single byte to an unsigned integer value.
 *
 * @param b The byte to convert.
 *
 * @return The byte value as a non-negative integer (0-255).
 */
static int lz4_byte_to_int(char b)
{
    int v = (unsigned char)b;
    return v;
}

/**
 * @brief Compute the maximum compressed output size for a given input length.
 *
 * @param src_len Length of the source data.
 *
 * @return Maximum possible compressed size, or 0 if src_len exceeds LZ4_MAX_INPUT_SIZE.
 */
size_t lz4_compress_bound(size_t src_len)
{
    if (src_len > LZ4_MAX_INPUT_SIZE)
        return 0;
    return src_len + (src_len / 255) + 16;
}

/**
 * @brief Find the best LZ77 match at the current position using hash-based search.
 *
 * Searches the hash table for the longest matching sequence of bytes
 * starting at position ip. Updates the hash table with the current position.
 *
 * @param src        Source data buffer.
 * @param src_len    Total length of source data.
 * @param ip         Current input position to search from.
 * @param hash_table Hash table mapping 3-byte prefixes to recent positions.
 * @param min_match  Minimum match length to consider (typically 4).
 * @param max_match  Maximum match distance to search back.
 * @param match_off  [out] Offset of the best match relative to ip.
 * @param match_len  [out] Length of the best match found.
 */
static void lz77_match_find(const char *src, size_t src_len, size_t ip, int *hash_table,
                            int min_match, size_t max_match, size_t *match_off, size_t *match_len)
{
    unsigned int h;
    int idx;
    size_t best_len = 0;
    size_t best_off = 0;
    size_t limit;
    size_t s;

    *match_off = 0;
    *match_len = 0;

    if (ip + min_match > src_len)
        return;

    h = ((unsigned char)src[ip] << 8) ^ (unsigned char)src[ip + 1];
    h = ((h << 8) ^ (unsigned char)src[ip + 2]) & (HASH_TABLE_SIZE - 1);

    idx = hash_table[h];

    if (idx < 0 || (size_t)idx >= ip)
    {
        hash_table[h] = (int)ip;
        return;
    }

    limit = ip;
    if (limit > max_match)
        limit = max_match;

    s = (size_t)idx;
    while (s < ip && best_len < limit)
    {
        size_t len = 0;
        size_t max_cmp = ip - s;
        if (max_cmp > limit)
            max_cmp = limit;

        while (len < max_cmp && src[s + len] == src[ip + len])
        {
            len++;
        }

        if (len >= min_match && len > best_len)
        {
            best_len = len;
            best_off = ip - s;
        }
        s++;
    }

    hash_table[h] = (int)ip;

    if (best_len >= min_match)
    {
        *match_off = best_off;
        *match_len = best_len;
    }
}

/**
 * @brief Compress data using LZ4 block format.
 *
 * Allocates and returns a buffer containing the LZ4-compressed representation
 * of the source data. Uses a hash table for LZ77 match finding.
 *
 * @param src     Source data to compress.
 * @param src_len Length of the source data.
 * @param out_len Output parameter receiving the compressed length.
 *
 * @return Pointer to the compressed buffer (caller must free), or NULL on error.
 */
char *lz4_compress(const char *src, size_t src_len, size_t *out_len)
{
    int *hash_table;
    char *out_buf;
    size_t max_out;
    size_t ip, ip_limit, anchor, op;
    size_t lit_len, match_len, match_off;
    size_t offset;
    size_t token_pos;
    size_t ext;
    size_t i;

    if (!src || src_len == 0 || src_len > LZ4_MAX_INPUT_SIZE)
    {
        if (out_len)
            *out_len = 0;
        return NULL;
    }

    max_out = lz4_compress_bound(src_len);
    out_buf = (char *)malloc(max_out + 1);
    if (!out_buf)
    {
        if (out_len)
            *out_len = 0;
        return NULL;
    }

    hash_table = (int *)malloc(HASH_TABLE_SIZE * sizeof(int));
    if (!hash_table)
    {
        free(out_buf);
        if (out_len)
            *out_len = 0;
        return NULL;
    }

    for (i = 0; i < HASH_TABLE_SIZE; i++)
    {
        hash_table[i] = -1;
    }

    /* Short inputs: no matches possible, emit all literals */
    if (src_len < 12)
    {
        op = 0;
        lit_len = src_len;
        token_pos = op;
        op += 1;
        ext = lit_len;
        if (ext >= 15)
        {
            ext -= 15;
            while (ext >= 255)
            {
                out_buf[op] = (char)255;
                op++;
                ext -= 255;
            }
            out_buf[op] = (char)ext;
            op++;
        }
        if (lit_len > 15)
        {
            out_buf[token_pos] = (char)(15 * 16);
        }
        else
        {
            out_buf[token_pos] = (char)(lit_len * 16);
        }
        for (i = 0; i < lit_len; i++)
        {
            out_buf[op] = src[i];
            op++;
        }
        free(hash_table);
        *out_len = op;
        return out_buf;
    }

    ip = 0;
    ip_limit = src_len - 12;
    anchor = 0;
    op = 0;

    while (ip < ip_limit)
    {
        lz77_match_find(src, src_len, ip, hash_table, MIN_MATCH, MAX_MATCH, &match_off, &match_len);

        if (match_len >= MIN_MATCH)
        {
            offset = match_off;
            lit_len = ip - anchor;
            ip += match_len;
            match_len -= MIN_MATCH;

            token_pos = op;
            op += 1;

            ext = lit_len;
            if (ext >= 15)
            {
                ext -= 15;
                while (ext >= 255)
                {
                    out_buf[op] = (char)255;
                    op++;
                    ext -= 255;
                }
                out_buf[op] = (char)ext;
                op++;
            }

            for (i = 0; i < lit_len; i++)
            {
                out_buf[op] = src[anchor];
                op++;
                anchor++;
            }

            out_buf[op] = (char)(offset % 256);
            out_buf[op + 1] = (char)(offset / 256);
            op += 2;

            ext = match_len;
            if (ext >= 15)
            {
                ext -= 15;
                while (ext >= 255)
                {
                    out_buf[op] = (char)255;
                    op++;
                    ext -= 255;
                }
                out_buf[op] = (char)ext;
                op++;
            }

            if (lit_len > 15)
            {
                if (match_len > 15)
                {
                    out_buf[token_pos] = (char)(15 * 16 + 15);
                }
                else
                {
                    out_buf[token_pos] = (char)(15 * 16 + match_len);
                }
            }
            else
            {
                if (match_len > 15)
                {
                    out_buf[token_pos] = (char)(lit_len * 16 + 15);
                }
                else
                {
                    out_buf[token_pos] = (char)(lit_len * 16 + match_len);
                }
            }

            anchor = ip;
        }
        else
        {
            ip++;
        }
    }

    lit_len = src_len - anchor;
    token_pos = op;
    op += 1;

    ext = lit_len;
    if (ext >= 15)
    {
        ext -= 15;
        while (ext >= 255)
        {
            out_buf[op] = (char)255;
            op++;
            ext -= 255;
        }
        out_buf[op] = (char)ext;
        op++;
    }

    if (lit_len > 15)
    {
        out_buf[token_pos] = (char)(15 * 16);
    }
    else
    {
        out_buf[token_pos] = (char)(lit_len * 16);
    }

    for (i = 0; i < lit_len; i++)
    {
        out_buf[op] = src[anchor];
        op++;
        anchor++;
    }

    free(hash_table);

    *out_len = op;
    return out_buf;
}

/**
 * @brief Decompress LZ4 block-compressed data.
 *
 * Allocates and returns a buffer containing the decompressed data.
 * Validates bounds during decompression to prevent buffer overflows.
 *
 * @param src         Compressed input data.
 * @param src_len     Length of the compressed data.
 * @param dst_capacity Maximum output buffer capacity.
 * @param out_len     Output parameter receiving the decompressed length.
 *
 * @return Pointer to the decompressed buffer (caller must free), or NULL on error.
 */
char *lz4_decompress(const char *src, size_t src_len, size_t dst_capacity, size_t *out_len)
{
    char *out_buf;
    size_t op, ip, ip_limit;
    int token;
    size_t lit_len, match_len, offset;
    size_t ext;

    if (!src || src_len == 0 || dst_capacity == 0)
    {
        if (out_len)
            *out_len = 0;
        return NULL;
    }

    out_buf = (char *)malloc(dst_capacity + 1);
    if (!out_buf)
    {
        if (out_len)
            *out_len = 0;
        return NULL;
    }

    op = 0;
    ip = 0;
    ip_limit = src_len;

    while (ip < ip_limit)
    {
        token = lz4_byte_to_int(src[ip]);
        ip++;
        lit_len = token / 16;
        match_len = token % 16;

        if (lit_len == 15)
        {
            if (ip >= ip_limit)
            {
                free(out_buf);
                if (out_len)
                    *out_len = 0;
                return NULL;
            }
            ext = (size_t)lz4_byte_to_int(src[ip]);
            ip++;
            lit_len += ext;
            while (ext == 255)
            {
                if (ip >= ip_limit)
                {
                    free(out_buf);
                    if (out_len)
                        *out_len = 0;
                    return NULL;
                }
                ext = (size_t)lz4_byte_to_int(src[ip]);
                ip++;
                lit_len += ext;
            }
        }

        while (lit_len > 0)
        {
            if (op >= dst_capacity || ip >= ip_limit)
            {
                free(out_buf);
                if (out_len)
                    *out_len = 0;
                return NULL;
            }
            out_buf[op] = src[ip];
            op++;
            ip++;
            lit_len--;
        }

        if (ip >= ip_limit)
            break;

        if (ip + 2 > ip_limit)
        {
            free(out_buf);
            if (out_len)
                *out_len = 0;
            return NULL;
        }
        offset = (size_t)lz4_byte_to_int(src[ip]) + (size_t)lz4_byte_to_int(src[ip + 1]) * 256;
        ip += 2;

        if (offset == 0)
        {
            free(out_buf);
            if (out_len)
                *out_len = 0;
            return NULL;
        }

        if (match_len == 15)
        {
            if (ip >= ip_limit)
            {
                free(out_buf);
                if (out_len)
                    *out_len = 0;
                return NULL;
            }
            ext = (size_t)lz4_byte_to_int(src[ip]);
            ip++;
            match_len += ext;
            while (ext == 255)
            {
                if (ip >= ip_limit)
                {
                    free(out_buf);
                    if (out_len)
                        *out_len = 0;
                    return NULL;
                }
                ext = (size_t)lz4_byte_to_int(src[ip]);
                ip++;
                match_len += ext;
            }
        }
        match_len += MIN_MATCH;

        while (match_len > 0)
        {
            if (op >= dst_capacity)
            {
                free(out_buf);
                if (out_len)
                    *out_len = 0;
                return NULL;
            }
            if (offset > 0 && offset <= op)
            {
                out_buf[op] = out_buf[op - offset];
            }
            else
            {
                out_buf[op] = 0;
            }
            op++;
            match_len--;
        }
    }

    *out_len = op;
    return out_buf;
}

/**
 * @brief Free a buffer allocated by lz4_compress or lz4_decompress.
 *
 * @param ptr Pointer to the buffer to free (may be NULL).
 */
void lz4_free(void *ptr)
{
    free(ptr);
}
