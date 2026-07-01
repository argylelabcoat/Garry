# Godot GDExtension Analysis

## Overview

The Godot GDExtension binding provides a `MeowDB` Node class for GDScript. The extension loads correctly, but has issues with file path handling.

## Test Results

| Test | Status | Notes |
|------|--------|-------|
| Class found | ✅ | `MeowDB` class exists |
| Instance created | ✅ | Can instantiate `MeowDB.new()` |
| Open with `/tmp/` path | ✅ | `db.open("/tmp/test.db")` works |
| Open with `user://` path | ❌ | `db.open("user://game.db")` fails |
| Open with `res://` path | ❌ | `db.open("res://test.db")` fails |
| String operations | ⚠️ | Only works with absolute paths |

## Root Cause

The GDExtension directly passes Godot paths to the C library's `garry_database_open()` function. The C library doesn't understand Godot's virtual file system paths:

- `user://` → Maps to `~/.local/share/godot/app_userdata/ProjectName/` on Linux
- `res://` → Maps to the project directory
- `C://` → Windows paths

The C library expects regular file system paths like `/tmp/test.db` or `./mydb.db`.

## Impact

- **High:** Cannot use standard Godot path conventions
- **Workaround:** Use absolute paths (`/tmp/` or `Application.persistentDataPath`)

## Fix Required

The GDExtension needs to convert Godot paths to actual file system paths before calling the C library:

```csharp
// Pseudocode for fix
string ConvertGodotPath(string godotPath)
{
    if (godotPath.StartsWith("user://"))
        return ProjectSettings.GlobalizePath(godotPath);
    if (godotPath.StartsWith("res://"))
        return ProjectSettings.GlobalizePath(godotPath);
    return godotPath; // Already absolute
}
```

## Effort Estimate

**4-6 hours** to implement proper path conversion:
1. Add `ProjectSettings.GlobalizePath()` call in the open method
2. Handle edge cases (relative paths, Windows vs Unix)
3. Add tests for different path formats
4. Update documentation

## Current Workaround

Use absolute paths in GDScript:

```gdscript
# Instead of:
db.open("user://game.db")

# Use:
db.open(OS.get_user_data_dir() + "/game.db")
# Or:
db.open(ProjectSettings.globalize_path("user://game.db"))
```

## Additional Issues

### 1. Resource Leak Warning

**Symptom:** `WARNING: 1 ObjectDB instance was leaked at exit`

**Cause:** The `MeowDB` instance is not properly freed when the script exits.

**Fix:** Add `queue_free()` or implement `_notification(NOTIFICATION_PREDELETE)` in the GDExtension.

### 2. Transaction Methods

**Status:** Implemented but not tested due to open() issues.

**Current implementation:**
- `begin_transaction()` → stores transaction handle
- `commit()` → commits stored transaction
- `rollback()` → rolls back stored transaction

**Testing blocked by:** Cannot open database with Godot paths.

## Recommendation

Fix the path conversion issue first, as it blocks all other testing and usage. The resource leak is minor and can be addressed later.
