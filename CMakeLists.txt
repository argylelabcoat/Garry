# Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
# Licensed under the BSD-3-Clause License.
# See LICENSE file for details.

cmake_minimum_required(VERSION 3.10)
project(Garry VERSION 0.1.0 LANGUAGES C)

if(WIN32)
    set(GARRY_PLATFORM_DIR platform_src/win32)
else()
    set(GARRY_PLATFORM_DIR platform_src/posix)
endif()

# NOTE: file_io.c and threading.c live in platform_src/{posix,win32}/.
# Never add src/file_io.c or src/garry_threading.c here — they are
# legacy duplicates that would cause duplicate-symbol linker errors.

set(GARRY_SOURCES
    src/lz4.c
    ${GARRY_PLATFORM_DIR}/threading.c
    src/garry_types.c
    src/engine_settings.c
    src/garry_config.c
    src/storage_core.c
    src/keysplit.c
    src/lock_table.c
    ${GARRY_PLATFORM_DIR}/file_io.c
    src/key_encoding.c
    src/slotted_page.c
    src/db_header.c
    src/btree_compression.c
    src/record_codec.c
    src/btree_node.c
    src/btree_search.c
    src/btree_modify.c
    src/btree_cursor.c
    src/buffer_pool.c
    src/version_chain.c
    src/hierarchical.c
    src/wal_record.c
    src/wal_log.c
    src/transaction.c
    src/gc.c
    src/storage_engine.c
    src/storage_txn.c
    src/storage_ops.c
    src/storage_cursor.c
    src/garry_api.c
    src/storage_navigation.c
    src/wal_recovery.c
    src/wal_checkpoint.c
)

# Static library
add_library(garry STATIC ${GARRY_SOURCES})
target_include_directories(garry PUBLIC include src .)
target_compile_options(garry PRIVATE -std=gnu89 -pedantic -Wall -Wextra)

# Shared library (DLL on Windows, .so on Linux, .dylib on macOS)
add_library(garry_shared SHARED ${GARRY_SOURCES})
target_include_directories(garry_shared PUBLIC include src .)
target_compile_options(garry_shared PRIVATE -std=gnu89 -pedantic -Wall -Wextra)
target_compile_definitions(garry_shared PRIVATE GARRY_BUILDING_DLL)
set_target_properties(garry_shared PROPERTIES
    OUTPUT_NAME garry
    VERSION 0.1.0
    SOVERSION 0
    POSITION_INDEPENDENT_CODE ON
)

# Install
install(DIRECTORY include/garry DESTINATION include)
install(TARGETS garry garry_shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Tests
enable_testing()
add_subdirectory(tests)

# Coverage
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(garry PRIVATE --coverage)
    target_link_options(garry PUBLIC --coverage)
endif()

add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --filter ${CMAKE_SOURCE_DIR}/src
    DEPENDS garry
)
