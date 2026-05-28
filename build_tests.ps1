# CP Language Test Build Script
param(
    [switch]$NoLLVM,
    [switch]$NoRun,
    [string]$Out = "build\cplang_tests.exe"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

# ─── 检测 VS 环境 ───────────────────────────────────────────────
$vsRoot = $null
$knownPaths = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise",
    "${env:ProgramFiles}\Microsoft Visual Studio\2019\Community",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community"
)
foreach ($p in $knownPaths) {
    $bat = Join-Path $p "VC\Auxiliary\Build\vcvarsall.bat"
    if (Test-Path $bat) { $vsRoot = $p; break }
}

if (-not $vsRoot) {
    Write-Host "[WARN] Visual Studio not found. Trying existing PATH..." -ForegroundColor Yellow
}

# ─── 检测 LLVM ──────────────────────────────────────────────────
$llvmDir = Join-Path $scriptDir "llvm-dev"
$llvmCfg = Join-Path $llvmDir "bin\llvm-config.exe"
$hasLLVM = (-not $NoLLVM) -and (Test-Path $llvmCfg)

if ($hasLLVM) {
    try {
        $llvmInc = (& "$llvmCfg" --includedir 2>$null) -join " "
        $llvmLibDir = (& "$llvmCfg" --libdir 2>$null) -join " "
        # Use --libnames to get shorter library names (avoids command line length issues)
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
$clOpts = "/utf-8 /std:c++17 /EHsc /W3 /O1 /MD /wd4244 /wd4018 /wd4996 /wd4267"
$clInc = '/I"include" /I"third_party\raylib\src" /I"third_party\raylib\src\external\glfw\include" /I"third_party\imgui" /I"tests"'
$clDef = '/D_CRT_SECURE_NO_WARNINGS /DNDEBUG /DMINIZ_NO_ARCHIVE_APIS /DGRAPHICS_API_OPENGL_33 /DPLATFORM_DESKTOP /DNO_FONT_AWESOME /DCPLANG_TEST_BUILD'

if ($hasLLVM) {
    $clInc += ' /I"', $llvmInc, '"' -join ''
    $clDef += ' /DCPLANG_HAS_LLVM'
}

$clFlags = "$clOpts $clInc $clDef"

# ─── 源文件列表 ──────────────────────────────────────────────────
$srcs = @()
$srcs += "src\core\verbose.cpp"
$srcs += "src\lexer\lexer.cpp"
$srcs += "src\parser\parser.cpp", "src\parser\parser_decl.cpp", "src\parser\parser_stmt.cpp", "src\parser\parser_expr.cpp"
$srcs += "src\semantic\semantic_analyzer.cpp"
$srcs += "src\codegen\codegen.cpp", "src\codegen\codegen_opt.cpp", "src\codegen\codegen_stmt.cpp", "src\codegen\codegen_expr.cpp", "src\codegen\bytecode_optimizer.cpp"
$srcs += "src\vm\vm.cpp", "src\vm\vm_containers.cpp", "src\vm\vm_objects.cpp", "src\vm\vm_exec.cpp", "src\vm\value.cpp", "src\vm\vm_opt_stub.cpp"
$srcs += "src\module\module_system.cpp"
$srcs += "src\stdlib\stdlib.cpp", "src\stdlib\stdlib_fix_missing.cpp", "src\stdlib\stdlib_stubs.cpp", "src\stdlib\stdlib_raylib_unit.cpp"
$srcs += "src\debug\debugger.cpp"
$srcs += "src\exception\exception_handler.cpp"
$srcs += "src\optimizer\optimizer.cpp", "src\optimizer\constant_folder.cpp", "src\optimizer\dead_code_eliminator.cpp", "src\optimizer\function_inliner.cpp", "src\optimizer\tail_recursion_optimizer.cpp", "src\optimizer\loop_unroller.cpp", "src\optimizer\escape_analyzer.cpp"
$srcs += "src\miniz.c", "src\miniz_tdef.c", "src\miniz_tinfl.c", "src\crypto\md5_impl.cpp", "src\sqlite\sqlite3.c"
$srcs += "third_party\imgui\imgui.cpp", "third_party\imgui\imgui_draw.cpp", "third_party\imgui\imgui_tables.cpp", "third_party\imgui\imgui_widgets.cpp", "third_party\imgui\rlImGui.cpp"

if ($hasLLVM) {
    $srcs += "src\codegen\aot_compiler.cpp", "src\codegen\llvm_codegen.cpp", "src\optimizer\llvm_optimizer.cpp", "src\jit\jit_compiler.cpp", "src\jit\jit_runtime.cpp", "src\jit\orc_jit.cpp"
}

$testSrcs = @("tests\run_all_tests.cpp")

# ─── 链接选项 ──────────────────────────────────────────────────
$sysLibs = "Shell32.lib Winhttp.lib Ws2_32.lib Cabinet.lib opengl32.lib gdi32.lib winmm.lib ole32.lib comctl32.lib user32.lib urlmon.lib"
$raylibLib = "third_party\raylib\build_release\raylib\Release\raylib.lib"
$linkFlags = "/FORCE:MULTIPLE /ignore:4006 /ignore:4088"
if ($hasLLVM) {
    $linkFlags += " /LIBPATH:`"$llvmLibDir`""
}

# IMPORTANT: test file MUST come first to avoid static init order crash (MSVC linker quirk)
$allSrcs = ($testSrcs + $srcs) -join " "

# ─── 构建 ───────────────────────────────────────────────────────
Write-Host "[Building CP Language Tests...]" -ForegroundColor Cyan

$clCmd = "cl $clFlags $allSrcs /Fe:$Out /link $sysLibs $raylibLib $linkFlags $llvmLibs"
Write-Host "[CMD] $clCmd" -ForegroundColor DarkGray

$result = Invoke-Expression $clCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "[Test Build Failed]" -ForegroundColor Red
    exit 1
}

Write-Host "[Test Build Success!]" -ForegroundColor Green

# ─── 运行测试 ──────────────────────────────────────────────────
if ($NoRun) {
    Write-Host "[Tests NOT run (--NoRun specified)]" -ForegroundColor Yellow
    exit 0
}

Write-Host "[Running Tests...]" -ForegroundColor Cyan

$testPath = $Out
if (-not [System.IO.Path]::IsPathRooted($testPath)) {
    $testPath = Join-Path (Get-Location) $testPath
}

if (Test-Path $testPath) {
    & $testPath
    exit $LASTEXITCODE
} else {
    Write-Host "[ERROR] Test binary not found: $testPath" -ForegroundColor Red
    exit 1
}
