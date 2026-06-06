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

set "SRCS=%SCRIPT_DIR%src\main.cpp %SCRIPT_DIR%src\core\verbose.cpp %SCRIPT_DIR%src\lexer\lexer.cpp %SCRIPT_DIR%src\parser\parser.cpp %SCRIPT_DIR%src\parser\parser_decl.cpp %SCRIPT_DIR%src\parser\parser_stmt.cpp %SCRIPT_DIR%src\parser\parser_expr.cpp %SCRIPT_DIR%src\semantic\semantic_analyzer.cpp %SCRIPT_DIR%src\codegen\codegen.cpp %SCRIPT_DIR%src\codegen\codegen_opt.cpp %SCRIPT_DIR%src\codegen\codegen_stmt.cpp %SCRIPT_DIR%src\codegen\codegen_expr.cpp %SCRIPT_DIR%src\codegen\bytecode_optimizer.cpp %SCRIPT_DIR%src\vm\vm.cpp %SCRIPT_DIR%src\vm\vm_containers.cpp %SCRIPT_DIR%src\vm\vm_objects.cpp %SCRIPT_DIR%src\vm\vm_exec.cpp %SCRIPT_DIR%src\vm\value.cpp %SCRIPT_DIR%src\vm\vm_opt_stub.cpp %SCRIPT_DIR%src\repl.cpp %SCRIPT_DIR%src\stdlib\stdlib.cpp %SCRIPT_DIR%src\stdlib\stdlib_fix_missing.cpp %SCRIPT_DIR%src\stdlib\stdlib_stubs.cpp %SCRIPT_DIR%src\stdlib\stdlib_imgui.cpp %SCRIPT_DIR%src\stdlib\stdlib_raylib_unit.cpp %SCRIPT_DIR%src\miniz.c %SCRIPT_DIR%src\miniz_tdef.c %SCRIPT_DIR%src\miniz_tinfl.c %SCRIPT_DIR%src\crypto\md5_impl.cpp %SCRIPT_DIR%src\sqlite\sqlite3.c %SCRIPT_DIR%src\optimizer\optimizer.cpp %SCRIPT_DIR%src\optimizer\constant_folder.cpp %SCRIPT_DIR%src\optimizer\dead_code_eliminator.cpp %SCRIPT_DIR%src\optimizer\function_inliner.cpp %SCRIPT_DIR%src\optimizer\tail_recursion_optimizer.cpp %SCRIPT_DIR%src\optimizer\loop_unroller.cpp %SCRIPT_DIR%src\optimizer\escape_analyzer.cpp %SCRIPT_DIR%src\module\module_system.cpp %SCRIPT_DIR%src\exception\exception_handler.cpp %SCRIPT_DIR%src\debug\debugger.cpp %SCRIPT_DIR%src\jit\hybrid_jit_stub.cpp %SCRIPT_DIR%src\codegen\aot_stub.cpp"

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
