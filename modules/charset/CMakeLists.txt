# @cp/charset �?CP语言 字符集转换模块（GBK/Big5/Shift-JIS�?cmake_minimum_required(VERSION 3.20)
project(cplang_charset LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_charset STATIC
    charset.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_charset.cpp
)

target_include_directories(cplang_charset PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(cplang_charset PRIVATE /utf-8)
target_compile_definitions(cplang_charset PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_charset PUBLIC cplang_vm)
