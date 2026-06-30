/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file record_codec.h
 * @brief CBOR-based record encoding for key-value pairs and descriptors.
 */

#ifndef GARRY_RECORD_CODEC_H
#define GARRY_RECORD_CODEC_H

#include "garry/config.h"
#include "garry/types.h"
#include "storage_types.h"
#include "storage_core.h"

/* CBOR major type base constants (from cbor_lib). */
#define GARRY_CBOR_UINT_BASE    0
#define GARRY_CBOR_NEGINT_BASE  32
#define GARRY_CBOR_BYTES_BASE   64
#define GARRY_CBOR_TEXT_BASE    96
#define GARRY_CBOR_ARRAY_BASE   128
#define GARRY_CBOR_MAP_BASE     160
#define GARRY_CBOR_SIMPLE_BASE  224
#define GARRY_CBOR_SIMPLE_FALSE 20
#define GARRY_CBOR_SIMPLE_TRUE  21

/**
 * @brief Encode a key-value pair into CBOR byte string format.
 *
 * Encodes the key and value as a CBOR array of two byte strings:
 * [key_bytes, value_bytes]. The output is written to out_buf.
 *
 * @param key     Key bytes to encode
 * @param klen    Key length in bytes
 * @param value   Value bytes to encode
 * @param vlen    Value length in bytes
 * @param out_buf Output buffer for encoded representation
 * @return Number of bytes written to out_buf
 */
garry_i32 garry_encode_kv(const garry_byte *key, garry_i32 klen, const garry_byte *value,
                          garry_i32 vlen, garry_byte *out_buf);

/**
 * @brief Decode a CBOR-encoded key-value pair.
 *
 * Inverse of garry_encode_kv. Extracts key and value byte strings from
 * the CBOR array representation.
 *
 * @param encoded CBOR-encoded buffer to decode
 * @param elen    Length of encoded data in bytes
 * @param key     Output buffer for decoded key
 * @param klen    Output: length of decoded key
 * @param value   Output buffer for decoded value
 * @param vlen    Output: length of decoded value
 * @return GARRY_TRUE on success, GARRY_FALSE on invalid input or decode error
 */
garry_bool garry_decode_kv(const garry_byte *encoded, garry_i32 elen, garry_byte *key,
                           garry_i32 *klen, garry_byte *value, garry_i32 *vlen);

/**
 * @brief Encode a key without its value (for B-tree index entries).
 *
 * Encodes just the key as a single CBOR byte string, omitting the value.
 * Used for index-only lookups where only the key is needed.
 *
 * @param key     Key bytes to encode
 * @param klen    Key length in bytes
 * @param out_buf Output buffer for encoded key
 * @return Number of bytes written to out_buf
 */
garry_i32 garry_encode_key_only(const garry_byte *key, garry_i32 klen, garry_byte *out_buf);

/**
 * @brief Decode a key-only encoded entry.
 *
 * Inverse of garry_encode_key_only. Extracts the key from a single
 * CBOR byte string encoding.
 *
 * @param encoded CBOR-encoded buffer to decode
 * @param elen    Length of encoded data in bytes
 * @param key     Output buffer for decoded key
 * @return Length of decoded key in bytes
 */
garry_i32 garry_decode_key_only(const garry_byte *encoded, garry_i32 elen, garry_byte *key);

/**
 * @brief Encode a B-tree node descriptor (chain ID + child flag).
 *
 * Encodes the descriptor as a CBOR map with two entries:
 * {"cid": chain_id, "hc": has_children}. Used as the value in
 * B-tree leaf entries to point to version chains.
 *
 * @param chain_id     Version chain ID for this key
 * @param has_children GARRY_TRUE if this key has child nodes
 * @param out_buf      Output buffer for encoded descriptor
 * @return Number of bytes written to out_buf
 */
garry_i32 garry_encode_descriptor(garry_i32 chain_id, garry_bool has_children, garry_byte *out_buf);

/**
 * @brief Decode a B-tree node descriptor.
 *
 * Inverse of garry_encode_descriptor. Extracts the chain ID and
 * child flag from the CBOR map encoding.
 *
 * @param encoded     CBOR-encoded descriptor to decode
 * @param elen        Length of encoded data in bytes
 * @param chain_id    Output: decoded version chain ID
 * @param has_children Output: GARRY_TRUE if key has children
 */
void garry_decode_descriptor(const garry_byte *encoded, garry_i32 elen, garry_i32 *chain_id,
                             garry_bool *has_children);

/**
 * @brief Encode a CBOR array header at the given position.
 *
 * Writes the array major type header for @p count elements. Uses
 * inline encoding for counts < 24, 1-byte encoding otherwise.
 *
 * @param out    Output buffer
 * @param pos    Current write position in the buffer
 * @param count  Number of elements in the array
 * @return New position after the header
 */
garry_i32 garry_cbor_encode_array_header_raw(garry_byte *out, garry_i32 pos, garry_i32 count);

/**
 * @brief Encode a CBOR byte string at the given position.
 *
 * Writes the byte string major type header followed by the raw data.
 * Uses inline encoding for lengths < 24, 1-byte encoding otherwise.
 *
 * @param data  Byte string data to encode
 * @param dlen  Length of the data in bytes
 * @param out   Output buffer
 * @param pos   Current write position in the buffer
 * @return New position after the encoded byte string
 */
garry_i32 garry_cbor_encode_byte_string_raw(const garry_byte *data, garry_i32 dlen, garry_byte *out,
                                            garry_i32 pos);

/**
 * @brief Decode a CBOR array header and return the element count.
 *
 * Reads the array major type header at @p pos and returns the number
 * of elements in the array.
 *
 * @param buf  Input buffer containing CBOR data
 * @param blen Length of the input buffer
 * @param pos  Position of the array header
 * @return Number of elements in the array, or 0 on malformed input
 */
garry_i32 garry_cbor_decode_array_header_raw(const garry_byte *buf, garry_i32 blen, garry_i32 pos);

/**
 * @brief Return the byte size of a CBOR array header.
 *
 * Computes how many bytes the array header occupies without decoding
 * the element count.
 *
 * @param buf  Input buffer containing CBOR data
 * @param pos  Position of the array header
 * @return Size of the header in bytes (1 or 2)
 */
garry_i32 garry_cbor_array_header_size_raw(const garry_byte *buf, garry_i32 pos);

/**
 * @brief Decode a CBOR byte string at the given position.
 *
 * Reads the byte string header and copies the data into @p out_data.
 *
 * @param buf      Input buffer containing CBOR data
 * @param blen     Length of the input buffer
 * @param pos      Position of the byte string
 * @param out_data Output buffer for the decoded bytes
 * @param dlen     Output: length of the decoded byte string
 * @return New position after the byte string, or 0 on error
 */
garry_i32 garry_cbor_decode_byte_string_raw(const garry_byte *buf, garry_i32 blen, garry_i32 pos,
                                            garry_byte *out_data, garry_i32 *dlen);

#endif /* GARRY_RECORD_CODEC_H */
