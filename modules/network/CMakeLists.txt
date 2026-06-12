# @cp/net — CP语言 网络模块（HTTP + JSON）
cmake_minimum_required(VERSION 3.20)
project(cplang_net LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_net STATIC
    network.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_json_http.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_http.cpp
)

target_include_directories(cplang_net PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(cplang_net PRIVATE /utf-8)
target_compile_definitions(cplang_net PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_net PUBLIC cplang_vm cplang_platform)
