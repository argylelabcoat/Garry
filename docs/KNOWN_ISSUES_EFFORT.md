# Known Issues - Effort Analysis

## Summary

| Issue | Effort | Priority | Impact |
|-------|--------|----------|--------|
| test_blog_storage SIGTRAP | 2-4 hours | Low | 1 test failing |
| Go transaction isolation | 4-8 hours | Medium | Transaction semantics |
| Godot open() returns false | 8-16 hours | High | Wrapper unusable |
| Delphi OOP wrapper | 4-8 hours | Medium | OOP layer broken |
| Node.js library build | 1-2 hours | Low | Expected behavior |
| Test coverage gaps | 8-16 hours | Low | Missing edge cases |
| CMake warnings | 1-2 hours | Low | Cosmetic |

**Total estimated effort:** 28-52 hours

---

## Detailed Analysis

### 1. test_blog_storage SIGTRAP
**Effort:** 2-4 hours  
**Priority:** Low  
**Impact:** 1 test failing (42/43 pass)

**Root cause investigation:**
- Test reads markdown files from `tests/content/`
- Crash occurs during file parsing or storage
- Likely issue: file path handling or content size exceeding internal limits

**Fix approach:**
1. Add debug output to identify exact crash location
2. Check if file paths are relative to correct directory
3. Verify content size doesn't exceed `GARRY_MAX_RECORD_SIZE` (16384 bytes)
4. May need to adjust test to use smaller content or fix path handling

**Risk:** Low - isolated test issue, not core library

---

### 2. Go Transaction Isolation
**Effort:** 4-8 hours  
**Priority:** Medium  
**Impact:** Transaction semantics differ from expected

**Root cause investigation:**
- C library uses some form of transaction isolation
- Go test expects rolled-back data to be invisible
- May be snapshot isolation vs read-committed

**Fix approach:**
1. Document the actual C library transaction semantics
2. Update Go test to match actual behavior
3. Add documentation explaining the isolation model
4. Consider if this is a C library bug or expected behavior

**Risk:** Medium - affects transaction-dependent code

---

### 3. Godot open() Returns False
**Effort:** 8-16 hours  
**Priority:** High  
**Impact:** Wrapper unusable

**Root cause investigation:**
- GDExtension loads correctly (class found, instance created)
- `garry_database_create` likely failing
- Possible causes:
  - Function pointer resolution issues
  - Memory alignment between GDExtension and C
  - Missing initialization steps

**Fix approach:**
1. Add debug output to GDExtension initialization
2. Verify all function pointers are correctly resolved
3. Check memory layout matches C library expectations
4. Test with minimal GDExtension that just calls C functions
5. May need to adjust GDExtension binding code

**Risk:** High - complex integration, multiple failure points

---

### 4. Delphi OOP Wrapper
**Effort:** 4-8 hours  
**Priority:** Medium  
**Impact:** OOP layer doesn't compile

**Root cause investigation:**
- Free Pascal 3.2.2 DELPHI mode incompatibilities
- `TBytes` type handling differs
- Function signatures don't match

**Fix approach:**
1. Option A: Fix FPC compatibility (complex, many changes)
2. Option B: Create separate FPC-optimized version (medium effort)
3. Option C: Use dynamic loading approach (already works)
4. Document that OOP wrapper requires Delphi, not FPC

**Risk:** Medium - FPC compatibility is inherently challenging

---

### 5. Node.js Library Build
**Effort:** 1-2 hours  
**Priority:** Low  
**Impact:** Expected behavior

**Root cause:** Native addons require the library to be built first

**Fix approach:**
1. Add build script to package.json
2. Auto-copy library on npm install
3. Document build requirements in README

**Risk:** Low - straightforward automation

---

### 6. Test Coverage Gaps
**Effort:** 8-16 hours  
**Priority:** Low  
**Impact:** Missing edge case tests

**Areas needing tests:**
- Go: ForEach with complex patterns
- Rust: Advanced serialization edge cases
- Node.js: Error handling in native addon
- Delphi: OOP wrapper (blocked by compilation)

**Fix approach:**
1. Prioritize tests based on actual usage
2. Add tests for critical paths first
3. Skip Delphi OOP tests until compilation fixed

**Risk:** Low - core functionality is tested

---

### 7. CMake Warnings
**Effort:** 1-2 hours  
**Priority:** Low  
**Impact:** Cosmetic only

**Issues:**
- Deprecated CMake options
- Missing ICU library search path

**Fix approach:**
1. Update CMakeLists.txt to use modern CMake
2. Add ICU library detection
3. Suppress non-critical warnings

**Risk:** Low - no functional impact

---

## Recommended Priority Order

1. **Godot open()** (High impact, complex)
2. **Go transaction isolation** (Medium impact, semantic issue)
3. **Delphi OOP wrapper** (Medium impact, compilation issue)
4. **test_blog_storage** (Low impact, isolated)
5. **Node.js build automation** (Low impact, expected)
6. **Test coverage** (Low impact, incremental)
7. **CMake warnings** (Cosmetic)

## Quick Wins (< 4 hours total)

1. Node.js build automation: 1-2 hours
2. CMake warnings: 1-2 hours
3. Document test_blog_storage as known limitation: 0.5 hours

**Total quick wins:** 2.5-4.5 hours
