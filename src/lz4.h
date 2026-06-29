/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
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
extern "C" {
#endif

#define LZ4_MAX_INPUT_SIZE 65536

size_t lz4_compress_bound(size_t src_len);

char *lz4_compress(const char *src, size_t src_len, size_t *out_len);

char *lz4_decompress(const char *src, size_t src_len,
                     size_t dst_capacity, size_t *out_len);

void lz4_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* LZ4_H */
