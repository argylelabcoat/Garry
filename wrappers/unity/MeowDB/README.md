# MeowDB - Unity Wrapper

Unity Package Manager wrapper for the [Garry](https://github.com/argylelabcoat/garry) storage engine.

## Installation

### Via Git URL (UPM)

1. Open **Window → Package Manager**
2. Click **+** → **Add package from git URL**
3. Enter: `https://github.com/argylelabcoat/garry.git#upm`

### Via Local Folder

Copy `wrappers/unity/MeowDB` into your project's `Packages/` folder.

## Quick Start

```csharp
using MeowDB;

// Open or create a database
using var db = new MeowDatabase(Application.persistentDataPath + "/game.db");

// Store an object
var player = new PlayerData
{
    Name = "Alice",
    Health = 100,
    Position = new Vector3Data { X = 1.0f, Y = 2.0f, Z = 3.0f }
};
db.Set("players/alice", player);

// Retrieve it
var loaded = db.Get<PlayerData>("players/alice");

// Query with prefix
var allPlayers = db.Query<PlayerData>("players");
```

## Object Mapping

```csharp
using MeowDB;

[System.Serializable]
public class PlayerData
{
    public string Name { get; set; }
    public int Health { get; set; }
    public Vector3Data Position { get; set; }
    public string[] Inventory { get; set; }
}

[System.Serializable]
public class Vector3Data
{
    public float X { get; set; }
    public float Y { get; set; }
    public float Z { get; set; }
}
```

### Key Mapping

Properties map directly to key subscripts:

```
PlayerData { Name = "Alice", Position = { X = 1.0f } }
```

Stored as:
```
[root]                → (container)
[root.Name]           → "Alice" (binary)
[root.Position]       → (container)
[root.Position.X]     → 1.0f (binary)
```

### Attributes

```csharp
// Override key name
[GarryKey("player_name")]
public string Name { get; set; }

// Exclude from serialization
[GarryIgnore]
public string Secret { get; set; }
```

## Transactions

```csharp
using var db = new MeowDatabase(path);

// Explicit transaction
using var txn = db.BeginTransaction();
txn.Set("key1", value1);
txn.Set("key2", value2);
txn.Commit();

// Auto-transaction (default)
db.Set("key", value); // auto-commits
```

## Raw Operations

```csharp
// String operations
db.Set_string("greeting", "hello");
string value = db.Get_string("greeting");

// Byte operations
db.Set("data", new byte[] { 1, 2, 3 });
byte[] raw = db.Get("data");
```

## Platform Support

- Windows (x86_64)
- macOS (x86_64, ARM64)
- Linux (x86_64)
- iOS (static linking)

## Native Library Setup

See `Plugins/README.md` for instructions on placing the native library.

## Requirements

- Unity 2021.3+
- .NET Standard 2.1

## Testing

Tests are in `Tests/Editor/` and use Unity's Test Framework (NUnit).

## License

BSD-3-Clause
