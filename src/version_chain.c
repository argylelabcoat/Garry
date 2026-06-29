/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file version_chain.c
 * @brief In-page version chain for MVCC multi-version storage.
 *
 * Each B-tree leaf slot stores a version chain page. The chain head
 * contains inline version slots; overflow versions spill to chained
 * overflow pages. Each slot records the writing transaction ID and
 * either a value or a tombstone marker. Pruning removes versions
 * invisible to all active transactions.
 *
 * Entry layout (all little-endian):
 *   [0..3]  txid_created
 *   [4..7]  txid_deleted (0 = alive)
 *   [8..11] value_len
 *   [12]    is_tombstone
 *   [13]    flags (bit 0 = overflow)
 *   [14..]  inline value bytes or overflow head page_id
 */

#include "version_chain.h"
#include "slotted_page.h"
#include "db_header.h"
#include "buffer_pool.h"
#include <stdlib.h>
#include <string.h>

#ifndef GARRY_TRUE
#define GARRY_TRUE  1
#endif
#ifndef GARRY_FALSE
#define GARRY_FALSE 0
#endif

void garry_chain_page_init(garry_page_buffer buf, garry_u32 page_size)
{
    garry_page_init(buf, GARRY_NODE_CHAIN, 0, (garry_i32)page_size);
}

garry_bool garry_chain_page_append(garry_page_buffer buf, garry_u32 page_size,
                                   garry_txn_id txn, const char *value, garry_i32 vlen)
{
    garry_byte entry_buf[GARRY_MAX_RECORD_SIZE];
    garry_i32 pos;
    garry_i32 v;
    garry_i32 i;
    garry_i32 slot_idx;
    garry_i32 inline_cap;

    inline_cap = garry_chain_inline_capacity(page_size);
    if (vlen > inline_cap) {
        return GARRY_FALSE;
    }

    pos = 0;

    /* txid_created (LE) */
    v = txn;
    entry_buf[pos] = (garry_u8)(v & 0xFF);           pos++;
    entry_buf[pos] = (garry_u8)((v >> 8) & 0xFF);    pos++;
    entry_buf[pos] = (garry_u8)((v >> 16) & 0xFF);   pos++;
    entry_buf[pos] = (garry_u8)((v >> 24) & 0xFF);   pos++;

    /* txid_deleted = 0 (NIL) */
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;

    /* value_len (LE) */
    v = vlen;
    entry_buf[pos] = (garry_u8)(v & 0xFF);           pos++;
    entry_buf[pos] = (garry_u8)((v >> 8) & 0xFF);    pos++;
    entry_buf[pos] = (garry_u8)((v >> 16) & 0xFF);   pos++;
    entry_buf[pos] = (garry_u8)((v >> 24) & 0xFF);   pos++;

    /* is_tombstone = 0 */
    entry_buf[pos] = 0; pos++;

    /* flags = 0 (inline) */
    entry_buf[pos] = 0; pos++;

    /* value bytes */
    if (value != NULL && vlen > 0) {
        for (i = 0; i < vlen; i++) {
            entry_buf[pos + i] = (garry_u8)value[i];
        }
        pos += vlen;
    }

    slot_idx = garry_page_insert(buf, entry_buf, pos, (garry_i32)page_size);
    if (slot_idx < 0) {
        return GARRY_FALSE;
    }
    return GARRY_TRUE;
}

garry_bool garry_chain_page_append_tombstone(garry_page_buffer buf, garry_u32 page_size,
                                             garry_txn_id txn)
{
    garry_byte entry_buf[GARRY_CHAIN_ENTRY_BUF_SIZE];
    garry_i32 pos;
    garry_i32 v;
    garry_i32 slot_idx;

    /* NOTE: Overflow chain pages referenced by existing versions are NOT freed
     * when a tombstone is appended. This is a known leak that requires a
     * free-list mechanism to reclaim overflow pages after all versions
     * referencing them become unreachable. Out of scope for now. */

    pos = 0;

    /* txid_created (LE) */
    v = txn;
    entry_buf[pos] = (garry_u8)(v & 0xFF);           pos++;
    entry_buf[pos] = (garry_u8)((v >> 8) & 0xFF);    pos++;
    entry_buf[pos] = (garry_u8)((v >> 16) & 0xFF);   pos++;
    entry_buf[pos] = (garry_u8)((v >> 24) & 0xFF);   pos++;

    /* txid_deleted = 0 (NIL) */
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;

    /* value_len = 0 */
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;

    /* is_tombstone = 1 */
    entry_buf[pos] = 1; pos++;

    /* flags = 0 */
    entry_buf[pos] = 0; pos++;

    slot_idx = garry_page_insert(buf, entry_buf, pos, (garry_i32)page_size);
    if (slot_idx < 0) {
        return GARRY_FALSE;
    }
    return GARRY_TRUE;
}

garry_i32 garry_overflow_chunk_size(garry_u32 page_size)
{
    return (garry_i32)page_size - GARRY_PAGE_HEADER_SIZE - GARRY_OVERFLOW_PTR_SIZE;
}

garry_i32 garry_chain_inline_capacity(garry_u32 page_size)
{
    return (garry_i32)page_size - GARRY_PAGE_HEADER_SIZE - GARRY_OVERFLOW_PTR_SIZE - GARRY_CHAIN_ENTRY_HEADER_SIZE;
}

garry_i32 garry_overflow_write(garry_buffer_pool *pool, const char *value,
                               garry_i32 vlen)
{
    garry_i32 head = -1;
    garry_i32 prev_pid = -1;
    garry_i32 offset = 0;
    garry_i32 chunk_cap;

    chunk_cap = garry_overflow_chunk_size(pool->page_size);

    while (offset < vlen) {
        garry_i32 this_pid;
        garry_page_buffer *pbuf;
        garry_i32 this_chunk;
        garry_i32 c;

        this_pid = garry_pool_allocate(pool);
        if (this_pid < 0) return -1;

        pbuf = garry_pool_pin_page(pool, this_pid);
        if (pbuf == NULL) return -1;

        garry_page_init(*pbuf, GARRY_NODE_OVERFLOW, 0, (garry_i32)pool->page_size);

        if (prev_pid >= 0) {
            garry_page_buffer *ppbuf;
            ppbuf = garry_pool_pin_page(pool, prev_pid);
            if (ppbuf != NULL) {
                garry_write_int32((garry_byte*)*ppbuf, GARRY_PAGE_HEADER_SIZE, this_pid);
                garry_pool_mark_dirty(pool, prev_pid);
                garry_pool_release_page(pool, prev_pid);
            }
        }

        this_chunk = chunk_cap;
        if (vlen - offset < this_chunk) {
            this_chunk = vlen - offset;
        }

        for (c = 0; c < this_chunk; c++) {
            (*pbuf)[GARRY_PAGE_HEADER_SIZE + GARRY_OVERFLOW_PTR_SIZE + c] = (garry_u8)value[offset + c];
        }

        garry_write_int32((garry_byte*)*pbuf, GARRY_PAGE_HEADER_SIZE, -1);
        garry_pool_mark_dirty(pool, this_pid);
        garry_pool_release_page(pool, this_pid);

        if (head == -1) head = this_pid;
        prev_pid = this_pid;
        offset += this_chunk;
    }

    return head;
}

garry_i32 garry_overflow_read(garry_buffer_pool *pool, garry_i32 head,
                              garry_i32 total_len, char *out_buf)
{
    garry_i32 pid;
    garry_i32 copied = 0;
    garry_i32 chunk_cap;

    chunk_cap = garry_overflow_chunk_size(pool->page_size);
    pid = head;

    while (pid >= 0 && copied < total_len) {
        garry_page_buffer *pbuf;
        garry_i32 next;
        garry_i32 this_chunk;
        garry_i32 c;

        pbuf = garry_pool_pin_page(pool, pid);
        if (pbuf == NULL) return copied;

        next = garry_read_int32((garry_byte*)*pbuf, GARRY_PAGE_HEADER_SIZE);

        this_chunk = chunk_cap;
        if (total_len - copied < this_chunk) {
            this_chunk = total_len - copied;
        }

        for (c = 0; c < this_chunk; c++) {
            out_buf[copied + c] = (char)(*pbuf)[GARRY_PAGE_HEADER_SIZE + GARRY_OVERFLOW_PTR_SIZE + c];
        }

        garry_pool_release_page(pool, pid);
        copied += this_chunk;
        pid = next;
    }

    return copied;
}

garry_bool garry_chain_page_append_overflow(garry_page_buffer buf, garry_u32 page_size,
                                            garry_txn_id txn, garry_i32 total_len,
                                            garry_i32 head)
{
    garry_byte entry_buf[GARRY_CHAIN_ENTRY_BUF_SIZE];
    garry_i32 pos;
    garry_i32 v;
    garry_i32 slot_idx;

    pos = 0;

    /* txid_created (LE) */
    v = txn;
    entry_buf[pos] = (garry_u8)(v & 0xFF);           pos++;
    entry_buf[pos] = (garry_u8)((v >> 8) & 0xFF);    pos++;
    entry_buf[pos] = (garry_u8)((v >> 16) & 0xFF);   pos++;
    entry_buf[pos] = (garry_u8)((v >> 24) & 0xFF);   pos++;

    /* txid_deleted = 0 */
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;
    entry_buf[pos] = 0; pos++;

    /* value_len (LE) */
    v = total_len;
    entry_buf[pos] = (garry_u8)(v & 0xFF);           pos++;
    entry_buf[pos] = (garry_u8)((v >> 8) & 0xFF);    pos++;
    entry_buf[pos] = (garry_u8)((v >> 16) & 0xFF);   pos++;
    entry_buf[pos] = (garry_u8)((v >> 24) & 0xFF);   pos++;

    /* is_tombstone = 0 */
    entry_buf[pos] = 0; pos++;

    /* flags = overflow */
    entry_buf[pos] = GARRY_CHAIN_FLAG_OVERFLOW; pos++;

    /* head page_id (LE) */
    entry_buf[pos] = (garry_u8)(head & 0xFF);           pos++;
    entry_buf[pos] = (garry_u8)((head >> 8) & 0xFF);    pos++;
    entry_buf[pos] = (garry_u8)((head >> 16) & 0xFF);   pos++;
    entry_buf[pos] = (garry_u8)((head >> 24) & 0xFF);   pos++;

    slot_idx = garry_page_insert(buf, entry_buf, pos, (garry_i32)page_size);
    if (slot_idx < 0) {
        return GARRY_FALSE;
    }
    return GARRY_TRUE;
}

static garry_i32 read_le32(const garry_byte *buf, garry_i32 p)
{
    garry_i32 b0, b1, b2, b3;
    b0 = (garry_i32)buf[p];     if (b0 < 0) b0 += 256;
    b1 = (garry_i32)buf[p + 1]; if (b1 < 0) b1 += 256;
    b2 = (garry_i32)buf[p + 2]; if (b2 < 0) b2 += 256;
    b3 = (garry_i32)buf[p + 3]; if (b3 < 0) b3 += 256;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

/**
 * Find the most recent version visible to the given snapshot.
 *
 * Scans the chain backward (newest to oldest). A version is visible if:
 *   1. txid_created <= snap_txid
 *   2. txid_deleted == 0 OR txid_deleted > snap_txid
 *   3. txid_created is not in the active transaction list (uncommitted)
 *
 * Returns a malloc'd copy of the value, or NULL if the latest visible
 * version is a tombstone or no version exists. Caller must free().
 */
char* garry_chain_page_find_visible(garry_buffer_pool *pool,
                                    garry_page_buffer buf, garry_u32 page_size,
                                    garry_txn_id snap_txid,
                                    garry_txn_id_ptr active, garry_i32 active_count,
                                    garry_i32 *vlen)
{
    garry_page_buffer local;
    garry_i32 rec_count;
    garry_i32 i;
    garry_byte rec_data[GARRY_CHAIN_ENTRY_BUF_SIZE];
    garry_i32 rlen;
    garry_i32 txid_created;
    garry_i32 txid_deleted;
    garry_i32 value_len;
    garry_i32 is_tombstone;
    garry_i32 fb;
    garry_i32 is_overflow;
    garry_i32 pos;
    char *result = NULL;
    garry_i32 k;
    garry_i32 visible;
    garry_i32 done;

    memcpy(local, buf, (size_t)page_size);
    rec_count = garry_page_record_count(&local);
    i = rec_count - 1;
    *vlen = 0;
    done = 0;

    while (i >= 0 && result == NULL && !done) {
        rlen = garry_page_get(&local, i, rec_data, (garry_i32)page_size);
        if (rlen >= GARRY_CHAIN_ENTRY_HEADER_SIZE) {
            pos = 0;
            txid_created = read_le32(rec_data, pos); pos += 4;
            txid_deleted = read_le32(rec_data, pos);  pos += 4;
            value_len    = read_le32(rec_data, pos);  pos += 4;
            is_tombstone = (rec_data[pos] != 0) ? 1 : 0; pos++;
            fb = (garry_i32)rec_data[pos]; if (fb < 0) fb += 256; pos++;
            is_overflow = (fb & 0x1) == GARRY_CHAIN_FLAG_OVERFLOW ? 1 : 0;

            visible = 1;
            if (txid_created > snap_txid) visible = 0;
            if (visible && txid_deleted != 0 && txid_deleted <= snap_txid) visible = 0;
            if (visible) {
                for (k = 0; k < active_count; k++) {
                    if (active[k] == snap_txid) continue;
                    if (active[k] == txid_created) {
                        visible = 0;
                        break;
                    }
                }
            }

            if (visible) {
                if (is_tombstone) {
                    done = 1;
                } else {
                    if (is_overflow) {
                        garry_i32 head_id;
                        garry_i32 copied;
                        head_id = read_le32(rec_data, pos); pos += 4;
                        result = (char*)malloc((size_t)value_len + 1);
                        if (result != NULL) {
                            copied = garry_overflow_read(pool, head_id, value_len, result);
                            result[value_len] = '\0';
                            *vlen = copied;
                        }
                    } else {
                        result = (char*)malloc((size_t)value_len + 1);
                        if (result != NULL) {
                            garry_i32 j;
                            for (j = 0; j < value_len; j++) {
                                result[j] = (char)rec_data[pos + j];
                            }
                            result[value_len] = '\0';
                            *vlen = value_len;
                        }
                    }
                }
            }
        }
        i--;
    }

    return result;
}

void garry_overflow_free(garry_buffer_pool *pool, garry_i32 head)
{
    garry_i32 pid = head;
    while (pid >= 0) {
        garry_page_buffer *pbuf;
        garry_i32 next;
        pbuf = garry_pool_try_pin(pool, pid);
        if (pbuf == NULL) break;
        next = garry_read_int32((garry_byte*)*pbuf, GARRY_PAGE_HEADER_SIZE);
        garry_pool_release_page(pool, pid);
        pid = next;
    }
}

/**
 * Prune old versions that are no longer visible to any active transaction.
 *
 * Keeps the latest version unconditionally. For older versions, checks
 * visibility against all active snapshot IDs. Copies surviving entries
 * into a fresh page buffer and overwrites the original. Frees overflow
 * pages from discarded entries.
 */
void garry_chain_page_prune(garry_buffer_pool *pool, garry_page_buffer buf,
                            garry_u32 page_size,
                            garry_txn_id_ptr active, garry_i32 active_count)
{
    garry_page_buffer local;
    garry_i32 rec_count;
    garry_i32 i, j, k;
    garry_byte rec[GARRY_CHAIN_ENTRY_BUF_SIZE];
    garry_i32 rlen;
    garry_i32 pos;
    garry_i32 txid_created;
    garry_i32 txid_deleted;
    garry_bool keep[GARRY_MAX_VERSIONS_PER_PAGE];
    garry_i32 visible;
    garry_i32 snap;
    garry_page_buffer tmp;

    memcpy(local, buf, (size_t)page_size);
    rec_count = garry_page_record_count(&local);
    if (rec_count <= 1) return;

    for (i = 0; i < rec_count; i++) {
        rlen = garry_page_get(&local, i, rec, (garry_i32)page_size);
        if (rlen >= GARRY_CHAIN_PRUNE_MIN_LEN) {
            pos = 0;
            txid_created = read_le32(rec, pos); pos += 4;
            txid_deleted = read_le32(rec, pos); pos += 4;

            if (i == rec_count - 1) {
                keep[i] = GARRY_TRUE;
            } else {
                visible = 0;
                for (k = 0; k < active_count; k++) {
                    snap = active[k];
                    if (txid_created <= snap &&
                        !(txid_deleted != 0 && txid_deleted <= snap)) {
                        visible = 1;
                    }
                }
                keep[i] = visible ? GARRY_TRUE : GARRY_FALSE;
            }
        } else {
            keep[i] = GARRY_TRUE;
        }
    }

    for (i = 0; i < rec_count; i++) {
        if (!keep[i]) {
            rlen = garry_page_get(&local, i, rec, (garry_i32)page_size);
            if (rlen >= GARRY_CHAIN_ENTRY_HEADER_SIZE + 4) {
                garry_i32 fb2;
                fb2 = (garry_i32)rec[13]; if (fb2 < 0) fb2 += 256;
                if ((fb2 & GARRY_CHAIN_FLAG_OVERFLOW) != 0) {
                    garry_i32 head_id;
                    head_id = read_le32(rec, 14);
                    if (pool != NULL && head_id >= 0) {
                        garry_overflow_free(pool, head_id);
                    }
                }
            }
        }
    }

    garry_page_init(tmp, GARRY_NODE_CHAIN, 0, (garry_i32)page_size);
    for (i = 0; i < rec_count; i++) {
        if (keep[i]) {
            rlen = garry_page_get(&local, i, rec, (garry_i32)page_size);
            garry_page_insert(tmp, rec, rlen, (garry_i32)page_size);
        }
    }
    for (j = 0; j < (garry_i32)page_size; j++) {
        buf[j] = tmp[j];
    }
}

garry_bool garry_chain_page_has_version(garry_page_buffer buf, garry_u32 page_size,
                                        garry_txn_id txn)
{
    garry_page_buffer local;
    garry_i32 rec_count;
    garry_i32 i;
    garry_byte rec_data[GARRY_CHAIN_ENTRY_BUF_SIZE];
    garry_i32 rlen;
    garry_i32 txid_created;

    memcpy(local, buf, (size_t)page_size);
    rec_count = garry_page_record_count(&local);
    for (i = 0; i < rec_count; i++) {
        rlen = garry_page_get(&local, i, rec_data, (garry_i32)page_size);
        if (rlen >= 4) {
            txid_created = read_le32(rec_data, 0);
            if (txid_created == txn) return GARRY_TRUE;
        }
    }
    return GARRY_FALSE;
}
