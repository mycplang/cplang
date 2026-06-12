# build_packages.ps1 — 编译所有模块包
# 用法: .\build_packages.ps1 [-Config Release|Debug]
param([string]$Config = "Release")

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "╔══════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║   CP 语言 — 模块包编译工具               ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

$modules = @("graphics", "database", "crypto", "ffi", "network", "container", "concurrent", "string_ext", "charset", "algorithm")

foreach ($mod in $modules) {
    Write-Host "▸ 编译 @cp/$mod ..." -ForegroundColor Yellow
    $buildDir = "$root\build_modules\$mod"
    $moduleDir = "$root\modules\$mod"
    
    # CMake configure
    cmake -S $moduleDir -B $buildDir -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=$Config
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed for @cp/$mod" }
    
    # Build
    cmake --build $buildDir --config $Config
    if ($LASTEXITCODE -ne 0) { throw "Build failed for @cp/$mod" }
    
    Write-Host "  ✓ @cp/$mod 编译完成" -ForegroundColor Green
}

Write-Host ""
Write-Host "全部模块编译完成!" -ForegroundColor Green
Write-Host "输出目录: $root\modules\*\win-x64\" -ForegroundColor Gray
