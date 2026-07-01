# Plan: Delphi + Free Pascal Compatibility for the Garry Wrapper

## Context

The Delphi OOP wrapper (`wrappers/delphi/garry/`) provides a class-based API
over the C library. It must compile under both Delphi and Free Pascal (FPC)
3.2.2+ from a single codebase, with no `{$IFDEF}` forks for the type system.

A previous analysis doc misdiagnosed the compatibility problems as
fundamental FPC language deficits. Isolation testing proved the real causes
were narrow and fixable. Most fixes are already applied in the current tree;
this plan covers the remaining work and codifies the conventions to prevent
regression.

## Root Causes (verified by isolation testing)

### Cause 1: `TBytes` type-name collision

FPC's `SysUtils` unit defines `TBytes = array of Byte`. The wrapper's
`garry_types.pas` also defined `TBytes = array of Byte`. When a unit imports
`garry_types` in the interface section and `SysUtils` in the implementation
section, FPC resolves `TBytes` to two *distinct* types depending on section.
Overloaded method signatures declared with `garry_types.TBytes` in the
interface no longer match implementations that see `SysUtils.TBytes`.

This is not an FPC bug — it is standard Pascal scoping. The fix is a unique
type name.

### Cause 2: Missing `overload` directives

FPC `{$MODE DELPHI}` requires the `overload` directive on *every* declaration
in an overload set. Delphi infers it. The original code had `overload` on
`SetKeyValue` but not on `Get`, `Delete`, `Exists`, causing cascading
"function header doesn't match" errors.

### Cause 3: `FPConvert` unit does not exist

`garry_codec.pas` referenced `FPConvert` under `{$IFDEF FPC}` for UTF-8
conversion helpers. This unit does not ship with FPC 3.2.2. FPC's `SysUtils`
already provides `UTF8Encode` (returns `RawByteString`) and
`UTF8ToString` (accepts `RawByteString`). The gap is just a type bridge
between `RawByteString` and the wrapper's byte-array type.

### Cause 4: Delphi dynamic-array constructor syntax in tests

Delphi supports `TBytes($48, $65, $6C, $6C, $6F)` as an inline array
constructor. FPC does not support this syntax for dynamic arrays. Tests must
use a helper function or explicit `SetLength` + element assignment.

## Conventions (the 4-step proposal, codified)

### Step 1: Unique type names

All wrapper-specific types must have names that cannot collide with RTL
units (`SysUtils`, `Classes`, etc.).

| Wrong | Right |
|-------|-------|
| `TBytes` | `TGarryBytes` |
| `TStringArray` | `TGarryStringArray` |

The type is defined once in `garry_types.pas` and used everywhere. Delphi and
FPC both accept this identically — it is just a type alias for
`array of Byte`.

**Status:** Done in `garry_types.pas`, `garry.pas`, `garry_codec.pas`,
`garry_test.lpr`. Not yet done in `garry_test.dpr`.

### Step 2: `overload` on every overloaded method declaration

Every method in an overload set must carry the `overload` directive in the
class declaration. FPC requires it; Delphi ignores the redundancy.

```pascal
function Get(const Key: string): TGarryBytes; overload;
function Get(const Key: TGarryBytes): TGarryBytes; overload;
```

The implementation section does **not** need `overload` — FPC matches by
signature. This was verified by isolation testing.

**Status:** Done in `garry.pas` (all 8 overload methods have `overload`).

### Step 3: Avoid FPC-isms; use `SysUtils` with explicit `RawByteString` conversion

Do not reference units that don't exist in FPC (`FPConvert`). For UTF-8
string conversion, bridge through `RawByteString` explicitly:

```pascal
// Encode: string -> RawByteString -> TGarryBytes
var S: RawByteString;
S := UTF8Encode(Value);
SetLength(Bytes, Length(S));
if Length(S) > 0 then
  Move(S[1], Bytes[0], Length(S));

// Decode: TGarryBytes -> RawByteString -> string
SetLength(S, Length(Sub));
if Length(Sub) > 0 then
  Move(Sub[0], S[1], Length(Sub));
Result := UTF8ToString(S);
```

Keep `{$MODE DELPHI}` in all units — it provides the best common ground. The
existing `{$IFDEF FPC} {$MODE DELPHI} {$ENDIF}` pattern is fine; under Delphi
the `{$IFDEF FPC}` block is skipped and Delphi's native Delphi mode applies.

**Status:** Done in `garry_codec.pas`.

### Step 4: No Delphi-only syntax in shared test code

Do not use `TBytes(...)` array constructor syntax. Use a helper:

```pascal
function BytesOf(const Vals: array of Byte): TGarryBytes;
var
  I: Integer;
begin
  SetLength(Result, Length(Vals));
  for I := 0 to High(Vals) do
    Result[I] := Vals[I];
end;

// Usage:
DB.SetKeyValue('hello', BytesOf([$48, $65, $6C, $6C, $6F]));
```

**Status:** Done in `garry_test.lpr`. Not yet done in `garry_test.dpr`.

## Work Completed

### Task 1 + 2: Merge test files into single `garry_test.lpr` (DONE)

The Delphi-format `garry_test.dpr` was merged into `garry_test.lpr` and
deleted. The merged file contains all 16 tests (11 original + 5 deep-nesting)
and compiles clean under FPC 3.2.2 with 0 errors.

Changes applied:
1. Deep-nesting record types (`TLevel10`..`TDeepRoot`) moved into `.lpr`.
2. `EncodeDeepString`, `DecodeDeepString`, `SerializeDeepRoot`,
   `DeserializeDeepRoot` moved into `.lpr`, all using `TGarryBytes`.
3. `TestDeepNesting` converted to a procedure taking `DB: TGarryDatabase`
   as a parameter (fixes the FPC scoping issue where the original `.dpr`
   referenced main-program `var` block from a nested procedure).
4. All `TBytes(...)` constructor calls replaced with `BytesOf([...])`.
5. All `TBytes` type references replaced with `TGarryBytes`.
6. Test 11 (reopen) renumbered to Test 17 to follow the deep-nesting block.

### Task 3: Delphi compilation status (DOCUMENTED)

FPC 3.2.2 is the verified target — all files compile with 0 errors and all
16 tests pass. Delphi compilation is **by-design but unverified** (no Delphi
compiler available in this environment).

The changes are Delphi-safe:
- `TGarryBytes` is a valid Delphi type alias for `array of Byte`.
- `overload` directives are redundant-but-legal in Delphi.
- `RawByteString` is a Delphi type.
- `BytesOf` helper uses standard Pascal.
- `{$IFDEF FPC}` blocks are skipped under Delphi.

If Delphi is available, verify with:
```
dcc garry_types.pas
dcc garry.pas
dcc garry_codec.pas
dcc garry_test.lpr
```

### Task 4: CI build rule for FPC (DONE)

A shell script `wrappers/delphi/garry/run_fpc_test.sh` was added. It
compiles all units and runs the test, exiting non-zero on any failure.

## Verification Checklist

| Check | Command | Expected |
|-------|---------|----------|
| `garry_types.pas` compiles | `fpc -Mdelphi garry_types.pas` | 0 errors |
| `garry.pas` compiles | `fpc -Mdelphi garry.pas` | 0 errors |
| `garry_codec.pas` compiles | `fpc -Mdelphi garry_codec.pas` | 0 errors |
| `garry_test.lpr` compiles | `fpc -Mdelphi garry_test.lpr` | 0 errors |
| Tests pass | `./garry_test` | "All tests passed!" |
| No `TBytes` in source | `grep -rn '\bTBytes\b' *.pas *.lpr` | no output |
| No `FPConvert` references | `grep -rn FPConvert *.pas` | no output |
| No `.dpr` file | `ls *.dpr` | no output |

## What Not To Do

- **Do not** drop to pure C bindings. The OOP wrapper works; the
  compatibility problem was a type-name collision, not a language deficit.
- **Do not** create a separate `garry_fpc.pas`. One codebase compiles under
  both.
- **Do not** use `{$MODE OBJFPC}`. `{$MODE DELPHI}` gives the best common
  ground and the fixes don't require FPC-specific mode.
- **Do not** add `{$IFDEF FPC}` blocks for type definitions. A unique type
  name eliminates the need.
- **Do not** add `overload` to the implementation section. FPC matches by
  signature; adding it is harmless but unnecessary noise.