#!/usr/bin/env node

/**
 * Post-install script to copy the native library if available.
 * This allows the package to work without requiring the user to
 * manually copy the library after building.
 */

const fs = require('fs');
const path = require('path');

const platform = process.platform;
const arch = process.arch;

// Determine library name and source path
let libName, srcDir;
if (platform === 'darwin') {
    libName = 'libgarry.dylib';
    srcDir = path.join(__dirname, '..', '..', '..', '..', 'build');
} else if (platform === 'linux') {
    libName = 'libgarry.so';
    srcDir = path.join(__dirname, '..', '..', '..', '..', 'build');
} else if (platform === 'win32') {
    libName = 'garry.dll';
    srcDir = path.join(__dirname, '..', '..', '..', '..', 'build', 'Release');
} else {
    console.log(`Unsupported platform: ${platform}`);
    process.exit(0);
}

const srcPath = path.join(srcDir, libName);
const destDir = path.join(__dirname, '..', 'build', 'Release');
const destPath = path.join(destDir, libName);

// Check if source exists
if (!fs.existsSync(srcPath)) {
    console.log(`Native library not found at ${srcPath}`);
    console.log('You may need to build libgarry first:');
    console.log('  cd ../../../build && make');
    process.exit(0);
}

// Create destination directory if needed
if (!fs.existsSync(destDir)) {
    fs.mkdirSync(destDir, { recursive: true });
}

// Copy the library
try {
    fs.copyFileSync(srcPath, destPath);
    console.log(`Copied ${libName} to ${destPath}`);
} catch (err) {
    console.error(`Failed to copy library: ${err.message}`);
    process.exit(1);
}
