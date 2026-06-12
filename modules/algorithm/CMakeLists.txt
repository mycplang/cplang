# @cp/algorithm �?CP语言 算法模块（排�?查找/分区/随机/位运算）
cmake_minimum_required(VERSION 3.20)
project(cplang_algorithm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_algorithm STATIC
    algorithm.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_algo_bitwise.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_algo_ext.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_algo_missing.cpp
)

target_include_directories(cplang_algorithm PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(cplang_algorithm PRIVATE /utf-8)
target_compile_definitions(cplang_algorithm PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_algorithm PUBLIC cplang_vm)
