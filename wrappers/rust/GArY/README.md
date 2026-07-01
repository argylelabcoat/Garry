# GArY - Rust Bindings

Rust FFI bindings for the [Garry](https://github.com/argylelabcoat/garry) storage engine.

## Prerequisites

- Rust 1.70+
- Garry C library built (`libgarry.a` or `libgarry.dylib`)

## Installation

Add to your `Cargo.toml`:

```toml
[dependencies]
garry = { path = "wrappers/rust/GArY" }
```

## Quick Start

```rust
use garry::{Database, Transaction};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Create or open a database
    let db = Database::create("my.db")?;
    
    // Simple key-value operations
    db.set(b"hello", b"world")?;
    let value = db.get(b"hello")?;
    println!("{}", String::from_utf8_lossy(&value));
    
    // Transaction-based operations
    let txn = db.begin_transaction()?;
    txn.set(b"key1", b"value1")?;
    txn.set(b"key2", b"value2")?;
    txn.commit()?;
    
    Ok(())
}
```

## Key Encoding

Composite keys use length-prefixed encoding:

```rust
use garry::encode_key;

// Create a composite key
let key = encode_key(&["users", "alice", "profile"]);

// Decode key parts
let parts = garry::decode_key(&key);
assert_eq!(parts, vec!["users", "alice", "profile"]);
```

## Object Serialization

Serialize Rust structs to hierarchical key-value pairs:

```rust
use garry::serializer::{Serialize, Deserialize, Value};

#[derive(Serialize, Deserialize)]
struct User {
    name: String,
    age: i32,
    email: Option<String>,
}

let user = User {
    name: "Alice".to_string(),
    age: 30,
    email: Some("alice@example.com".to_string()),
};

// Serialize to key-value pairs
let pairs = garry::serializer::serialize_object("users", &user);

// Deserialize from key-value pairs
let restored: User = garry::serializer::deserialize_object("users", &pairs)?;
```

## Binary Codec

Encode/decode values to Garry's binary format:

```rust
use garry::serializer::{encode, decode, Value};

// Encode a value
let encoded = encode(&Value::String("hello".to_string()));

// Decode it back
let decoded = decode(&encoded)?;
assert_eq!(decoded, Value::String("hello".to_string()));
```

Supported types: `Null`, `Bool`, `I32`, `I64`, `F32`, `F64`, `String`, `Bytes`, `Array`, `Map`.

## Cursor Iteration

```rust
use garry::Cursor;

let mut cursor = db.cursor(b"users")?;
while let Some((key, value)) = cursor.next()? {
    let parts = garry::decode_key(&key);
    println!("{}: {}", parts.join("/"), String::from_utf8_lossy(&value));
}
```

## Error Handling

All operations return `Result<T, GarryError>`:

```rust
use garry::GarryError;

match db.get(b"missing") {
    Ok(value) => println!("Found: {:?}", value),
    Err(GarryError::NotFound) => println!("Key not found"),
    Err(e) => println!("Error: {}", e),
}
```

## API Reference

| Type | Description |
|------|-------------|
| `Database` | Main database handle |
| `Transaction` | Transaction with commit/rollback |
| `Cursor` | Prefix-scoped iteration |
| `Value` | Dynamic value type for serialization |
| `GarryError` | Error enum |

| Function | Description |
|----------|-------------|
| `Database::create(path)` | Create a new database |
| `Database::open(path)` | Open existing database |
| `encode_key(parts)` | Create composite key |
| `decode_key(key)` | Decode composite key |
| `encode(value)` | Encode value to bytes |
| `decode(bytes)` | Decode bytes to value |

## Testing

```bash
cargo test
```

## License

BSD-3-Clause
