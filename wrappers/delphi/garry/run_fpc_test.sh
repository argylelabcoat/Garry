#!/bin/sh
# Compile and run the FPC test for the Garry Delphi/FPC wrapper.
# Exits non-zero on any failure. Requires fpc in PATH and libgarry in build/.
set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
GARRY_ROOT=$(cd "$SCRIPT_DIR/../../.." && pwd)
BUILD_DIR="$GARRY_ROOT/build"

# Ensure libgarry is available
if [ ! -f "$BUILD_DIR/libgarry.dylib" ] && [ ! -f "$BUILD_DIR/libgarry.so" ] && [ ! -f "$BUILD_DIR/libgarry.a" ]; then
  echo "ERROR: libgarry not found in $BUILD_DIR"
  echo "Build the C library first: cd $GARRY_ROOT/build && cmake .. && make"
  exit 1
fi

cd "$SCRIPT_DIR"

# Copy the shared library next to the test binary
if [ -f "$BUILD_DIR/libgarry.dylib" ]; then
  cp "$BUILD_DIR/libgarry.dylib" .
elif [ -f "$BUILD_DIR/libgarry.so" ]; then
  cp "$BUILD_DIR/libgarry.so" .
fi

# Compile all units
echo "=== Compiling garry_types.pas ==="
fpc -Mdelphi garry_types.pas
echo "=== Compiling garry.pas ==="
fpc -Mdelphi garry.pas
echo "=== Compiling garry_codec.pas ==="
fpc -Mdelphi garry_codec.pas
echo "=== Compiling garry_test.lpr ==="
fpc -Mdelphi garry_test.lpr

# Run the test
echo "=== Running tests ==="
./garry_test

# Clean up build artifacts
rm -f *.o *.ppu garry_test test_fpc.db libgarry.dylib libgarry.so 2>/dev/null

echo "=== FPC test PASSED ==="