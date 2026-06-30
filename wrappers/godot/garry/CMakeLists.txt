cmake_minimum_required(VERSION 3.21)
project(garry_gdext C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(GDEXT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(GARRY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../include")

add_library(garry_gdext SHARED
    src/gdext.c
    src/garry_db.c
)

set_target_properties(garry_gdext PROPERTIES PREFIX "lib")
set_target_properties(garry_gdext PROPERTIES OUTPUT_NAME "garry")

target_include_directories(garry_gdext PRIVATE
    src
    ${GDEXT_INCLUDE_DIR}
    ${GARRY_INCLUDE_DIR}
)

target_compile_options(garry_gdext PRIVATE
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
    -Wno-unused-function
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libgarry.a")
    target_link_libraries(garry_gdext PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libgarry.a")
else()
    target_link_libraries(garry_gdext PRIVATE garry)
endif()

set_target_properties(garry_gdext PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/project/addons/garry/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/project/addons/garry/bin"
)
