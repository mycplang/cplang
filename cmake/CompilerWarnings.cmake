# CompilerWarnings.cmake
# 统一编译器警告配置，所有 target 通过 set_target_warnings() 调用

function(set_target_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4           # 警告级别 4
            /permissive-  # 标准一致性
            /utf-8        # UTF-8 源文件编码
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wshadow
            -Wunused
            -Wimplicit-fallthrough
            -Wno-unknown-pragmas        # 允许未知 pragma (用于 IDE)
        )
    endif()

    if(CPLANG_WARNINGS_AS_ERRORS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /WX)
        else()
            target_compile_options(${target} PRIVATE -Werror)
        endif()
        message(STATUS "  [${target}] 警告即错误已启用")
    endif()
endfunction()
