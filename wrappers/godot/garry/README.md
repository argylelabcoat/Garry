# Garry Godot GDExtension

GDExtension binding for the Garry storage engine, exposing a `MeowDB` Node to GDScript.

## Building

```bash
cd wrappers/godot/garry
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

The built library is placed in `project/addons/garry/bin/`.

## Usage

```gdscript
var db = MeowDB.new()
db.open("user://game.db")

# String operations
db.set_string("player_name", "Hero")
var name = db.get_string("player_name")

# Raw bytes
var data = PackedByteArray([1, 2, 3])
db.set("config", data)
var raw = db.get("config")

# Transactions
db.begin_transaction()
db.set_string("hp", "100")
db.set_string("mp", "50")
db.commit()

db.close()
```

## Testing

Uses GUT. Open `project/` in Godot and run tests from the GUT panel.
