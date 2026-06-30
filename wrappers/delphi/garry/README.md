# Garry Delphi/Free Pascal Bindings

Delphi and Free Pascal bindings for the [Garry storage engine](https://github.com/argylelabcoat/garry).

## Files

| File | Purpose |
|------|---------|
| `garry_types.pas` | Type definitions, status codes, config record |
| `garry.pas` | Main bindings — low-level cdecl imports + OOP wrapper |
| `garry_codec.pas` | Binary codec for encoding/decoding typed values |
| `garry_test.dpr` | Delphi test project |
| `garry_test.lpr` | Free Pascal test project |

> **Note:** `*.ppu` files are Free Pascal compilation artifacts and should not be committed to version control.

## Usage

### Quick Start

```pascal
uses garry, garry_types;

var
  DB: TGarryDatabase;
  Value: TBytes;
begin
  DB := TGarryDatabase.Create('mydb.db', True);
  try
    DB.SetKeyValue('hello', TBytes($48, $65, $6C, $6C, $6F));
    Value := DB.Get('hello');
  finally
    DB.Free;
  end;
end;
```

### Composite Keys

```pascal
var
  Key: TBytes;
begin
  Key := EncodeKey(['users', 'matthew', 'profile']);
  DB.SetKeyValue(Key, MyData);
end;
```

### Binary Codec

```pascal
uses garry_codec;

var
  Encoded: TBytes;
begin
  Encoded := EncodeValueInt32(42);
  Encoded := EncodeValueString('Hello');
  Encoded := EncodeValueDouble(3.14);

  WriteLn(DecodeInt32(Encoded));
  WriteLn(DecodeString(Encoded));
end;
```

### Transactions

```pascal
var
  Txn: Integer;
begin
  Txn := DB.BeginTxn;
  try
    DB.SetKeyValue('key1', Data1);
    DB.SetKeyValue('key2', Data2);
    DB.Commit(Txn);
  except
    DB.Rollback(Txn);
    raise;
  end;
end;
```

### Cursor Iteration

```pascal
var
  Cursor: TGarryCursor;
  Key, Value: TBytes;
begin
  Cursor := DB.OpenCursorAll;
  try
    while Cursor.Next(Key, Value) do
      WriteLn(DecodeKey(Key), ' => ', Length(Value), ' bytes');
  finally
    Cursor.Free;
  end;
end;
```

## Building

### Delphi

```
dcc32 garry_test.dpr
```

### Free Pascal

```
fpc garry_test.lpr
```

## Requirements

- Shared library: `libgarry.so` (Linux), `libgarry.dylib` (macOS), `garry.dll` (Windows)
- Library must be in the library search path or working directory

## API Coverage

| C API | Pascal Binding |
|-------|---------------|
| `garry_database_create` | `garry_database_create` / `TGarryDatabase.Create` |
| `garry_database_open` | `garry_database_open` / `TGarryDatabase.Create` |
| `garry_database_close` | `garry_database_close` / `TGarryDatabase.Destroy` |
| `garry_txn_begin/commit/rollback` | Direct + `TGarryDatabase.BeginTxn/Commit/Rollback` |
| `garry_get/set/delete` | Direct + `TGarryDatabase.Get/SetKeyValue/Delete` |
| `garry_exists/count` | `TGarryDatabase.Exists/Count` |
| `garry_first/last/next_key/prev_key` | `TGarryDatabase.First/Last/NextKey/PrevKey` |
| `garry_cursor_*` | `TGarryCursor` class |
| `garry_for_each` | `garry_for_each` |
| `garry_key_split/unsplit` | `garry_key_split/unsplit` |
| `garry_make_key*` | `garry_make_key*` + `EncodeKey` |
| `garry_encode_key_tuple` | `garry_encode_key_tuple` |
| `garry_byte_compare` | `garry_byte_compare` |
| `garry_set_str/get_str` | `garry_set_str/get_str` |
| `garry_strerror` | `garry_strerror` / `GarryStatusToStr` |

## License

BSD-3-Clause — same as the Garry storage engine.
