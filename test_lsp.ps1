$node = "C:\Program Files\nodejs\node.exe"
$server = "D:\CPLANG\tools\vscode-cp\cplsp.js"

$init = @{
    jsonrpc = "2.0"
    id = 1
    method = "initialize"
    params = @{
        capabilities = @{}
        processId = $null
        rootUri = $null
    }
}
$body = ($init | ConvertTo-Json -Compress -Depth 10)
$header = "Content-Length: $([System.Text.Encoding]::UTF8.GetByteCount($body))`r`n`r`n"

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $node
$psi.Arguments = $server
$psi.UseShellExecute = $false
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true

$p = [System.Diagnostics.Process]::Start($psi)
$p.StandardInput.Write($header + $body)
$p.StandardInput.Close()

$output = $p.StandardOutput.ReadToEnd()
$err = $p.StandardError.ReadToEnd()
$p.WaitForExit(3000)

if ($output) { Write-Output "STDOUT: $output" }
if ($err) { Write-Output "STDERR: $err" }
Write-Output "Exit: $($p.ExitCode)"
