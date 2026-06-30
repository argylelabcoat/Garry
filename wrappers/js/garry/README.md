# garry

Node.js N-API binding for the [Garry](https://github.com/argylelabcoat/garry) storage engine.

## Install

```bash
npm install garry
```

Requires the Garry shared library (`libgarry`) on your library path.

## Quick Start

```javascript
const { Database } = require('garry');

const db = new Database('my.db');

db.set('greeting', 'hello world');
console.log(db.get('greeting')); // 'hello world'

db.set('counter', 42);
console.log(db.get('counter')); // '42'

db.delete('greeting');
console.log(db.get('greeting')); // null

db.close();
```

## API

### `new Database(path, config?)`

Opens or creates a database at `path`.

Optional `config` object:
- `pool_size` — number of pages in the in-memory pool (max 8)
- `page_size` — page size in bytes (512–65536, must be power of two)
- `max_record_size` — maximum record size in bytes

### `db.get(key)` → `string | null`

Retrieve a value by key. Returns `null` if not found.

### `db.set(key, value)`

Insert or update a key-value pair. Both `key` and `value` are strings (converted to UTF-8).

### `db.delete(key)`

Delete a key.

### `db.exists(key)` → `boolean`

Check if a key exists.

### `db.beginTransaction()` → `Transaction`

Begin an explicit transaction. Must call `commit()` or `rollback()`.

### `db.cursor(prefix?)` → `Cursor`

Open a cursor for prefix-scoped iteration.

### `db.close()`

Close the database and release resources.

### Transaction

```javascript
const txn = db.beginTransaction();
txn.set('key', 'value');
txn.commit(); // or txn.rollback()
```

### Cursor

```javascript
const cur = db.cursor('users/');
for (const { key, value } of cur) {
    console.log(key, value);
}
cur.close();
```

## Key Encoder

```javascript
const { encodeKey, decodeKey, combineKey } = require('garry/lib/encoder');

const key = encodeKey('users', 'matthew');
const parts = decodeKey(key); // ['users', 'matthew']
const extended = combineKey(key, 'email'); // encodeKey('users', 'matthew', 'email')
```

## Binary Codec

```javascript
const codec = require('garry/lib/codec');

const encoded = codec.encode(42);   // Buffer with tag + payload
const decoded = codec.decode(encoded); // 42
```

Supported types: `null`, `boolean`, `number` (int32/uint32/float64), `string`, `Buffer`, `bigint`.

## Serializer

Flatten objects to hierarchical key-value pairs for storage:

```javascript
const { serialize, deserialize } = require('garry/lib/serializer');

const pairs = serialize('user', { name: 'matthew', age: 30 });
// [{ key: encodeKey('user'), value: Buffer(0) },  // container marker
//  { key: encodeKey('user', 'name'), value: ... },
//  { key: encodeKey('user', 'age'), value: ... }]

const obj = deserialize('user', pairs);
// { name: 'matthew', age: 30 }
```

## Tests

```bash
DYLD_LIBRARY_PATH=../../../build npm test
```
