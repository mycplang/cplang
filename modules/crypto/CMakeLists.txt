# @cp/crypto �?CP语言 加密模块（AES + SHA + MD5 + Base64�?cmake_minimum_required(VERSION 3.20)
project(cplang_crypto LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_crypto STATIC
    crypto.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_crypto_impl.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_crypto_plus.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_aes.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_encoding.cpp
    ${CMAKE_SOURCE_DIR}/src/crypto/md5_impl.cpp
    ${CMAKE_SOURCE_DIR}/src/miniz.c
    ${CMAKE_SOURCE_DIR}/src/miniz_tdef.c
    ${CMAKE_SOURCE_DIR}/src/miniz_tinfl.c
)

target_include_directories(cplang_crypto PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(cplang_crypto PRIVATE /utf-8)
target_compile_definitions(cplang_crypto PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_crypto PUBLIC cplang_vm)
