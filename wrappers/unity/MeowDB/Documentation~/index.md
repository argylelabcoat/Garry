# MeowDB

C# wrapper for the Garry storage engine, designed for Unity.

## Features

- **Object Mapper** — Flatten/reconstruct .NET objects to hierarchical key trees
- **Binary Serialization** — Efficient binary encoding for all primitive types
- **Transaction Support** — Atomic operations with auto-rollback
- **Cross-Platform** — Windows, macOS, Linux, iOS support

## Installation

### Via Git URL (Unity Package Manager)

1. Open **Window → Package Manager**
2. Click **+** → **Add package from git URL**
3. Enter: `https://github.com/argylelabcoat/garry.git#upm`

### Via Local Folder

1. Copy `wrappers/unity/MeowDB` into your project's `Packages/` folder
2. Unity will automatically detect the package

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

## Defining Data Classes

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

## Key Mapping

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

## Attributes

- `[GarryKey("name")]` — Override the key subscript name
- `[GarryIgnore]` — Exclude a property from serialization

## Platform Setup

See `Plugins/README.md` for native library installation instructions.

## Requirements

- Unity 2021.3+
- .NET Standard 2.1
- Native Garry library (see Plugins folder)

## License

BSD-3-Clause
