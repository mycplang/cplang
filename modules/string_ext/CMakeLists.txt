# @cp/string_ext �?CP语言 字符串高级模块（正则/格式�?搜索/编码扩展�?cmake_minimum_required(VERSION 3.20)
project(cplang_string_ext LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_string_ext STATIC
    string_ext.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_string_ext.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_string_more_impl.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_str_search.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_regex.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_stats_utils.cpp
)

target_include_directories(cplang_string_ext PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(cplang_string_ext PRIVATE /utf-8)
target_compile_definitions(cplang_string_ext PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_string_ext PUBLIC cplang_vm)
