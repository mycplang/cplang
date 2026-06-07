# CP Language Debug Client (PowerShell - zero dependencies)
param([string]$file)

$CPLANG = "C:\cplang\build\cplang.exe"
$PORT = 4711

# Kill old processes
Get-Process cplang -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500

# Start compiler in debug server mode
$proc = Start-Process -FilePath $CPLANG -ArgumentList "--debug-server $PORT -c $file" -NoNewWindow -PassThru
Start-Sleep -Seconds 2

# Connect TCP
$sock = New-Object System.Net.Sockets.TcpClient
for ($i = 0; $i -lt 20; $i++) {
    try { $sock.Connect("127.0.0.1", $PORT); break }
    catch { Start-Sleep -Milliseconds 300 }
}
if (-not $sock.Connected) { Write-Host "Cannot connect to debug server"; $proc.Kill(); exit 1 }

$stream = $sock.GetStream()
$reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::UTF8)
$writer = New-Object System.IO.StreamWriter($stream, [System.Text.Encoding]::UTF8)
$writer.AutoFlush = $true

function Send($cmd) { $writer.WriteLine((ConvertTo-Json -Compress $cmd)) }
function Recv {
    try { $line = $reader.ReadLine(); if ($line) { return ConvertFrom-Json $line } } catch {}
    return $null
}

$msg = Recv
Write-Host "Connected: $($msg.type)"

Send @{cmd="setBreakpoints"; file=$file; lines=@()}
Write-Host "== CP Debugger == c=continue s=step n=next bt=stack v=vars q=quit"

$showPrompt = $true
while (-not $proc.HasExited) {
    if ($stream.DataAvailable) {
        $msg = Recv
        if ($msg.type -eq "paused") {
            Write-Host "`nPAUSED $($msg.reason) | $($msg.file):$($msg.line)"
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
        'bt' {
            Send @{cmd="getStack"}
            $resp = Recv
            if ($resp -and $resp.frames) {
                $i = 0
                foreach ($f in $resp.frames) { Write-Host "  #$i $($f.name) $($f.file):$($f.line)"; $i++ }
            }
            $showPrompt = $true
        }
        'v' {
            Send @{cmd="getVars"}
            $resp = Recv
            if ($resp -and $resp.vars) {
                foreach ($p in $resp.vars.PSObject.Properties) { Write-Host "  $($p.Name) = $($p.Value)" }
            }
            $showPrompt = $true
        }
        'q' { Send @{cmd="continue"}; break }
        default { Write-Host "?"; $showPrompt = $true }
    }
}

$reader.Close(); $writer.Close(); $sock.Close()
if (-not $proc.HasExited) { $proc.Kill() }
Write-Host "Debug session ended"
