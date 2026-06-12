# publish_packages.ps1 — 上传模块包到 Gitee
# 用法: 
#   设置环境变量: $env:CPKG_GITEE_TOKEN = "your_token"
#   运行: .\publish_packages.ps1
param()

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocationManager.MyCommand.Path

$token = $env:CPKG_GITEE_TOKEN
if (-not $token) {
    Write-Host "错误: 请设置环境变量 CPKG_GITEE_TOKEN" -ForegroundColor Red
    Write-Host "  `$env:CPKG_GITEE_TOKEN = 'your_token'" -ForegroundColor Yellow
    exit 1
}

$owner = "cplang"
$repo  = "bao"
$apiBase = "https://gitee.com/api/v5/repos/$owner/$repo/contents"

$modules = @(
    @{name="graphics"; file="graphics.lib"; desc="图形模块（Raylib 2D/3D + ImGui）"},
    @{name="database";  file="database.lib";  desc="数据库模块（SQLite + MySQL + Redis）"},
    @{name="crypto";    file="crypto.lib";    desc="加密模块（AES + SHA + MD5 + Base64）"},
    @{name="ffi";       file="ffi.lib";       desc="外部函数接口模块"},
    @{name="network";   file="network.lib";   desc="网络模块（HTTP + JSON）"},
    @{name="container"; file="container.lib"; desc="高级容器模块（栈/队列/链表/集合/映射）"},
    @{name="concurrent";file="concurrent.lib";desc="并发模块（线程/互斥/Channel）"},
    @{name="string_ext";file="string_ext.lib";desc="字符串高级模块（正则/格式化/搜索）"},
    @{name="charset";   file="charset.lib";   desc="字符集转换模块（GBK/Big5）"},
    @{name="algorithm"; file="algorithm.lib"; desc="算法模块（排序/查找/随机/位运算）"}
)

Write-Host "╔══════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║   CP 语言 — 包发布工具 (Gitee)           ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

foreach ($mod in $modules) {
    $name = $mod.name
    $file = $mod.file
    $desc = $mod.desc
    $localPath = "$root\build_modules\$name\$name\$file"
    
    # 也检查 modules 目录下的编译产物
    if (-not (Test-Path $localPath)) {
        $localPath = "$root\modules\$name\win-x64\$file"
    }
    
    if (-not (Test-Path $localPath)) {
        Write-Host "  ✗ @cp/$name — 未找到 $file，请先运行 build_packages.ps1" -ForegroundColor Red
        continue
    }
    
    Write-Host "▸ 上传 @cp/$name ($desc) ..." -ForegroundColor Yellow
    $fileSize = (Get-Item $localPath).Length
    Write-Host "  文件: $localPath ($([math]::Round($fileSize/1KB, 1)) KB)" -ForegroundColor Gray
    
    $bytes = [System.IO.File]::ReadAllBytes($localPath)
    $base64 = [System.Convert]::ToBase64String($bytes)
    
    $remotePath = "packages/@cp/$name/win-x64/$file"
    $body = @{
        access_token = $token
        content      = $base64
        message      = "发布 @cp/$name v0.2.0 — $desc"
    } | ConvertTo-Json
    
    try {
        $response = Invoke-RestMethod -Uri "$apiBase/$remotePath" `
            -Method Post `
            -Body $body `
            -ContentType "application/json"
        Write-Host "  ✓ @cp/$name 上传成功" -ForegroundColor Green
    } catch {
        $errMsg = $_.Exception.Message
        if ($errMsg -match "already exists") {
            Write-Host "  ⚠ @cp/$name 已存在，跳过（如需更新请手动删除后重试）" -ForegroundColor Yellow
        } else {
            Write-Host "  ✗ @cp/$name 上传失败: $errMsg" -ForegroundColor Red
        }
    }
}

Write-Host ""
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
Write-Host "请手动更新 index.json:" -ForegroundColor Yellow
Write-Host "  https://gitee.com/$owner/$repo/edit/main/index.json" -ForegroundColor White
Write-Host ""
Write-Host "index.json 参考内容见: $root\modules\index.json" -ForegroundColor Gray
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
