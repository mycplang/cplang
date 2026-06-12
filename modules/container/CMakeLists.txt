# @cp/container �?CP语言 高级容器模块（栈/队列/链表/集合/映射/�?位集�?cmake_minimum_required(VERSION 3.20)
project(cplang_container LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_container STATIC
    container.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_containers.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_bitset.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_heap.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_complex_pair.cpp
)

target_include_directories(cplang_container PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(cplang_container PRIVATE /utf-8)
target_compile_definitions(cplang_container PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_container PUBLIC cplang_vm)
