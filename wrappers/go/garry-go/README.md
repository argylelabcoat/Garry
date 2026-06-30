# garry-go

Go CGO bindings for the [Garry](https://github.com/argylelabcoat/garry) storage engine.

## Prerequisites

- Go 1.21+
- Garry C library installed (`libgarry.so` / `libgarry.dylib`)

## Installation

```bash
go get github.com/argylelabcoat/garry-go
```

## Quick Start

```go
package main

import (
    "fmt"
    garry "github.com/argylelabcoat/garry-go"
)

func main() {
    db, _ := garry.Create("my.db")
    defer db.Close()

    txn := db.Begin()
    db.Set(txn, []byte("hello"), []byte("world"))
    txn.Commit()

    txn2 := db.Begin()
    val, _ := db.Get(txn2, []byte("hello"))
    fmt.Println(string(val)) // "world"
    txn2.Rollback()
}
```

## Key Encoding

Composite keys use length-prefixed encoding:

```go
key := garry.EncodeKey("users", "alice")
parts := garry.DecodeKey(key) // ["users", "alice"]
```

## Struct Serialization

```go
type User struct {
    Name string `garry:"name"`
    Age  int32  `garry:"age"`
}

pairs := garry.Serialize("users", user)
var result User
garry.Deserialize("users", pairs, &result)
```

## Binary Codec

```go
encoded := garry.EncodeValue("hello")
decoded := garry.DecodeValue(encoded) // "hello"
```

Supported types: `nil`, `bool`, `int32`, `uint32`, `int64`, `float64`, `string`, `[]byte`.

## API

| Function | Description |
|----------|-------------|
| `Create(path)` | Create a new database |
| `Open(path)` | Open an existing database |
| `db.Begin()` | Begin a transaction |
| `txn.Commit()` | Commit transaction |
| `txn.Rollback()` | Rollback transaction |
| `db.Get(txn, key)` | Get a value |
| `db.Set(txn, key, value)` | Set a value |
| `db.Delete(txn, key)` | Delete a key |
| `db.Exists(txn, key)` | Check key existence |
| `db.Count(txn)` | Count all keys |
| `db.CursorOpen(txn, prefix)` | Open a cursor |
| `db.First(txn)` / `db.Last(txn)` | First/last key |
| `db.NextKey(txn, after)` / `db.PrevKey(txn, before)` | Adjacent keys |
| `db.ForEach(txn, prefix, visitor)` | Iterate with callback |
| `db.SetStr(txn, key, value)` | String convenience |
| `db.GetStr(txn, key)` | String convenience |

## License

BSD-3-Clause
