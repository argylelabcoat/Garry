use std::os::raw::{c_char, c_int};

pub type GarryBool = c_int;

pub const GARRY_TRUE: GarryBool = 1;
pub const GARRY_FALSE: GarryBool = 0;

pub const GARRY_OK: i32 = 0;
pub const GARRY_ERR_NOT_FOUND: i32 = 1;
pub const GARRY_ERR_LOCK_CONFLICT: i32 = 2;
pub const GARRY_ERR_IO: i32 = 3;
pub const GARRY_ERR_CORRUPT: i32 = 4;
pub const GARRY_ERR_INVALID_ARG: i32 = 5;
pub const GARRY_ERR_NOMEM: i32 = 6;
pub const GARRY_ERR_BUFFER_TOO_SMALL: i32 = 7;

pub const GARRY_DATA_NOT_FOUND: i32 = 0;
pub const GARRY_DATA_HAS_CHILDREN: i32 = 1;
pub const GARRY_DATA_HAS_VALUE: i32 = 10;
pub const GARRY_DATA_HAS_BOTH: i32 = 11;

pub const GARRY_MAX_KEY_SIZE: usize = 256;
pub const GARRY_MAX_SUBSCRIPTS: usize = 16;

#[repr(C)]
pub struct garry_database {
    _private: [u8; 0],
}

#[repr(C)]
pub struct garry_cursor {
    _private: [u8; 0],
}

pub type garry_txn = i32;

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct garry_config {
    pub pool_size: i32,
    pub max_record_size: i32,
    pub page_size: i32,
    pub max_txns: i32,
    pub max_versions: i32,
    pub max_key_size: i32,
    pub max_subscripts: i32,
    pub compression: i32,
    pub btree_flags: i32,
}

extern "C" {
    pub fn garry_config_default() -> garry_config;

    pub fn garry_database_create(path: *const c_char) -> *mut garry_database;
    pub fn garry_database_create_with_config(
        path: *const c_char,
        config: garry_config,
    ) -> *mut garry_database;
    pub fn garry_database_open(path: *const c_char) -> *mut garry_database;
    pub fn garry_database_close(db: *mut garry_database);

    pub fn garry_txn_begin(db: *mut garry_database) -> garry_txn;
    pub fn garry_txn_commit(db: *mut garry_database, txn: garry_txn);
    pub fn garry_txn_rollback(db: *mut garry_database, txn: garry_txn);

    pub fn garry_get(
        db: *mut garry_database,
        txn: garry_txn,
        key: *const u8,
        klen: i32,
        value: *mut u8,
        vlen: *mut i32,
    ) -> i32;

    pub fn garry_set(
        db: *mut garry_database,
        txn: garry_txn,
        key: *const u8,
        klen: i32,
        value: *const u8,
        vlen: i32,
    ) -> i32;

    pub fn garry_delete(
        db: *mut garry_database,
        txn: garry_txn,
        key: *const u8,
        klen: i32,
    ) -> i32;

    pub fn garry_data(
        db: *mut garry_database,
        txn: garry_txn,
        key: *const u8,
        klen: i32,
    ) -> i32;

    pub fn garry_get_default(
        db: *mut garry_database,
        txn: garry_txn,
        key: *const u8,
        klen: i32,
        def: *const u8,
        dlen: i32,
        value: *mut u8,
        vlen: *mut i32,
    ) -> i32;

    pub fn garry_exists(
        db: *mut garry_database,
        txn: garry_txn,
        key: *const u8,
        klen: i32,
    ) -> GarryBool;

    pub fn garry_count(db: *mut garry_database, txn: garry_txn) -> i32;

    pub fn garry_first(
        db: *mut garry_database,
        txn: garry_txn,
        key: *mut u8,
        klen: *mut i32,
    ) -> GarryBool;

    pub fn garry_last(
        db: *mut garry_database,
        txn: garry_txn,
        key: *mut u8,
        klen: *mut i32,
    ) -> GarryBool;

    pub fn garry_next_key(
        db: *mut garry_database,
        txn: garry_txn,
        after: *const u8,
        after_len: i32,
        key: *mut u8,
        klen: *mut i32,
    ) -> GarryBool;

    pub fn garry_prev_key(
        db: *mut garry_database,
        txn: garry_txn,
        before: *const u8,
        before_len: i32,
        key: *mut u8,
        klen: *mut i32,
    ) -> GarryBool;

    pub fn garry_cursor_open(
        db: *mut garry_database,
        txn: garry_txn,
        prefix: *const u8,
        plen: i32,
    ) -> *mut garry_cursor;

    pub fn garry_cursor_next(
        cur: *mut garry_cursor,
        key: *mut u8,
        klen: *mut i32,
        value: *mut u8,
        vlen: *mut i32,
    ) -> GarryBool;

    pub fn garry_cursor_next_key(
        cur: *mut garry_cursor,
        key: *mut u8,
        klen: *mut i32,
    ) -> GarryBool;

    pub fn garry_cursor_close(cur: *mut garry_cursor);

    pub fn garry_empty_byte_array(out: *mut u8);

    pub fn garry_make_key_2(p1: *const c_char, p2: *const c_char) -> GarryKeyTupleFfi;
    pub fn garry_make_key_3(
        p1: *const c_char,
        p2: *const c_char,
        p3: *const c_char,
    ) -> GarryKeyTupleFfi;
    pub fn garry_make_key_4(
        p1: *const c_char,
        p2: *const c_char,
        p3: *const c_char,
        p4: *const c_char,
    ) -> GarryKeyTupleFfi;
    pub fn garry_encode_key_tuple(tuple: *const GarryKeyTupleFfi, out: *mut u8);
    pub fn garry_byte_compare(a: *const u8, alen: i32, b: *const u8, blen: i32) -> i32;

    pub fn garry_strerror(status: i32) -> *const c_char;
}

#[repr(C)]
pub struct GarryKeyTupleFfi {
    pub parts: [*const u8; GARRY_MAX_SUBSCRIPTS],
    pub counts: [i32; GARRY_MAX_SUBSCRIPTS],
    pub count: i32,
}
