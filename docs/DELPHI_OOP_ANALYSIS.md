# Delphi/Free Pascal OOP Wrapper Analysis

## Overview

The Delphi/Free Pascal wrapper (`garry.pas`) provides an OOP layer over the C library. While the minimal test (`test_minimal.lpr`) works by directly calling C functions via dynamic loading, the OOP wrapper has compilation issues with Free Pascal 3.2.2.

## Current Status

- **Minimal test:** ✅ Works (5/5 tests pass)
- **OOP wrapper:** ❌ Fails to compile with Free Pascal 3.2.2

## Root Cause Analysis

### 1. `TBytes` Type Incompatibility

**Issue:** Free Pascal's `TBytes` (dynamic array of bytes) behaves differently from Delphi's `TBytes` in DELPHI mode.

**Symptoms:**
- Compiler treats `TBytes` as `{Dynamic} Array Of Byte` internally
- Function signatures with `TBytes` parameters don't match between declaration and implementation
- Method overloading with `const TBytes` vs `var TBytes` causes conflicts

**Example:**
```pascal
// Declaration
function Next(var Key, Value: TBytes): Boolean;

// Implementation
function TGarryCursor.Next(var Key, Value: TBytes): Boolean;
// Error: function header doesn't match any method of this class
```

**Why it happens:** Free Pascal's DELPHI mode is not 100% compatible with Delphi's type system, especially for managed types like dynamic arrays.

### 2. Function Signature Mismatches

**Issue:** Some functions have different parameter types between declaration and implementation.

**Example:**
```pascal
// Declaration in interface
function EncodeKey(const Parts: array of AnsiString): TBytes;

// Implementation
function EncodeKey(const Parts: array of AnsiString): TBytes;
// Error: function header doesn't match the previous declaration
```

**Why it happens:** Free Pascal treats `array of AnsiString` differently than Delphi in some contexts.

### 3. Generic Type Syntax

**Issue:** Free Pascal doesn't support some Delphi generic syntax.

**Example:**
```pascal
// This works in Delphi but not Free Pascal
type
  TArray<T> = array of T;
  
// Free Pascal requires explicit specialization
type
  TStringArray = array of AnsiString;
```

### 4. Uninitialized Result Variables

**Issue:** Free Pascal warns about uninitialized dynamic array results.

**Example:**
```pascal
function EncodeKey(...): TBytes;
begin
  // Compiler warns: function result variable of a managed type 
  // does not seem to be initialized
  SetLength(Result, len);
  // ...
end;
```

**Why it happens:** Free Pascal is more strict about uninitialized managed types than Delphi.

## Potential Solutions

### Option 1: Fix FPC Compatibility (High Effort)

**Approach:** Modify `garry.pas` to be compatible with both Delphi and Free Pascal.

**Changes needed:**
1. Replace `TBytes` with `array of Byte` throughout
2. Remove function overloading that FPC doesn't support
3. Add explicit result initialization
4. Use `{$IFDEF FPC}` for compiler-specific code

**Estimated effort:** 8-12 hours

**Pros:**
- Single codebase for both Delphi and FPC
- Better long-term maintainability

**Cons:**
- Significant refactoring required
- May break Delphi compatibility
- Ongoing maintenance burden

### Option 2: Create Separate FPC Version (Medium Effort)

**Approach:** Create a new `garry_fpc.pas` optimized for Free Pascal.

**Changes needed:**
1. Create `garry_fpc.pas` with FPC-specific syntax
2. Use `{$MODE OBJFPC}` instead of `{$MODE DELPHI}`
3. Avoid dynamic arrays, use static arrays or pointers
4. Simplify the OOP interface

**Estimated effort:** 4-8 hours

**Pros:**
- Optimized for FPC
- No risk to Delphi compatibility
- Can use FPC-specific features

**Cons:**
- Two codebases to maintain
- Duplicated logic

### Option 3: Use Dynamic Loading (Already Works)

**Approach:** Keep the minimal test approach, document that OOP wrapper requires Delphi.

**Changes needed:**
1. Document that `garry.pas` requires Delphi, not FPC
2. Provide `test_minimal.lpr` as the FPC example
3. Create helper functions for common operations

**Estimated effort:** 1-2 hours

**Pros:**
- Already works
- Minimal changes needed
- Clear documentation

**Cons:**
- No OOP wrapper for FPC users
- Less ergonomic API

### Option 4: Use C Bindings Directly (Low Effort)

**Approach:** Create a thin wrapper around the C functions without OOP.

**Changes needed:**
1. Create `garry_c.pas` with direct C function bindings
2. Provide helper functions for common operations
3. Document the API

**Estimated effort:** 2-4 hours

**Pros:**
- Simple, works with FPC
- No OOP complexity
- Easy to maintain

**Cons:**
- Less ergonomic than OOP wrapper
- More verbose usage

## Recommendation

**For immediate fix:** Use Option 3 (dynamic loading) - it already works and requires minimal changes.

**For long-term:** Consider Option 2 (separate FPC version) if FPC support is important. This provides a clean FPC-specific API without compromising Delphi compatibility.

## Code Examples

### Current Working Approach (Dynamic Loading)

```pascal
program test_minimal;
{$MODE DELPHI}
uses SysUtils, dynlibs;

var
  LibHandle: TLibHandle;
  p_create: function(Path: PChar): Pointer; cdecl;
  // ... other function pointers
  
begin
  LibHandle := LoadLibrary('libgarry.dylib');
  @p_create := GetProcedureAddress(LibHandle, 'garry_database_create');
  // ... use functions directly
end.
```

### Potential FPC-Optimized Wrapper

```pascal
unit garry_fpc;
{$MODE OBJFPC}
interface

uses dynlibs;

type
  TGarryDB = class
  private
    FHandle: Pointer;
  public
    constructor Create(const Path: string);
    destructor Destroy; override;
    function Get(const Key: string): string;
    procedure SetKeyValue(const Key, Value: string);
    // ... more methods
  end;

implementation

constructor TGarryDB.Create(const Path: string);
begin
  FHandle := garry_database_create(PChar(Path));
end;

// ... etc
```

## Conclusion

The Delphi OOP wrapper has significant Free Pascal compatibility issues due to differences in type systems and language features. The quickest path forward is to document the limitation and provide the dynamic loading approach as the recommended FPC solution.
