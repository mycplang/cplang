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
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VSROOT=C:\Program Files (x86)\Microsoft Visual Studio\2022\Community"
if exist "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" set "VSROOT=D:\Program Files\Microsoft Visual Studio\2022\Community"
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
REM LLVM enabled for JIT and AOT
set HAS_LLVM=0
REM Clear LLVM libs when disabled
set "LLVM_LIBS="
if "!HAS_LLVM!"=="1" (
    set "LLVM_INC=%SCRIPT_DIR%llvm-dev\include"
    set "LLVM_LIBDIR=%SCRIPT_DIR%llvm-dev\lib"
    set "LLVM_LIBS=LLVMDemangle.lib LLVMSupport.lib LLVMTableGen.lib LLVMTableGenGlobalISel.lib LLVMTableGenCommon.lib LLVMCore.lib LLVMFuzzerCLI.lib LLVMIRReader.lib LLVMCodeGenTypes.lib LLVMCodeGen.lib LLVMSelectionDAG.lib LLVMAsmPrinter.lib LLVMMIRParser.lib LLVMGlobalISel.lib LLVMDebugInfoDWARF.lib LLVMDebugInfoGSYM.lib LLVMDebugInfoCodeView.lib LLVMDebugInfoMSF.lib LLVMDebugInfoPDB.lib LLVMDebugInfoBTF.lib LLVMDWARFLinker.lib LLVMDWARFLinkerClassic.lib LLVMDWARFLinkerParallel.lib LLVMDWP.lib LLVMAggressiveInstCombine.lib LLVMInstCombine.lib LLVMScalarOpts.lib LLVMipo.lib LLVMVectorize.lib LLVMInstrumentation.lib LLVMCFGuard.lib LLVMCFIVerify.lib LLVMLinker.lib LLVMAnalysis.lib LLVMTransformUtils.lib LLVMTarget.lib LLVMPasses.lib LLVMMC.lib LLVMMCParser.lib LLVMMCDisassembler.lib LLVMMCA.lib LLVMObject.lib LLVMObjectYAML.lib LLVMBinaryFormat.lib LLVMBitReader.lib LLVMBitWriter.lib LLVMBitstreamReader.lib LLVMOption.lib LLVMDiff.lib LLVMProfileData.lib LLVMCoverage.lib LLVMDebuginfod.lib LLVMTextAPI.lib LLVMTextAPIBinaryReader.lib LLVMRemarks.lib LLVMSymbolize.lib LLVMExecutionEngine.lib LLVMRuntimeDyld.lib LLVMJITLink.lib LLVMOrcJIT.lib LLVMOrcDebugging.lib LLVMOrcShared.lib LLVMOrcTargetProcess.lib LLVMTargetParser.lib LLVMLineEditor.lib LLVMFrontendOpenACC.lib LLVMFrontendOpenMP.lib LLVMFrontendHLSL.lib LLVMFrontendDriver.lib LLVMFrontendOffloading.lib LLVMExtensions.lib LLVMWindowsDriver.lib LLVMWindowsManifest.lib LLVMXRay.lib LLVMAsmParser.lib LLVMObjCARCOpts.lib LLVMCoroutines.lib LLVMFuzzMutate.lib LLVMFileCheck.lib LLVMInterfaceStub.lib LLVMObjCopy.lib LLVMDlltoolDriver.lib LLVMLibDriver.lib LLVMCoverage.lib LLVMTargetParser.lib LLVMTextAPI.lib LLVMDemangle.lib LLVMSupport.lib LLVMTableGen.lib LLVMTableGenGlobalISel.lib LLVMTableGenCommon.lib LLVMCore.lib LLVMFuzzerCLI.lib LLVMIRReader.lib LLVMCodeGenTypes.lib LLVMCodeGen.lib LLVMSelectionDAG.lib LLVMAsmPrinter.lib LLVMMIRParser.lib LLVMGlobalISel.lib LLVMDebugInfoDWARF.lib LLVMDebugInfoGSYM.lib LLVMDebugInfoCodeView.lib LLVMDebugInfoMSF.lib LLVMDebugInfoPDB.lib LLVMDebugInfoBTF.lib LLVMDWARFLinker.lib LLVMDWARFLinkerClassic.lib LLVMDWARFLinkerParallel.lib LLVMDWP.lib LLVMAggressiveInstCombine.lib LLVMInstCombine.lib LLVMScalarOpts.lib LLVMipo.lib LLVMVectorize.lib LLVMInstrumentation.lib LLVMCFGuard.lib LLVMCFIVerify.lib LLVMLinker.lib LLVMAnalysis.lib LLVMTransformUtils.lib LLVMTarget.lib LLVMPasses.lib LLVMMC.lib LLVMMCParser.lib LLVMMCDisassembler.lib LLVMMCA.lib LLVMObject.lib LLVMObjectYAML.lib LLVMBinaryFormat.lib LLVMBitReader.lib LLVMBitWriter.lib LLVMBitstreamReader.lib LLVMOption.lib LLVMDiff.lib LLVMProfileData.lib LLVMCoverage.lib LLVMDebuginfod.lib LLVMTextAPI.lib LLVMTextAPIBinaryReader.lib LLVMRemarks.lib LLVMSymbolize.lib LLVMExecutionEngine.lib LLVMRuntimeDyld.lib LLVMJITLink.lib LLVMOrcJIT.lib LLVMOrcDebugging.lib LLVMOrcShared.lib LLVMOrcTargetProcess.lib LLVMTargetParser.lib LLVMLineEditor.lib LLVMFrontendOpenACC.lib LLVMFrontendOpenMP.lib LLVMFrontendHLSL.lib LLVMFrontendDriver.lib LLVMFrontendOffloading.lib LLVMExtensions.lib LLVMWindowsDriver.lib LLVMWindowsManifest.lib LLVMXRay.lib LLVMAsmParser.lib LLVMObjCARCOpts.lib LLVMCoroutines.lib LLVMFuzzMutate.lib LLVMFileCheck.lib LLVMInterfaceStub.lib LLVMObjCopy.lib LLVMDlltoolDriver.lib LLVMLibDriver.lib LLVMCoverage.lib LLVMTargetParser.lib LLVMTextAPI.lib LLVMHipStdPar.lib LLVMIRPrinter.lib LLVMHipStdPar.lib LLVMIRPrinter.lib LLVMX86Info.lib LLVMX86Desc.lib LLVMX86CodeGen.lib LLVMX86AsmParser.lib LLVMX86Disassembler.lib LLVMX86TargetMCA.lib LLVMX86Info.lib LLVMX86Desc.lib LLVMX86CodeGen.lib LLVMX86AsmParser.lib LLVMX86Disassembler.lib LLVMX86TargetMCA.lib"
)





:compile
set CL_OPTS=/utf-8 /std:c++17 /EHsc /W3 /O1 /MD /wd4244 /Zm600
set CL_INC=/I"include" /I"third_party\raylib\src" /I"third_party\raylib\src\external\glfw\include" /I"third_party\imgui"
set CL_DEF=/D_CRT_SECURE_NO_WARNINGS /DNDEBUG /D_ALLOW_RUNTIME_LIBRARY_MISMATCH /DMINIZ_NO_ARCHIVE_APIS /DGRAPHICS_API_OPENGL_21 /DPLATFORM_DESKTOP /DNO_FONT_AWESOME
if "!HAS_LLVM!"=="1" (
    set CL_INC=!CL_INC! /I"!LLVM_INC!"
    set CL_DEF=!CL_DEF! /DCPLANG_HAS_LLVM
)
set CL_FLAGS=!CL_OPTS! /Fo:build\ !CL_INC! !CL_DEF!
if "!HAS_LLVM!"=="1" set "CL_FLAGS=!CL_FLAGS! /external:W0 /external:I"!LLVM_INC!""
set "SRCS=src\main.cpp src\core\verbose.cpp src\lexer\lexer.cpp src\parser\parser.cpp src\parser\parser_decl.cpp src\parser\parser_stmt.cpp src\parser\parser_expr.cpp src\semantic\semantic_analyzer.cpp src\codegen\codegen.cpp src\codegen\codegen_opt.cpp src\codegen\codegen_stmt.cpp src\codegen\codegen_expr.cpp src\codegen\bytecode_optimizer.cpp src\vm\vm.cpp src\vm\vm_containers.cpp src\vm\vm_objects.cpp src\vm\vm_exec.cpp src\vm\value.cpp src\jit\jit_dispatch.cpp src\vm\vm_opt_stub.cpp src\repl.cpp src\stdlib\stdlib.cpp src\stdlib\stdlib_fix_missing.cpp src\stdlib\stdlib_stubs.cpp src\stdlib\stdlib_imgui.cpp src\stdlib\stdlib_raylib_unit.cpp src\miniz.c src\miniz_tdef.c src\miniz_tinfl.c src\crypto\md5_impl.cpp src\sqlite\sqlite3.c src\optimizer\optimizer.cpp src\optimizer\constant_folder.cpp src\optimizer\dead_code_eliminator.cpp src\optimizer\function_inliner.cpp src\optimizer\tail_recursion_optimizer.cpp src\optimizer\loop_unroller.cpp src\optimizer\escape_analyzer.cpp src\module\module_system.cpp src\exception\exception_handler.cpp src\debug\debugger.cpp src\aot\aot_vm_bridge.cpp "
if "!HAS_LLVM!"=="1" ( set "SRCS=!SRCS! src\codegen\aot_compiler.cpp src\codegen\llvm_codegen.cpp src\optimizer\llvm_optimizer.cpp src\jit\jit_compiler.cpp src\jit\jit_runtime.cpp src\jit\orc_jit.cpp src\jit\hybrid_jit.cpp" ) else ( set "SRCS=!SRCS! src\jit\hybrid_jit_stub.cpp src\codegen\aot_stub.cpp" )
set "SRCS=!SRCS! third_party\imgui\imgui.cpp third_party\imgui\imgui_draw.cpp third_party\imgui\imgui_tables.cpp third_party\imgui\imgui_widgets.cpp third_party\imgui\rlImGui.cpp"
REM raylib compiled from source (uses OPENGL_21 define)
set "SRCS=!SRCS! third_party\raylib\src\rcore.c third_party\raylib\src\rshapes.c third_party\raylib\src\rtextures.c third_party\raylib\src\rtext.c third_party\raylib\src\rmodels.c third_party\raylib\src\raudio.c third_party\raylib\src\rglfw.c"
REM Resource file (icon, version info)
set "SRCS=!SRCS! build\cplang.res"
set SYS_LIBS=Shell32.lib Winhttp.lib Ws2_32.lib Cabinet.lib opengl32.lib gdi32.lib winmm.lib ole32.lib comctl32.lib user32.lib urlmon.lib
REM raylib is compiled from source
set RAYLIB_LIB=
set LINK_FLAGS=/FORCE:MULTIPLE /ignore:4006 /ignore:4088
if "!HAS_LLVM!"=="1" set "LINK_FLAGS=!LINK_FLAGS! /LIBPATH:!LLVM_LIBDIR!"
echo [Cleaning...]
if exist *.obj del /Q *.obj 2>nul
if exist build\*.obj del /Q build\*.obj 2>nul
echo [Building CP Language...]
if "!HAS_LLVM!"=="1" ( echo [Mode: JIT enabled] ) else ( echo [Mode: Bytecode only] )
REM cb_dispatcher 已改为 C++ 全局变量方案，无需 MASM
REM 保留 asm 文件供将来参考

cl !CL_FLAGS! !SRCS! /Fe:build\cplang.exe /link !SYS_LIBS! !RAYLIB_LIB! !LINK_FLAGS! !LLVM_LIBS!
if %ERRORLEVEL% NEQ 0 (
    echo [Build Failed]
    endlocal
    exit /b 1
)
echo [Build Success!]
if "!HAS_LLVM!"=="1" echo [JIT engine: LLVM enabled]

REM 创建完整的 cplang.lib 静态库
if exist build\*.obj (
    echo [Building cplang_full.lib static library for AOT linker...]
    REM 排除 LLVM 相关 .obj（需完整 LLVM lib 支持，AOT 不需要）
    for %%x in (llvm_codegen jit_compiler orc_jit hybrid_jit llvm_optimizer aot_compiler jit_runtime) do if exist build\%%x.obj del build\%%x.obj
    REM 清理残留的 .hide 文件，然后临时隐藏图形 .obj
    if exist build\*.obj.hide del /Q build\*.obj.hide 2>nul
    for %%x in (rcore rshapes rtextures rtext rmodels raudio rglfw imgui imgui_draw imgui_tables imgui_widgets rlImGui) do (
        if exist build\%%x.obj.hide del build\%%x.obj.hide
        if exist build\%%x.obj ren build\%%x.obj %%x.obj.hide
    )
    REM 编译 HybridJIT 存根（替换真实 JIT 实现，避免 AOT 链接时引用 LLVM 符号）
    cl /c /EHsc /std:c++17 /O1 /nologo /utf-8 /I"%SCRIPT_DIR%include" "%SCRIPT_DIR%src\jit\hybrid_jit_stub.cpp" /Fo:%SCRIPT_DIR%build\hybrid_jit_stub.obj
    cl /c /EHsc /std:c++17 /O1 /nologo /utf-8 /I"%SCRIPT_DIR%include" "%SCRIPT_DIR%src\codegen\aot_stub.cpp" /Fo:%SCRIPT_DIR%build\aot_stub.obj
    REM 编译 stdlib 图形存根（替代 raylib/ImGui 注册函数，避免依赖 raylib 图形库）
    cl /c /EHsc /std:c++17 /O1 /nologo /utf-8 /I"%SCRIPT_DIR%include" "%SCRIPT_DIR%src\aot\aot_stdlib_stubs.cpp" /Fo:%SCRIPT_DIR%build\aot_stdlib_stubs.obj
    REM Compile aot_vm_bridge into cplang_full.lib
    REM 临时隐藏图形注册 obj（cplang_full.lib 用存根）
    if exist build\stdlib_raylib_unit.obj ren build\stdlib_raylib_unit.obj stdlib_raylib_unit.obj.hide
    if exist build\stdlib_imgui.obj ren build\stdlib_imgui.obj stdlib_imgui.obj.hide
    lib /OUT:build\cplang_full.lib build\*.obj 2>nul
    if exist build\cplang_full.lib (
        echo [Created build\cplang_full.lib]
    )
    REM 恢复全部图形 obj（包括 raylib/ImGui C 源码对象）
    for %%x in (rcore rshapes rtextures rtext rmodels raudio rglfw imgui imgui_draw imgui_tables imgui_widgets rlImGui stdlib_raylib_unit stdlib_imgui) do (
        if exist build\%%x.obj.hide (
            if exist build\%%x.obj del build\%%x.obj
            ren build\%%x.obj.hide %%x.obj
        )
    )
    if exist build\aot_stdlib_stubs.obj del build\aot_stdlib_stubs.obj
    lib /OUT:build\cplang_graphics.lib build\*.obj 2>nul
    if exist build\cplang_graphics.lib (
        echo [Created build\cplang_graphics.lib]
    )
)

REM 为 AOT 链接器编译独立的 jit_runtime
if "!HAS_LLVM!"=="1" (
    echo [Building jit_runtime standalone for AOT linker...]
    cl /c /EHsc /std:c++17 /O2 /MD /nologo /utf-8 /I"%SCRIPT_DIR%include" "%SCRIPT_DIR%src\jit\jit_runtime_standalone.cpp" /Fo:build\jit_runtime_standalone.obj
    if exist build\jit_runtime_standalone.obj (
        lib /OUT:build\jit_runtime.lib build\jit_runtime_standalone.obj 2>nul
        echo [Created build\jit_runtime.lib]
    )
)

echo [Cleaning intermediates...]
if exist build\*.obj del /Q build\*.obj 2>nul
if exist build\*.exp del /Q build\*.exp 2>nul

REM 为 AOT 链接器编译 aot_vm_bridge（独立 obj，不清除）
if "!HAS_LLVM!"=="1" (
    cl /c /EHsc /std:c++17 /O2 /nologo /utf-8 /I"%SCRIPT_DIR%include" "%SCRIPT_DIR%src\aot\aot_vm_bridge.cpp" /Fo:build\aot_vm_bridge.obj
    if exist build\aot_vm_bridge.obj (
        echo [Created build\aot_vm_bridge.obj]
    )
)
endlocal
exit /b 0
