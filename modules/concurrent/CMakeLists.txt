# @cp/concurrent �?CP语言 并发模块（线�?互斥/Channel/Future/RWLock�?cmake_minimum_required(VERSION 3.20)
project(cplang_concurrent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_concurrent STATIC
    concurrent.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_threading.cpp
)

target_include_directories(cplang_concurrent PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(cplang_concurrent PRIVATE /utf-8)
target_compile_definitions(cplang_concurrent PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_concurrent PUBLIC cplang_vm)
