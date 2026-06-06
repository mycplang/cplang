@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

set "SCRIPT_DIR=%~dp0"
set "LLVM_DIR=%SCRIPT_DIR%llvm-dev"
set "CLANG=%LLVM_DIR%\bin\clang-cl.exe"
set "LINKER=%LLVM_DIR%\bin\lld-link.exe"

if not exist "!CLANG!" (
    echo [ERROR] clang-cl.exe not found at !CLANG!
    echo Please ensure llvm-dev/ is present.
    endlocal
    exit /b 1
)

echo [Building CP Language with Clang-CL...]
echo [Clang: !CLANG!]

set CL_OPTS=/utf-8 /std:c++17 /EHsc /O1 /W3 /wd4244
set CL_INC=/I"%SCRIPT_DIR%include" /I"%SCRIPT_DIR%third_party\raylib\src" /I"%SCRIPT_DIR%third_party\imgui"
set CL_DEF=/D_CRT_SECURE_NO_WARNINGS /DNDEBUG /DMINIZ_NO_ARCHIVE_APIS /DNO_FONT_AWESOME

set "STDLIB_SRCS=%SCRIPT_DIR%src\stdlib\stdlib.cpp %SCRIPT_DIR%src\stdlib\stdlib_aes.cpp %SCRIPT_DIR%src\stdlib\stdlib_algo_bitwise.cpp %SCRIPT_DIR%src\stdlib\stdlib_algo_ext.cpp %SCRIPT_DIR%src\stdlib\stdlib_algo_missing.cpp %SCRIPT_DIR%src\stdlib\stdlib_array.cpp %SCRIPT_DIR%src\stdlib\stdlib_array_file_more.cpp %SCRIPT_DIR%src\stdlib\stdlib_bitset.cpp %SCRIPT_DIR%src\stdlib\stdlib_charset.cpp %SCRIPT_DIR%src\stdlib\stdlib_complex_pair.cpp %SCRIPT_DIR%src\stdlib\stdlib_containers.cpp %SCRIPT_DIR%src\stdlib\stdlib_crypto_impl.cpp %SCRIPT_DIR%src\stdlib\stdlib_crypto_plus.cpp %SCRIPT_DIR%src\stdlib\stdlib_db.cpp %SCRIPT_DIR%src\stdlib\stdlib_encoding.cpp %SCRIPT_DIR%src\stdlib\stdlib_ffi.cpp %SCRIPT_DIR%src\stdlib\stdlib_file.cpp %SCRIPT_DIR%src\stdlib\stdlib_file_log.cpp %SCRIPT_DIR%src\stdlib\stdlib_heap.cpp %SCRIPT_DIR%src\stdlib\stdlib_http.cpp %SCRIPT_DIR%src\stdlib\stdlib_io.cpp %SCRIPT_DIR%src\stdlib\stdlib_iterator.cpp %SCRIPT_DIR%src\stdlib\stdlib_json_http.cpp %SCRIPT_DIR%src\stdlib\stdlib_map.cpp %SCRIPT_DIR%src\stdlib\stdlib_math.cpp %SCRIPT_DIR%src\stdlib\stdlib_math_special.cpp %SCRIPT_DIR%src\stdlib\stdlib_matrix_color_path_console.cpp %SCRIPT_DIR%src\stdlib\stdlib_misc_modules.cpp %SCRIPT_DIR%src\stdlib\stdlib_net_ws_sql.cpp %SCRIPT_DIR%src\stdlib\stdlib_numeric_limits.cpp %SCRIPT_DIR%src\stdlib\stdlib_p1_enhance.cpp %SCRIPT_DIR%src\stdlib\stdlib_p2_more.cpp %SCRIPT_DIR%src\stdlib\stdlib_p3_util.cpp %SCRIPT_DIR%src\stdlib\stdlib_r10_r11.cpp %SCRIPT_DIR%src\stdlib\stdlib_raylib.cpp %SCRIPT_DIR%src\stdlib\stdlib_redis.cpp %SCRIPT_DIR%src\stdlib\stdlib_reflect.cpp %SCRIPT_DIR%src\stdlib\stdlib_regex.cpp %SCRIPT_DIR%src\stdlib\stdlib_stats_utils.cpp %SCRIPT_DIR%src\stdlib\stdlib_str_search.cpp %SCRIPT_DIR%src\stdlib\stdlib_string.cpp %SCRIPT_DIR%src\stdlib\stdlib_string_ext.cpp %SCRIPT_DIR%src\stdlib\stdlib_string_more_impl.cpp %SCRIPT_DIR%src\stdlib\stdlib_table.cpp %SCRIPT_DIR%src\stdlib\stdlib_threading.cpp %SCRIPT_DIR%src\stdlib\stdlib_time_sys_more.cpp %SCRIPT_DIR%src\stdlib\stdlib_time_system.cpp %SCRIPT_DIR%src\stdlib\stdlib_types_net.cpp %SCRIPT_DIR%src\stdlib\stdlib_variant_utils.cpp %SCRIPT_DIR%src\stdlib\stdlib_fix_missing.cpp %SCRIPT_DIR%src\stdlib\stdlib_stubs.cpp %SCRIPT_DIR%src\stdlib\stdlib_imgui.cpp"

set "SRCS=%SCRIPT_DIR%src\main.cpp %SCRIPT_DIR%src\core\verbose.cpp %SCRIPT_DIR%src\lexer\lexer.cpp %SCRIPT_DIR%src\parser\parser.cpp %SCRIPT_DIR%src\parser\parser_decl.cpp %SCRIPT_DIR%src\parser\parser_stmt.cpp %SCRIPT_DIR%src\parser\parser_expr.cpp %SCRIPT_DIR%src\semantic\semantic_analyzer.cpp %SCRIPT_DIR%src\codegen\codegen.cpp %SCRIPT_DIR%src\codegen\codegen_opt.cpp %SCRIPT_DIR%src\codegen\codegen_stmt.cpp %SCRIPT_DIR%src\codegen\codegen_expr.cpp %SCRIPT_DIR%src\codegen\bytecode_optimizer.cpp %SCRIPT_DIR%src\vm\vm.cpp %SCRIPT_DIR%src\vm\vm_containers.cpp %SCRIPT_DIR%src\vm\vm_objects.cpp %SCRIPT_DIR%src\vm\vm_exec.cpp %SCRIPT_DIR%src\vm\value.cpp %SCRIPT_DIR%src\vm\vm_opt_stub.cpp %SCRIPT_DIR%src\repl.cpp !STDLIB_SRCS! %SCRIPT_DIR%src\miniz.c %SCRIPT_DIR%src\miniz_tdef.c %SCRIPT_DIR%src\miniz_tinfl.c %SCRIPT_DIR%src\crypto\md5_impl.cpp %SCRIPT_DIR%src\sqlite\sqlite3.c %SCRIPT_DIR%src\optimizer\optimizer.cpp %SCRIPT_DIR%src\optimizer\constant_folder.cpp %SCRIPT_DIR%src\optimizer\dead_code_eliminator.cpp %SCRIPT_DIR%src\optimizer\function_inliner.cpp %SCRIPT_DIR%src\optimizer\tail_recursion_optimizer.cpp %SCRIPT_DIR%src\optimizer\loop_unroller.cpp %SCRIPT_DIR%src\optimizer\escape_analyzer.cpp %SCRIPT_DIR%src\module\module_system.cpp %SCRIPT_DIR%src\exception\exception_handler.cpp %SCRIPT_DIR%src\debug\debugger.cpp %SCRIPT_DIR%src\jit\hybrid_jit_stub.cpp %SCRIPT_DIR%src\codegen\aot_stub.cpp"

echo [Compiling (bytecode-only mode)...]
echo.

md build_clang 2>nul

!CLANG! !CL_OPTS! !CL_INC! !CL_DEF! !SRCS! /Fe:%SCRIPT_DIR%build_clang\cplang.exe /link /LIBPATH:%LLVM_DIR%\lib

if %ERRORLEVEL% NEQ 0 (
    echo [Build Failed (clang-cl)]
    endlocal
    exit /b 1
)

echo.
echo [Build Success!]
echo [Output: %SCRIPT_DIR%build_clang\cplang.exe]

endlocal
