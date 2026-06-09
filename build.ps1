# CP Language Main Build Script
param(
    [switch]$NoLLVM,
    [switch]$Clean,
    [string]$Config = "Release",
    [string]$Out = "build\cplang.exe"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

# ─── 检测 VS 环境 ───────────────────────────────────────────────
function Find-VisualStudio {
    $knownPaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise",
        "D:\Program Files\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles}\Microsoft Visual Studio\2019\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community"
    )
    foreach ($p in $knownPaths) {
        $bat = Join-Path $p "VC\Auxiliary\Build\vcvarsall.bat"
        if (Test-Path $bat) { return $p }
    }
    return $null
}

$vsRoot = Find-VisualStudio
if (-not $vsRoot) {
    # Check if we're already in a VS prompt
    if ($env:VSCMD_ARG_TGT_ARCH) {
        Write-Host "[VS] Using existing developer environment" -ForegroundColor Green
    } else {
        Write-Host "[ERROR] Visual Studio not found. Install VS or run from a Developer Command Prompt." -ForegroundColor Red
        exit 1
    }
} else {
    $vsBat = Join-Path $vsRoot "VC\Auxiliary\Build\vcvarsall.bat"
    Write-Host "[VS] Found: $vsRoot" -ForegroundColor Green

    # Run vcvarsall to set up environment
    $arch = "x64"
    & cmd.exe /c "`"$vsBat`" $arch > nul 2>&1 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], [System.EnvironmentVariableTarget]::Process)
        }
    }
}

# ─── 清理 ───────────────────────────────────────────────────────
if ($Clean) {
    Write-Host "[Clean] Removing build artifacts..." -ForegroundColor Yellow
    Remove-Item -Path "build\*.obj", "build\*.exp", "build\*.lib", "*.obj" -Force -ErrorAction SilentlyContinue
}

# Kill any zombie cplang.exe processes
Get-Process -Name "cplang" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

# ─── 检测 LLVM ──────────────────────────────────────────────────
$llvmDir = Join-Path $scriptDir "llvm-dev"
$llvmCfg = Join-Path $llvmDir "bin\llvm-config.exe"
$hasLLVM = (-not $NoLLVM) -and (Test-Path $llvmCfg)

if ($hasLLVM) {
    try {
        $llvmInc = (& "$llvmCfg" --includedir 2>$null) -join " "
        $llvmLibDir = (& "$llvmCfg" --libdir 2>$null) -join " "
        $llvmLibs = (& "$llvmCfg" --libnames core orcjit native irreader 2>$null) -join " "
        if ([string]::IsNullOrEmpty($llvmInc)) { $hasLLVM = $false }
    } catch {
        Write-Host "[WARN] LLVM detection failed: $_" -ForegroundColor Yellow
        $hasLLVM = $false
    }
}

if ($hasLLVM) {
    Write-Host "[LLVM] Found at $llvmDir — JIT/AOT enabled" -ForegroundColor Green
} else {
    Write-Host "[LLVM] Not found — building in bytecode-only mode" -ForegroundColor Yellow
}

# ─── 编译器选项 ──────────────────────────────────────────────────
$clOpts = "/utf-8 /std:c++17 /EHsc /W3 /O1 /MD /wd4244"
$clInc = '/I"include" /I"third_party\raylib\src" /I"third_party\raylib\src\external\glfw\include" /I"third_party\imgui"'
$clDef = '/D_CRT_SECURE_NO_WARNINGS /DNDEBUG /DMINIZ_NO_ARCHIVE_APIS /DGRAPHICS_API_OPENGL_21 /DPLATFORM_DESKTOP /DNO_FONT_AWESOME'

if ($hasLLVM) {
    $clInc += ' /I"', $llvmInc, '"' -join ''
    $clDef += ' /DCPLANG_HAS_LLVM'
}

$clFlags = "$clOpts /Fo:build\ $clInc $clDef"

# ─── 源文件列表 ──────────────────────────────────────────────────
$srcs = @()
$srcs += "src\main.cpp"
$srcs += "src\core\verbose.cpp"
$srcs += "src\lexer\lexer.cpp"
$srcs += "src\parser\parser.cpp", "src\parser\parser_decl.cpp", "src\parser\parser_stmt.cpp", "src\parser\parser_expr.cpp"
$srcs += "src\semantic\semantic_analyzer.cpp"
$srcs += "src\codegen\codegen.cpp", "src\codegen\codegen_opt.cpp", "src\codegen\codegen_stmt.cpp", "src\codegen\codegen_expr.cpp", "src\codegen\bytecode_optimizer.cpp"
$srcs += "src\vm\vm.cpp", "src\vm\vm_containers.cpp", "src\vm\vm_objects.cpp", "src\vm\vm_exec.cpp", "src\vm\value.cpp", "src\vm\vm_opt_stub.cpp"
$srcs += "src\repl.cpp"
$srcs += "src\stdlib\stdlib.cpp", "src\stdlib\stdlib_fix_missing.cpp", "src\stdlib\stdlib_stubs.cpp", "src\stdlib\stdlib_raylib_unit.cpp"
$srcs += "src\miniz.c", "src\miniz_tdef.c", "src\miniz_tinfl.c"
$srcs += "src\crypto\md5_impl.cpp"
$srcs += "src\sqlite\sqlite3.c"
$srcs += "src\optimizer\optimizer.cpp", "src\optimizer\constant_folder.cpp", "src\optimizer\dead_code_eliminator.cpp", "src\optimizer\function_inliner.cpp", "src\optimizer\tail_recursion_optimizer.cpp", "src\optimizer\loop_unroller.cpp", "src\optimizer\escape_analyzer.cpp"
$srcs += "src\module\module_system.cpp"
$srcs += "src\exception\exception_handler.cpp"
$srcs += "src\debug\debugger.cpp"

if ($hasLLVM) {
    $srcs += "src\codegen\aot_compiler.cpp", "src\codegen\llvm_codegen.cpp", "src\optimizer\llvm_optimizer.cpp"
    $srcs += "src\jit\jit_compiler.cpp", "src\jit\jit_runtime.cpp", "src\jit\orc_jit.cpp"
}

$srcs += "third_party\imgui\imgui.cpp", "third_party\imgui\imgui_draw.cpp", "third_party\imgui\imgui_tables.cpp", "third_party\imgui\imgui_widgets.cpp", "third_party\imgui\rlImGui.cpp"

# ─── 链接选项 ──────────────────────────────────────────────────
$sysLibs = "Shell32.lib Winhttp.lib Ws2_32.lib Cabinet.lib opengl32.lib gdi32.lib winmm.lib ole32.lib comctl32.lib user32.lib urlmon.lib"
$raylibLib = "third_party\raylib\build_release\raylib\raylib.lib"
$linkFlags = "/FORCE:MULTIPLE /ignore:4006 /ignore:4088"
if ($hasLLVM) {
    $linkFlags += " /LIBPATH:`"$llvmLibDir`""
}

$allSrcs = $srcs -join " "

# ─── 构建主程序 ──────────────────────────────────────────────────
Write-Host "[Building CP Language...]" -ForegroundColor Cyan
if ($hasLLVM) {
    Write-Host "[Mode: JIT enabled]" -ForegroundColor Green
} else {
    Write-Host "[Mode: Bytecode only]" -ForegroundColor Yellow
}

$clCmd = "cl $clFlags $allSrcs /Fe:$Out /link $sysLibs $raylibLib $linkFlags $llvmLibs"
Write-Host "[CMD] $clCmd" -ForegroundColor DarkGray

$result = Invoke-Expression $clCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "[Build Failed]" -ForegroundColor Red
    exit 1
}

Write-Host "[Build Success!]" -ForegroundColor Green
if ($hasLLVM) {
    Write-Host "[JIT engine: LLVM enabled]" -ForegroundColor Green
}

# ─── 构建 JIT 运行时（用于 AOT 链接器） ──────────────────────────
if ($hasLLVM) {
    Write-Host "[Building jit_runtime standalone for AOT linker...]" -ForegroundColor Cyan
    $aotClOpts = "/c /EHsc /std:c++17 /O2 /nologo /utf-8 /MT /I`"include`""
    $jitCmd = "cl $aotClOpts `"src\jit\jit_runtime_standalone.cpp`" /Fo:build\jit_runtime_standalone.obj"
    Invoke-Expression $jitCmd 2>$null
    if (Test-Path "build\jit_runtime_standalone.obj") {
        & lib /OUT:build\jit_runtime.lib build\jit_runtime_standalone.obj 2>$null
        Write-Host "[Created build\jit_runtime.lib]" -ForegroundColor Green
    }

    Write-Host "[Building aot_vm_bridge for AOT linker...]" -ForegroundColor Cyan
    $bridgeCmd = "cl $aotClOpts `"src\aot\aot_vm_bridge.cpp`" /Fo:build\aot_vm_bridge.obj"
    Invoke-Expression $bridgeCmd 2>$null
    if (Test-Path "build\aot_vm_bridge.obj") {
        Write-Host "[Created build\aot_vm_bridge.obj]" -ForegroundColor Green
    }
}

# ─── 部署 AOT 支持文件到 exe 同目录 ────────────────────────────────
if ($hasLLVM) {
    Write-Host "[Deploying AOT support files...]" -ForegroundColor Cyan
    # 从 build_msvc\bin\ 复制预编译的 stdlib 静态库
    foreach ($lib in @("cplang_aot.lib", "cplang_graphics.lib", "raylib.lib")) {
        $src = "build_msvc\bin\$lib"
        if (Test-Path $src) {
            Copy-Item $src build\ -Force
            Write-Host "  Copied $lib" -ForegroundColor Gray
        }
    }
    # jit_runtime.lib 和 aot_vm_bridge.obj 已在本脚本中编译到 build\
}

# ─── 清理中间文件 ────────────────────────────────────────────────
Write-Host "[Cleaning intermediates...]" -ForegroundColor Yellow
Remove-Item -Path "build\*.exp" -Force -ErrorAction SilentlyContinue
# 保留 AOT 链接所需的 obj 文件
Get-ChildItem "build\*.obj" -Exclude "aot_vm_bridge.obj" -ErrorAction SilentlyContinue | Remove-Item -Force

Write-Host "[Done]" -ForegroundColor Green
exit 0