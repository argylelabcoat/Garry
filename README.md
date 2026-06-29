# Garry

**Garry** (*global array*) — a C89 transactional key-value store.
Mascot: *Garry the Arborist* (a B-tree surgeon). I'm still trying to draw him, sorry. 
 

A portable, embeddable storage engine with B-tree indexing, MVCC
version chains, prefix-compressed hierarchical keys, write-ahead
logging with crash recovery, per-key logical locking, LZ4 compression,
and thread-safe concurrent access. Zero external dependencies.

## Quick Start

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage Examples

### Basic Key-Value Operations

```c
#include <garry/api.h>

garry_database *db = garry_database_create("my.db");
garry_txn txn = garry_txn_begin(db);

garry_set_str(db, txn, "hello", "world");
garry_txn_commit(db, txn);

txn = garry_txn_begin(db);
char result[GARRY_MAX_KEY_SIZE];
garry_get_str(db, txn, "hello", result, sizeof(result));
// result = "world"
garry_txn_rollback(db, txn);
garry_database_close(db);
```

### Hierarchical Keys

```c
#include <garry/keysplit.h>

/* Split path into length-prefixed key */
garry_byte_array key;
garry_i32 n = garry_key_split("users/matthew/articles/A04", '/', key);

/* Use with set/get */
garry_txn txn = garry_txn_begin(db);
garry_set(db, txn, key, n, value, vlen);

/* Unsplit back to string */
char buf[GARRY_MAX_KEY_SIZE];
garry_i32 len = garry_key_unsplit(key, n, '/', buf, sizeof(buf));
// buf = "users/matthew/articles/A04"
```

### Cursor Iteration

```c
garry_u8 key[GARRY_MAX_KEY_SIZE], val[GARRY_MAX_KEY_SIZE];
garry_i32 klen, vlen;

garry_txn txn = garry_txn_begin(db);
garry_cursor *cur = garry_cursor_open(db, txn, NULL, 0);
while (garry_cursor_next(cur, key, &klen, val, &vlen)) {
    printf("%.*s = %.*s\n", klen, key, vlen, val);
}
garry_cursor_close(cur);

/* Keys only (no value overhead) */
cur = garry_cursor_open(db, txn, NULL, 0);
while (garry_cursor_next_key(cur, key, &klen)) {
    printf("key: %.*s\n", klen, key);
}
garry_cursor_close(cur);
```

### Callback Iteration

```c
void my_visitor(const garry_u8 *key, garry_i32 klen,
                const garry_u8 *val, garry_i32 vlen, void *ud) {
    printf("%.*s\n", klen, key);
}
garry_for_each(db, txn, NULL, 0, my_visitor, NULL);
```

### Navigation

```c
garry_u8 key[GARRY_MAX_KEY_SIZE];
garry_i32 klen;

memset(key, 0, sizeof(key));
klen = 0;
if (garry_first(db, txn, key, &klen)) { /* found first */ }

memset(key, 0, sizeof(key));
klen = 0;
if (garry_last(db, txn, key, &klen)) { /* found last */ }

if (garry_next_key(db, txn, (const garry_u8*)"aaa", 3, key, &klen)) { /* next */ }

if (garry_exists(db, txn, key, klen)) { /* exists */ }
garry_i32 count = garry_count(db, txn);
```

### Custom Configuration

```c
garry_config cfg = garry_config_default();
cfg.pool_size = 4;
cfg.compression = GARRY_COMPRESS_LZ4;
garry_database *db = garry_database_create_with_config("my.db", cfg);
```

## Building

### POSIX (Linux/macOS)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
cmake --install build --prefix /usr/local
```

### Windows (MinGW-w64)

```bash
cmake -S . -B build-win32 \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc
cmake --build build-win32
```

## Architecture

- **B-tree** with 3 keys per leaf, prefix-compressed keys
- **Buffer pool** with 8-slot LRU eviction, dirty page flushing
- **MVCC** via version chains on slotted pages with overflow support
- **WAL** for crash recovery with two-phase redo
- **LZ4** compression for large values
- **Thread-safe** via per-slot rwlocks (POSIX pthreads / Win32 SRWLOCK)

## License

BSD-3-Clause. See LICENSE file.
