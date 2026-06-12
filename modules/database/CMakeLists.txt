# @cp/database �?CP语言 数据库模块（SQLite + MySQL + Redis�?cmake_minimum_required(VERSION 3.20)
project(cplang_database LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_database STATIC
    database.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_db.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_redis.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_net_ws_sql.cpp
    ${CMAKE_SOURCE_DIR}/src/sqlite/sqlite3.c
)

target_include_directories(cplang_database PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(cplang_database PRIVATE /utf-8)
target_compile_definitions(cplang_database PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_database PUBLIC cplang_vm cplang_platform)
