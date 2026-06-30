/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 *
 * Original LZ4 implementation written for SPARK by Kimi-2.6,
 * ported to C for the Garry storage engine.
 */

/**
 * @file lz4.h
 * @brief LZ4 block compression and decompression API.
 *
 * Provides allocate-on-call compress/decompress functions. All returned
 * buffers must be freed with lz4_free(). Maximum input size is 64 KiB.
 */

#ifndef LZ4_H
#define LZ4_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LZ4_MAX_INPUT_SIZE 65536

/**
 * @brief Return the maximum possible compressed output size.
 *
 * @param src_len  Length of the input data in bytes
 * @return Upper bound on compressed size, or 0 if src_len exceeds
 *         LZ4_MAX_INPUT_SIZE
 */
    size_t lz4_compress_bound(size_t src_len);

/**
 * @brief Compress a block of data using LZ4.
 *
 * Allocates an output buffer and compresses @p src into it.
 * The caller must free the returned buffer with lz4_free().
 *
 * @param src     Input data to compress
 * @param src_len Length of input in bytes
 * @param out_len Output: number of bytes written to the result
 * @return Heap-allocated compressed buffer, or NULL on error
 */
    char *lz4_compress(const char *src, size_t src_len, size_t *out_len);

/**
 * @brief Decompress LZ4-compressed data.
 *
 * Allocates an output buffer and decompresses @p src into it.
 * The caller must free the returned buffer with lz4_free().
 *
 * @param src          Compressed input data
 * @param src_len      Length of compressed input in bytes
 * @param dst_capacity Maximum capacity of the decompressed output
 * @param out_len      Output: number of bytes written to the result
 * @return Heap-allocated decompressed buffer, or NULL on error
 */
    char *lz4_decompress(const char *src, size_t src_len, size_t dst_capacity, size_t *out_len);

/**
 * @brief Free a buffer allocated by lz4_compress or lz4_decompress.
 *
 * @param ptr  Buffer to free (may be NULL)
 */
    void lz4_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* LZ4_H */
