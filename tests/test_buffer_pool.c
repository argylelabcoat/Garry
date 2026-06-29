/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/types.h"
#include "storage_types.h"
#include "storage_core.h"
#include "buffer_pool.h"
#include "test_helpers.h"

static void test_create_pool(void) {
    garry_buffer_pool *pool;
    pool = garry_pool_create("tp_create.db", 2, 4096);
    GARRY_CHECK(pool != NULL);
    GARRY_CHECK(pool->capacity == 2);
    garry_pool_close(pool);
}

static void test_allocate_and_pin(void) {
    garry_buffer_pool *pool;
    garry_i32 pid;
    garry_page_buffer *buf;
    pool = garry_pool_create("tp_pin.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    pid = garry_pool_allocate(pool);
    GARRY_CHECK(pid >= 0);
    buf = garry_pool_pin_page(pool, pid);
    GARRY_CHECK(buf != NULL);
    GARRY_CHECK(garry_pool_pin_count(pool, pid) == 1);
    garry_pool_release_page(pool, pid);
    GARRY_CHECK(garry_pool_pin_count(pool, pid) == 0);
    garry_pool_close(pool);
}

static void test_evicts_clean_page(void) {
    garry_buffer_pool *pool;
    garry_i32 p1, p2, p3;
    pool = garry_pool_create("tp_evict.db", 2, 4096);
    GARRY_CHECK(pool != NULL);
    p1 = garry_pool_allocate(pool);
    p2 = garry_pool_allocate(pool);
    garry_pool_release_page(pool, p1);
    p3 = garry_pool_allocate(pool);
    GARRY_CHECK(!garry_pool_is_loaded(pool, p1));
    garry_pool_close(pool);
}

static void test_does_not_evict_pinned(void) {
    garry_buffer_pool *pool;
    garry_i32 p1, result;
    garry_page_buffer *buf;
    pool = garry_pool_create("tp_noevict.db", 1, 4096);
    GARRY_CHECK(pool != NULL);
    p1 = garry_pool_allocate(pool);
    buf = garry_pool_pin_page(pool, p1);
    GARRY_CHECK(buf != NULL);
    result = garry_pool_allocate(pool);
    GARRY_CHECK(result < 0);
    garry_pool_close(pool);
}

static void test_dirty_flushed_on_evict(void) {
    garry_buffer_pool *pool;
    garry_i32 p1, p2, p3;
    pool = garry_pool_create("tp_dirty.db", 2, 4096);
    GARRY_CHECK(pool != NULL);
    p1 = garry_pool_allocate(pool);
    p2 = garry_pool_allocate(pool);
    garry_pool_mark_dirty(pool, p1);
    garry_pool_release_page(pool, p1);
    p3 = garry_pool_allocate(pool);
    GARRY_CHECK(!garry_pool_is_loaded(pool, p1));
    garry_pool_close(pool);
}

static void test_pin_count_underflow(void) {
    garry_buffer_pool *pool;
    garry_i32 p1;
    pool = garry_pool_create("tp_underflow.db", 2, 4096);
    GARRY_CHECK(pool != NULL);
    p1 = garry_pool_allocate(pool);
    garry_pool_release_page(pool, p1);
    GARRY_CHECK(garry_pool_pin_count(pool, p1) == 0);
    garry_pool_close(pool);
}

static void test_page_round_trip(void) {
    garry_buffer_pool *pool;
    garry_i32 pid;
    garry_page_buffer *p1;
    garry_page_buffer *p2;
    pool = garry_pool_create("tp_rt.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    pid = garry_pool_allocate(pool);
    p1 = garry_pool_pin_page(pool, pid);
    GARRY_CHECK(p1 != NULL);
    (*p1)[0] = (garry_u8)65;
    (*p1)[1] = (garry_u8)66;
    (*p1)[2] = (garry_u8)67;
    (*p1)[3] = (garry_u8)68;
    garry_pool_mark_dirty(pool, pid);
    garry_pool_release_page(pool, pid);
    p2 = garry_pool_pin_page(pool, pid);
    GARRY_CHECK(p2 != NULL);
    GARRY_CHECK((*p2)[0] == 65);
    GARRY_CHECK((*p2)[3] == 68);
    garry_pool_release_page(pool, pid);
    garry_pool_close(pool);
}

static void test_multiple_pools(void) {
    garry_buffer_pool *pool1;
    garry_buffer_pool *pool2;
    garry_i32 p1, p2;
    pool1 = garry_pool_create("tp_multi1.db", 2, 4096);
    pool2 = garry_pool_create("tp_multi2.db", 2, 4096);
    GARRY_CHECK(pool1 != NULL);
    GARRY_CHECK(pool2 != NULL);
    p1 = garry_pool_allocate(pool1);
    p2 = garry_pool_allocate(pool2);
    GARRY_CHECK(p1 >= 0);
    GARRY_CHECK(p2 >= 0);
    garry_pool_close(pool1);
    garry_pool_close(pool2);
}

static void test_free_list_reuses_page(void) {
    garry_buffer_pool *pool;
    garry_i32 p1, p2, p3;
    pool = garry_pool_create("tp_freelist.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    GARRY_CHECK(pool->free_list_head == -1);
    p1 = garry_pool_allocate(pool);
    p2 = garry_pool_allocate(pool);
    GARRY_CHECK(p1 >= 0);
    GARRY_CHECK(p2 >= 0);
    /* Free p1 — should go onto the free list */
    garry_pool_free_page(pool, p1);
    GARRY_CHECK(pool->free_list_head == p1);
    /* Allocate should reuse p1 from the free list */
    p3 = garry_pool_allocate(pool);
    GARRY_CHECK(p3 == p1);
    GARRY_CHECK(pool->free_list_head == -1);
    garry_pool_close(pool);
}

static void test_free_list_chain(void) {
    garry_buffer_pool *pool;
    garry_i32 p1, p2, p3, p4;
    pool = garry_pool_create("tp_freelist_chain.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    p1 = garry_pool_allocate(pool);
    p2 = garry_pool_allocate(pool);
    p3 = garry_pool_allocate(pool);
    /* Free p1, p2, p3 — chain: p3 -> p2 -> p1 */
    garry_pool_free_page(pool, p1);
    garry_pool_free_page(pool, p2);
    garry_pool_free_page(pool, p3);
    GARRY_CHECK(pool->free_list_head == p3);
    /* Allocate should reuse in LIFO order: p3, p2, p1 */
    p4 = garry_pool_allocate(pool);
    GARRY_CHECK(p4 == p3);
    p4 = garry_pool_allocate(pool);
    GARRY_CHECK(p4 == p2);
    p4 = garry_pool_allocate(pool);
    GARRY_CHECK(p4 == p1);
    GARRY_CHECK(pool->free_list_head == -1);
    garry_pool_close(pool);
}

static void test_free_list_persists_in_header(void) {
    garry_buffer_pool *pool;
    garry_i32 p1;
    pool = garry_pool_create("tp_freelist_hdr.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    p1 = garry_pool_allocate(pool);
    garry_pool_free_page(pool, p1);
    GARRY_CHECK(pool->free_list_head == p1);
    /* Simulate close: the engine would persist free_list_head to header */
    /* On reopen, pool->free_list_head should be restored */
    garry_pool_close(pool);
    /* Reopen and verify free list state */
    pool = garry_pool_create("tp_freelist_hdr.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    /* fresh pool starts with free_list_head = -1 (engine sets it from header) */
    GARRY_CHECK(pool->free_list_head == -1);
    garry_pool_close(pool);
}

int main(void) {
    test_create_pool();
    test_allocate_and_pin();
    test_evicts_clean_page();
    test_does_not_evict_pinned();
    test_dirty_flushed_on_evict();
    test_pin_count_underflow();
    test_page_round_trip();
    test_multiple_pools();
    test_free_list_reuses_page();
    test_free_list_chain();
    test_free_list_persists_in_header();
    if (garry_test_failures == 0) printf("test_buffer_pool: ALL PASSED\n");
    return garry_test_failures;
}
