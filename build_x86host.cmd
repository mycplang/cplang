@echo off
cd /d D:\CPLANG

REM Use cl.exe from Hostx86\x64 (32-bit host running 64-bit compiler)
set "MSVCROOT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
set "CL_EXE=%MSVCROOT%\bin\Hostx86\x64\cl.exe"

REM Set up paths needed by cl.exe
set "PATH=%MSVCROOT%\bin\Hostx86\x64;%MSVCROOT%\bin\Hostx64\x64;%PATH%"
set "LIB=%MSVCROOT%\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64"
set "INCLUDE=%MSVCROOT%\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared"

echo [Testing cl.exe from Hostx86\x64...]
echo int main(){return 0;}>_test_msvc.c
"%CL_EXE%" /nologo /c _test_msvc.c /Fo_test_msvc.obj
if %ERRORLEVEL% EQU 0 (
    echo [OK! cl.exe from Hostx86\x64 works]
    del _test_msvc.c _test_msvc.obj 2>nul

    echo.
    echo [Now building full cplang.exe using Hostx86\x64 cl.exe...]
    
    set "HAS_LLVM=1"
    set "LLVM_DIR=D:\CPLANG\llvm-dev"
    set "LLVM_INC=%LLVM_DIR%\include"
    set "LLVM_LIBDIR=%LLVM_DIR%\lib"
    set "LLVM_LIBS=LLVMDemangle.lib LLVMSupport.lib LLVMTableGen.lib LLVMTableGenGlobalISel.lib LLVMTableGenCommon.lib LLVMCore.lib LLVMFuzzerCLI.lib LLVMIRReader.lib LLVMCodeGenTypes.lib LLVMCodeGen.lib LLVMSelectionDAG.lib LLVMAsmPrinter.lib LLVMMIRParser.lib LLVMGlobalISel.lib LLVMDebugInfoDWARF.lib LLVMDebugInfoGSYM.lib LLVMDebugInfoCodeView.lib LLVMDebugInfoMSF.lib LLVMDebugInfoPDB.lib LLVMDebugInfoBTF.lib LLVMDWARFLinker.lib LLVMDWARFLinkerClassic.lib LLVMDWARFLinkerParallel.lib LLVMDWP.lib LLVMAggressiveInstCombine.lib LLVMInstCombine.lib LLVMScalarOpts.lib LLVMipo.lib LLVMVectorize.lib LLVMInstrumentation.lib LLVMCFGuard.lib LLVMCFIVerify.lib LLVMLinker.lib LLVMAnalysis.lib LLVMTransformUtils.lib LLVMTarget.lib LLVMPasses.lib LLVMMC.lib LLVMMCParser.lib LLVMMCDisassembler.lib LLVMMCA.lib LLVMObject.lib LLVMObjectYAML.lib LLVMBinaryFormat.lib LLVMBitReader.lib LLVMBitWriter.lib LLVMBitstreamReader.lib LLVMOption.lib LLVMDiff.lib LLVMProfileData.lib LLVMCoverage.lib LLVMDebuginfod.lib LLVMTextAPI.lib LLVMTextAPIBinaryReader.lib LLVMRemarks.lib LLVMSymbolize.lib LLVMExecutionEngine.lib LLVMRuntimeDyld.lib LLVMJITLink.lib LLVMOrcJIT.lib LLVMOrcDebugging.lib LLVMOrcShared.lib LLVMOrcTargetProcess.lib LLVMTargetParser.lib LLVMLineEditor.lib LLVMFrontendOpenACC.lib LLVMFrontendOpenMP.lib LLVMFrontendHLSL.lib LLVMFrontendDriver.lib LLVMFrontendOffloading.lib LLVMExtensions.lib LLVMWindowsDriver.lib LLVMWindowsManifest.lib LLVMXRay.lib LLVMAsmParser.lib LLVMObjCARCOpts.lib LLVMCoroutines.lib LLVMFuzzMutate.lib LLVMFileCheck.lib LLVMInterfaceStub.lib LLVMObjCopy.lib LLVMDlltoolDriver.lib LLVMLibDriver.lib LLVMHipStdPar.lib LLVMIRPrinter.lib LLVMX86Info.lib LLVMX86Desc.lib LLVMX86CodeGen.lib LLVMX86AsmParser.lib LLVMX86Disassembler.lib LLVMX86TargetMCA.lib"
    
    set "CL_OPTS=/utf-8 /std:c++17 /EHsc /W3 /O1 /MD /wd4244 /Zm600 /D_CRT_SECURE_NO_WARNINGS /DNDEBUG /DMINIZ_NO_ARCHIVE_APIS /DCPLANG_HAS_LLVM /DGRAPHICS_API_OPENGL_21 /DPLATFORM_DESKTOP /DNO_FONT_AWESOME"
    set "CL_INC=/Iinclude /Ithird_party\raylib\src /Ithird_party\raylib\src\external\glfw\include /Ithird_party\imgui /I%LLVM_INC%"
    set "SRCS=src\main.cpp src\core\verbose.cpp src\lexer\lexer.cpp src\parser\parser.cpp src\parser\parser_decl.cpp src\parser\parser_stmt.cpp src\parser\parser_expr.cpp src\semantic\semantic_analyzer.cpp src\codegen\codegen.cpp src\codegen\codegen_opt.cpp src\codegen\codegen_stmt.cpp src\codegen\codegen_expr.cpp src\codegen\bytecode_optimizer.cpp src\vm\vm.cpp src\vm\vm_containers.cpp src\vm\vm_objects.cpp src\vm\vm_exec.cpp src\vm\value.cpp src\vm\vm_opt_stub.cpp src\repl.cpp src\stdlib\stdlib.cpp src\stdlib\stdlib_fix_missing.cpp src\stdlib\stdlib_stubs.cpp src\stdlib\stdlib_imgui.cpp src\stdlib\stdlib_raylib_unit.cpp src\miniz.c src\miniz_tdef.c src\miniz_tinfl.c src\crypto\md5_impl.cpp src\sqlite\sqlite3.c src\optimizer\optimizer.cpp src\optimizer\constant_folder.cpp src\optimizer\dead_code_eliminator.cpp src\optimizer\function_inliner.cpp src\optimizer\tail_recursion_optimizer.cpp src\optimizer\loop_unroller.cpp src\optimizer\escape_analyzer.cpp src\module\module_system.cpp src\exception\exception_handler.cpp src\debug\debugger.cpp src\codegen\aot_compiler.cpp src\codegen\llvm_codegen.cpp src\optimizer\llvm_optimizer.cpp src\jit\jit_compiler.cpp src\jit\jit_runtime.cpp src\jit\orc_jit.cpp src\jit\hybrid_jit.cpp third_party\imgui\imgui.cpp third_party\imgui\imgui_draw.cpp third_party\imgui\imgui_tables.cpp third_party\imgui\imgui_widgets.cpp third_party\imgui\rlImGui.cpp third_party\raylib\src\rcore.c third_party\raylib\src\rshapes.c third_party\raylib\src\rtextures.c third_party\raylib\src\rtext.c third_party\raylib\src\rmodels.c third_party\raylib\src\raudio.c third_party\raylib\src\rglfw.c"
    set "SYS_LIBS=Shell32.lib Winhttp.lib Ws2_32.lib Cabinet.lib opengl32.lib gdi32.lib winmm.lib ole32.lib comctl32.lib user32.lib urlmon.lib"
    
    "%CL_EXE%" %CL_OPTS% %CL_INC% %SRCS% /Febuild\cplang.exe /link /LIBPATH:%LLVM_LIBDIR% %SYS_LIBS% %LLVM_LIBS% /FORCE:MULTIPLE /ignore:4006 /ignore:4088
    
    if %ERRORLEVEL% EQU 0 (
        echo [BUILD SUCCESS!]
    ) else (
        echo [BUILD FAILED]
    )
) else (
    echo [FAILED] cl.exe from Hostx86\x64 also fails
    del _test_msvc.c _test_msvc.obj 2>nul
)
