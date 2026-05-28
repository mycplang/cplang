@echo off
setlocal enabledelayedexpansion

REM 清理残留的 cplang.exe / a.out 进程
for /f "tokens=2" %%I in ('tasklist /NH /FI "IMAGENAME eq cplang.exe" 2^>nul ^| find "cplang.exe"') do (
    taskkill /F /PID %%I >nul 2>&1 && echo [Cleanup] 已终止僵尸进程 cplang.exe (PID %%I)
)

set "SCRIPT_DIR=%~dp0"
set "LLVM_DIR=%SCRIPT_DIR%llvm-dev"
set "VSROOT="
if not "%VSCMD_ARG_TGT_ARCH%"=="" goto :found_vs
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VSROOT=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VSROOT=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community"
if exist "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VSROOT=D:\Program Files\Microsoft Visual Studio\2022\Community"
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VSROOT=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community"
if exist "%ProgramFiles%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VSROOT=%ProgramFiles%\Microsoft Visual Studio\2019\Community"
if not defined VSROOT (
    echo [ERROR] Visual Studio 2019/2022 not found. Install VS or run from a Developer Command Prompt.
    endlocal
    exit /b 1
)
:found_vs
call "!VSROOT!\VC\Auxiliary\Build\vcvarsall.bat" x64
REM 修复 mspdbcore.dll 路径问题（某些 VS 安装缺少 PATH 项）
if exist "!VSROOT!\Common7\IDE\mspdbcore.dll" set "PATH=!VSROOT!\Common7\IDE;%PATH%"
if exist "!VSROOT!\Common7\IDE\Remote Debugger\x64\mspdbcore.dll" set "PATH=!VSROOT!\Common7\IDE\Remote Debugger\x64;%PATH%"
REM LLVM disabled due to memory limits - set HAS_LLVM=0
set HAS_LLVM=0
goto :compile
REM LLVM sections skipped





:no_llvm
set HAS_LLVM=0
:compile
set CL_OPTS=/utf-8 /std:c++17 /EHsc /W3 /O1 /MD /wd4244 /Zm600
set CL_INC=/I"include" /I"third_party\raylib\src" /I"third_party\raylib\src\external\glfw\include" /I"third_party\imgui"
set CL_DEF=/D_CRT_SECURE_NO_WARNINGS /DNDEBUG /DMINIZ_NO_ARCHIVE_APIS /DGRAPHICS_API_OPENGL_33 /DPLATFORM_DESKTOP /DNO_FONT_AWESOME
if "!HAS_LLVM!"=="1" (
    set CL_INC=!CL_INC! /I"!LLVM_INC!"
    set CL_DEF=!CL_DEF! /DCPLANG_HAS_LLVM
)
set CL_FLAGS=!CL_OPTS! /Fo:build\ !CL_INC! !CL_DEF!
if "!HAS_LLVM!"=="1" set "CL_FLAGS=!CL_FLAGS! /external:W0 /external:I"!LLVM_INC!""
set "SRCS=src\main.cpp src\core\verbose.cpp src\lexer\lexer.cpp src\parser\parser.cpp src\parser\parser_decl.cpp src\parser\parser_stmt.cpp src\parser\parser_expr.cpp src\semantic\semantic_analyzer.cpp src\codegen\codegen.cpp src\codegen\codegen_opt.cpp src\codegen\codegen_stmt.cpp src\codegen\codegen_expr.cpp src\codegen\bytecode_optimizer.cpp src\vm\vm.cpp src\vm\vm_containers.cpp src\vm\vm_objects.cpp src\vm\vm_exec.cpp src\vm\value.cpp src\vm\vm_opt_stub.cpp src\repl.cpp src\stdlib\stdlib.cpp src\stdlib\stdlib_fix_missing.cpp src\stdlib\stdlib_stubs.cpp src\stdlib\stdlib_imgui.cpp src\stdlib\stdlib_raylib_unit.cpp src\miniz.c src\miniz_tdef.c src\miniz_tinfl.c src\crypto\md5_impl.cpp src\sqlite\sqlite3.c src\optimizer\optimizer.cpp src\optimizer\constant_folder.cpp src\optimizer\dead_code_eliminator.cpp src\optimizer\function_inliner.cpp src\optimizer\tail_recursion_optimizer.cpp src\optimizer\loop_unroller.cpp src\optimizer\escape_analyzer.cpp src\module\module_system.cpp src\exception\exception_handler.cpp src\debug\debugger.cpp src\jit\hybrid_jit_stub.cpp src\codegen\aot_stub.cpp"
if "!HAS_LLVM!"=="1" set "SRCS=!SRCS! src\codegen\aot_compiler.cpp src\codegen\llvm_codegen.cpp src\optimizer\llvm_optimizer.cpp src\jit\jit_compiler.cpp src\jit\jit_runtime.cpp src\jit\orc_jit.cpp src\jit\hybrid_jit.cpp"
set "SRCS=!SRCS! third_party\imgui\imgui.cpp third_party\imgui\imgui_draw.cpp third_party\imgui\imgui_tables.cpp third_party\imgui\imgui_widgets.cpp third_party\imgui\rlImGui.cpp"
set SYS_LIBS=Shell32.lib Winhttp.lib Ws2_32.lib Cabinet.lib opengl32.lib gdi32.lib winmm.lib ole32.lib comctl32.lib user32.lib urlmon.lib
set RAYLIB_LIB=third_party\raylib\build_release\raylib\Release\raylib.lib
set LINK_FLAGS=/FORCE:MULTIPLE /ignore:4006 /ignore:4088
if "!HAS_LLVM!"=="1" set "LINK_FLAGS=!LINK_FLAGS! /LIBPATH:!LLVM_LIBDIR!"
echo [Cleaning...]
if exist *.obj del /Q *.obj 2>nul
if exist build\*.obj del /Q build\*.obj 2>nul
echo [Building CP Language...]
if "!HAS_LLVM!"=="1" ( echo [Mode: JIT enabled] ) else ( echo [Mode: Bytecode only] )
cl !CL_FLAGS! !SRCS! /Fe:build\cplang.exe /link !SYS_LIBS! !RAYLIB_LIB! !LINK_FLAGS! !LLVM_LIBS!
if %ERRORLEVEL% NEQ 0 (
    echo [Build Failed]
    endlocal
    exit /b 1
)
echo [Build Success!]
if "!HAS_LLVM!"=="1" echo [JIT engine: LLVM enabled]

REM 为 AOT 链接器编译独立的 jit_runtime
if "!HAS_LLVM!"=="1" (
    echo [Building jit_runtime standalone for AOT linker...]
    cl /c /EHsc /std:c++17 /O2 /nologo /utf-8 /I"C:\cplang\include" "C:\cplang\src\jit\jit_runtime_standalone.cpp" /Fo:build\jit_runtime_standalone.obj
    if exist build\jit_runtime_standalone.obj (
        lib /OUT:build\jit_runtime.lib build\jit_runtime_standalone.obj 2>nul
        echo [Created build\jit_runtime.lib]
    )
)

echo [Cleaning intermediates...]
if exist build\*.obj del /Q build\*.obj 2>nul
if exist build\*.exp del /Q build\*.exp 2>nul
endlocal
exit /b 0
