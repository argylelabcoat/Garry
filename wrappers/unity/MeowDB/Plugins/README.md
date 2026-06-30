# MeowDB Native Plugins

Place compiled Garry native libraries in the appropriate platform folder:

## Folder Structure

```
Plugins/
├── x86_64/              # Linux x86_64
│   └── libgarry.so
├── x86_64_macOS/        # macOS x86_64/ARM64
│   └── libgarry.dylib
└── x86_64_windows/      # Windows x86_64
    └── garry.dll
```

## Building the Native Library

From the Garry repository root:

```bash
# Linux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cp libgarry.so wrappers/unity/MeowDB/Plugins/x86_64/

# macOS
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
cp libgarry.dylib wrappers/unity/MeowDB/Plugins/x86_64_macOS/

# Windows (MSVC)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
cp Release/garry.dll wrappers/unity/MeowDB/Plugins/x86_64_windows/
```

## Unity Import Settings

After placing the native libraries, Unity should automatically detect them.
If not, select each `.dll`/`.so`/`.dylib` file in the Project window and set:

- **Platform:** Check only the appropriate platform (e.g., "Editor" + "Standalone")
- **CPU:** x86_64
- **Load Type:** Native Plugin (default)

## iOS (Static Linking)

For iOS, the native library must be statically linked:
1. Build as a static library (`.a`)
2. Add to `Plugins/iOS/`
3. Use `UNITY_IOS` preprocessor directive (already handled in MeowNative.cs)
