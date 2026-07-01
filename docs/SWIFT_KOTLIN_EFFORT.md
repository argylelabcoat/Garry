# Swift/Kotlin Wrapper Effort Estimate

## Summary

| Wrapper | Effort | Complexity | Priority |
|---------|--------|------------|----------|
| Swift (iOS/macOS) | 40-60 hours | Medium | High for iOS apps |
| Kotlin (Android/JVM) | 40-60 hours | Medium | High for Android apps |

**Total estimated effort:** 80-120 hours for both wrappers

---

## Swift Wrapper (iOS/macOS)

### Architecture

Similar to the Go wrapper but using Swift's native FFI:

```
wrappers/swift/Garry/
├── Package.swift              # SPM package definition
├── Sources/
│   ├── Garry/
│   │   ├── GarryNative.swift  # P/Invoke (Darwin calls)
│   │   ├── Database.swift     # OOP wrapper
│   │   ├── Transaction.swift  # Transaction handling
│   │   ├── Cursor.swift       # Cursor iteration
│   │   ├── KeyEncoder.swift   # Key encoding
│   │   ├── BinaryCodec.swift  # Binary value encoding
│   │   └── Serializer.swift   # Object mapping
│   └── GarryObjC/             # Objective-C bridge (if needed)
└── Tests/
    └── GarryTests/
        ├── DatabaseTests.swift
        ├── SerializerTests.swift
        └── CodecTests.swift
```

### Effort Breakdown

| Task | Hours | Notes |
|------|-------|-------|
| Project setup (SPM) | 2-4 | Package.swift, Xcode project |
| P/Invoke bindings | 8-12 | Darwin `dlopen`/`dlsym` calls |
| OOP wrapper | 8-12 | Database, Transaction, Cursor |
| Key encoding | 4-6 | Same logic as Go/Node.js |
| Binary codec | 6-8 | Swift type system differences |
| Object serializer | 8-12 | Swift Mirror API |
| Tests | 8-12 | Unit + integration tests |
| Documentation | 4-6 | README, examples |

### Key Differences from Go

1. **No CGO**: Swift uses `dlopen`/`dlsym` for dynamic loading (like Delphi)
2. **Memory safety**: Swift has automatic reference counting, no manual `free()`
3. **Type system**: Swift enums with associated values for serialization
4. **Concurrency**: Swift async/await for transaction handling

### Challenges

- **Swift/Objective-C interop**: May need Objective-C bridge for some C API calls
- **Memory management**: Swift's ARC vs C's manual allocation
- **Platform support**: iOS requires static linking or Framework embedding

---

## Kotlin Wrapper (Android/JVM)

### Architecture

Similar to the Go wrapper but using Kotlin/JNI:

```
wrappers/kotlin/garry/
├── build.gradle.kts           # Gradle build file
├── src/
│   ├── main/
│   │   ├── java/com/garry/
│   │   │   ├── GarryNative.kt   # JNI bindings
│   │   │   ├── Database.kt      # OOP wrapper
│   │   │   ├── Transaction.kt   # Transaction handling
│   │   │   ├── Cursor.kt        # Cursor iteration
│   │   │   ├── KeyEncoder.kt    # Key encoding
│   │   │   ├── BinaryCodec.kt   # Binary value encoding
│   │   │   └── Serializer.kt    # Object mapping
│   │   └── cpp/
│   │       └── garry_jni.cpp    # JNI bridge
│   └── test/
│       └── java/com/garry/
│           ├── DatabaseTests.kt
│           ├── SerializerTests.kt
│           └── CodecTests.kt
└── gradle/
    └── wrapper/
```

### Effort Breakdown

| Task | Hours | Notes |
|------|-------|-------|
| Project setup (Gradle) | 2-4 | build.gradle.kts, CMake |
| JNI bridge | 8-12 | C++ ↔ Kotlin interop |
| OOP wrapper | 8-12 | Database, Transaction, Cursor |
| Key encoding | 4-6 | Same logic as other wrappers |
| Binary codec | 6-8 | Kotlin type system |
| Object serializer | 8-12 | Kotlin reflection |
| Tests | 8-12 | JUnit + integration tests |
| Documentation | 4-6 | README, examples |

### Key Differences from Go

1. **JNI bridge**: Requires C++ code to bridge Kotlin ↔ C
2. **Memory management**: JVM garbage collection vs C manual allocation
3. **Type system**: Kotlin data classes for serialization
4. **Concurrency**: Kotlin coroutines for transaction handling

### Challenges

- **JNI complexity**: C++ bridge code adds maintenance burden
- **Android NDK**: Native library loading on Android
- **JVM memory**: JNI global references and garbage collection

---

## Comparison with Existing Wrappers

| Aspect | Go | Swift | Kotlin |
|--------|-----|-------|--------|
| FFI approach | CGO | dlopen/dlsym | JNI |
| Memory management | Manual | ARC | GC |
| Type system | Structural | Nominal | Nominal |
| Concurrency | Goroutines | async/await | Coroutines |
| Build system | Go modules | SPM | Gradle |
| Platform | Cross-platform | Apple ecosystem | Android/JVM |

---

## Recommendation

**For iOS apps:** Swift wrapper is essential. Consider using the C library directly via Objective-C bridge if Swift wrapper is too complex.

**For Android apps:** Kotlin wrapper is essential. Consider using the C library directly via JNI if Kotlin wrapper is too complex.

**Priority:** Both wrappers are high priority for mobile app support, but can be deferred if web/desktop are the primary targets.
