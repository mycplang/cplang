# CP语言环境配置下载脚本
# 下载: PortableGit + LLVM 完整开发包 (用于 JIT)

$ErrorActionPreference = "Stop"
$outDir = "C:\Users\Administrator\Downloads"

# ─── Git Portable ───
Write-Host "=== 下载 PortableGit ==="
$gitUrl = "https://github.com/git-for-windows/git/releases/download/v2.49.0.windows.1/PortableGit-2.49.0-64-bit.7z.exe"
$gitOut = "$outDir\PortableGit-2.49.0-64-bit.7z.exe"

if (Test-Path $gitOut) {
    Write-Host "  Git 已存在, 跳过"
} else {
    Write-Host "  从 $gitUrl 下载..."
    try {
        Invoke-WebRequest -Uri $gitUrl -OutFile $gitOut -ErrorAction Stop
        Write-Host "  Git 下载完成: $gitOut"
    } catch {
        Write-Host "  Git 下载失败: $_"
    }
}

# ─── LLVM 18.1.8 完整开发包 ───
Write-Host "=== 下载 LLVM 18.1.8 (完整开发包) ==="
$llvmUrl = "https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/LLVM-18.1.8-win64.exe"
$llvmOut = "$outDir\LLVM-18.1.8-win64.exe"

if (Test-Path $llvmOut) {
    Write-Host "  LLVM 已存在, 跳过"
} else {
    Write-Host "  从 $llvmUrl 下载..."
    Write-Host "  (文件较大, 约 600MB, 请耐心等待...)"
    try {
        Invoke-WebRequest -Uri $llvmUrl -OutFile $llvmOut -ErrorAction Stop
        Write-Host "  LLVM 下载完成: $llvmOut"
    } catch {
        Write-Host "  LLVM 下载失败: $_"
    }
}

Write-Host ""
Write-Host "=== 下载完毕 ==="
Write-Host "Git Portable: $gitOut"
Write-Host " 解压到任意目录, 将解压目录加入 PATH 后即可使用 git"
Write-Host ""
Write-Host "LLVM 18.1.8: $llvmOut"
Write-Host " 双击运行安装, 选择 Add to PATH 完成安装"
Write-Host " 安装后 cmake 会自动检测到 LLVM, JIT 编译即可启用"