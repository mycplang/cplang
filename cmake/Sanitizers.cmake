# Sanitizers.cmake
# 仅在 Debug 模式下使用

if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    message(STATUS "Sanitizers: 仅 Debug 模式, 跳过")
    return()
endif()

set(SANITIZER_FLAGS "")

if(MSVC)
    # MSVC 自带 AddressSanitizer (/fsanitize=address)
    option(CPLANG_SANITIZE_ADDRESS "启用 AddressSanitizer" ON)
    if(CPLANG_SANITIZE_ADDRESS)
        set(SANITIZER_FLAGS "${SANITIZER_FLAGS} /fsanitize=address")
    endif()
else()
    option(CPLANG_SANITIZE_ADDRESS  "启用 AddressSanitizer"  ON)
    option(CPLANG_SANITIZE_UNDEFINED "启用 UndefinedBehaviorSanitizer" ON)

    if(CPLANG_SANITIZE_ADDRESS)
        set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=address")
    endif()
    if(CPLANG_SANITIZE_UNDEFINED)
        set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=undefined")
    endif()
endif()

if(SANITIZER_FLAGS)
    add_compile_options(${SANITIZER_FLAGS})
    add_link_options(${SANITIZER_FLAGS})
    message(STATUS "Sanitizers: ${SANITIZER_FLAGS}")
endif()
