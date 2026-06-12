# @cp/ffi �?CP语言 外部函数接口模块（DLL 调用�?cmake_minimum_required(VERSION 3.20)
project(cplang_ffi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_ffi STATIC
    ffi.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_ffi.cpp
)

target_include_directories(cplang_ffi PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(cplang_ffi PRIVATE /utf-8)
target_compile_definitions(cplang_ffi PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_ffi PUBLIC cplang_vm)
