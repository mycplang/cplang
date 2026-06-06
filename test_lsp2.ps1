$node = "C:\Program Files\nodejs\node.exe"
$server = "D:\CPLANG\tools\vscode-cp\cplsp.js"

function Send-LspMessage($proc, $msg) {
    $body = ($msg | ConvertTo-Json -Compress -Depth 10)
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($body)
    $header = "Content-Length: $($bytes.Length)`r`n`r`n"
    $proc.StandardInput.Write($header)
    $proc.StandardInput.Flush()
    Start-Sleep -Milliseconds 200
}

function Read-LspResponse($proc, $timeoutMs = 1000) {
    $start = Get-Date
    $buf = ""
    while ((Get-Date) -lt $start.AddMilliseconds($timeoutMs)) {
        if ($proc.StandardOutput.Peek() -ge 0) {
            $buf += $proc.StandardOutput.ReadToEnd()
        }
        if ($buf -match "Content-Length: (\d+)\r\n\r\n(.*)") {
            $len = [int]$Matches[1]
            $json = $Matches[2]
            return $json
        }
        Start-Sleep -Milliseconds 100
    }
    return $buf
}

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $node; $psi.Arguments = $server
$psi.UseShellExecute = $false
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$p = [System.Diagnostics.Process]::Start($psi)

# Send initialize
Send-LspMessage $p @{jsonrpc="2.0"; id=1; method="initialize"; params=@{}}
Start-Sleep 1

# Read response
$resp = Read-LspResponse $p
Write-Output "=== Initialize Response ==="
Write-Output $resp

# Send initialized notification
Send-LspMessage $p @{jsonrpc="2.0"; method="initialized"; params=@{}}

# Send shutdown
Send-LspMessage $p @{jsonrpc="2.0"; id=2; method="shutdown"; params=@{}}
Start-Sleep 0.5
$resp2 = Read-LspResponse $p
Write-Output "=== Shutdown Response ==="
Write-Output $resp2

# Send exit
Send-LspMessage $p @{jsonrpc="2.0"; method="exit"; params=@{}}
$p.WaitForExit(2000)
Write-Output "Exit code: $($p.ExitCode)"
