# GarrySharp - C# Bindings

C# wrapper for the [Garry](https://github.com/argylelabcoat/garry) storage engine with object mapping support.

## Prerequisites

- .NET 8.0+
- Garry C library (`libgarry.dylib` / `libgarry.so` / `garry.dll`)

## Installation

```bash
dotnet add package GarrySharp
```

Or build from source:

```bash
cd wrappers/c-sharp
dotnet build
```

## Quick Start

```csharp
using GarrySharp;

// Open or create a database
using var db = new GarryDatabase("my.db");

// Basic operations
db.Set("greeting", Encoding.UTF8.GetBytes("hello"));
byte[] value = db.Get("greeting");
Console.WriteLine(Encoding.UTF8.GetString(value));

// Object operations
var person = new Person { Name = "Alice", Age = 25 };
db.Set("players", person);

var loaded = db.Get<Person>("players");
Console.WriteLine(loaded.Name);
```

## Object Mapping

Flatten objects to hierarchical key trees:

```csharp
[GarryKey("person_name")]
public class Person
{
    public string Name { get; set; }
    public int Age { get; set; }
    public Address Address { get; set; }
    [GarryIgnore]
    public string Secret { get; set; }
}

public class Address
{
    public string City { get; set; }
    public string Zip { get; set; }
}

// Serialization produces:
// [person]              → container marker
// [person.person_name]  → "Alice" (leaf)
// [person.Age]          → 25 (leaf)
// [person.Address]      → container marker
// [person.Address.City] → "NYC" (leaf)
// [person.Address.Zip]  → "10001" (leaf)
```

## Transactions

```csharp
using var db = new GarryDatabase("my.db");

// Explicit transaction
using var txn = db.BeginTransaction();
txn.Set("key1", value1);
txn.Set("key2", value2);
txn.Commit();

// Auto-transaction (default)
db.Set("key", value); // auto-commits
```

## Key Encoding

```csharp
// Composite keys
byte[] key = KeyEncoder.Encode("users", "alice", "profile");
string[] parts = KeyEncoder.DecodeParts(key);
// ["users", "alice", "profile"]

// Combine keys
byte[] parent = KeyEncoder.Encode("users");
byte[] child = KeyEncoder.Combine(parent, "alice");
```

## Binary Codec

```csharp
// Encode values
byte[] encoded = BinaryCodec.Encode(42);
byte[] encoded = BinaryCodec.Encode("hello");
byte[] encoded = BinaryCodec.Encode(3.14f);

// Decode values
object decoded = BinaryCodec.Decode(encoded);
```

Supported types: `null`, `bool`, `byte`, `sbyte`, `short`, `ushort`, `int`, `uint`, `long`, `ulong`, `float`, `double`, `string`, `byte[]`, `DateTime`, `Guid`.

## Cursor Iteration

```csharp
using var db = new GarryDatabase("my.db");

var results = db.Query("users");
foreach (var (key, value) in results)
{
    Console.WriteLine($"{key}: {value.Length} bytes");
}

// Typed query
var users = db.Query<Person>("users");
```

## Configuration

```csharp
var config = new GarryConfig
{
    PoolSize = 16,
    PageSize = 8192,
    MaxRecordSize = 32768,
    UseCompression = true
};

using var db = new GarryDatabase("my.db", config);
```

## API Reference

| Class | Description |
|-------|-------------|
| `GarryDatabase` | Main database wrapper |
| `GarryTransaction` | Transaction with auto-rollback |
| `GarryConfig` | Database configuration |
| `GarrySerializer` | Object mapper |
| `BinaryCodec` | Binary value encoding |
| `KeyEncoder` | Key encoding/decoding |
| `GarryKeyAttribute` | Override key name |
| `GarryIgnoreAttribute` | Exclude from serialization |

## Testing

```bash
dotnet test
```

## License

BSD-3-Clause
