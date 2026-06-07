# CP Language 调试客户端 (PowerShell - Windows 自带，零依赖)
param([string]$file)

$CPLANG = "C:\cplang\build\cplang.exe"
$PORT = 4711

# 清理旧进程
Get-Process cplang -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 300

# 启动编译器调试服务器
$proc = Start-Process -FilePath $CPLANG -ArgumentList "--debug-server",$PORT,"-c",$file `
    -NoNewWindow -PassThru -RedirectStandardOutput "$env:TEMP\cp_debug_out.txt"

Start-Sleep -Seconds 1

# 连接 TCP
$sock = New-Object System.Net.Sockets.TcpClient
for ($i = 0; $i -lt 20; $i++) {
    try { $sock.Connect("127.0.0.1", $PORT); break }
    catch { Start-Sleep -Milliseconds 300 }
}
if (-not $sock.Connected) { Write-Host "无法连接调试服务器"; $proc.Kill(); exit 1 }

$stream = $sock.GetStream()
$reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::UTF8)
$writer = New-Object System.IO.StreamWriter($stream, [System.Text.Encoding]::UTF8)
$writer.AutoFlush = $true

function Send($cmd) { $writer.WriteLine((ConvertTo-Json $cmd)) }
function Recv {
    $line = $reader.ReadLine()
    if ($line) { ConvertFrom-Json $line } else { $null }
}

# 接收 connected
$msg = Recv
Write-Host $msg.type

# 设置断点
Send @{ cmd="setBreakpoints"; file=$file; lines=@() }
Write-Host "══ CP 调试器 ══  c=继续 s=单步 n=跳过 bt=堆栈 v=变量 q=退出"

$showPrompt = $true
while (-not $proc.HasExited) {
    if ($stream.DataAvailable) {
        $msg = Recv
        if ($msg.type -eq "paused") {
            Write-Host "`n⏸ $($msg.reason) | $($msg.file):$($msg.line)"
            $showPrompt = $true
        }
    }
    if ($showPrompt) { $cmd = Read-Host "(dbg)"; $showPrompt = $false } else { Start-Sleep -Milliseconds 100; continue }

    switch ($cmd) {
        '' { Send @{cmd="continue"}; $showPrompt = $true }
        'c' { Send @{cmd="continue"}; $showPrompt = $true }
        's' { Send @{cmd="stepInto"}; $showPrompt = $true }
        'n' { Send @{cmd="stepOver"}; $showPrompt = $true }
        'o' { Send @{cmd="stepOut"}; $showPrompt = $true }
        'bt' { Send @{cmd="getStack"}; $resp = Recv; foreach($f in $resp.frames){Write-Host "  #$($f.name) $($f.file):$($f.line)"}; $showPrompt = $true }
        'v' { Send @{cmd="getVars"}; $resp = Recv; foreach($p in $resp.vars.PSObject.Properties){Write-Host "  $($p.Name) = $($p.Value)"}; $showPrompt = $true }
        'q' { Send @{cmd="continue"}; break }
        default { Write-Host "?"; $showPrompt = $true }
    }
    # 打印编译器输出
    if (Test-Path "$env:TEMP\cp_debug_out.txt") {
        Get-Content "$env:TEMP\cp_debug_out.txt" -Tail 5 | ForEach-Object { if ($_.Trim()) { Write-Host $_ } }
    }
}

$reader.Close(); $writer.Close(); $sock.Close()
if (-not $proc.HasExited) { $proc.Kill() }
Write-Host "调试结束"
