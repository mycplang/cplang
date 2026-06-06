$ErrorActionPreference = "Stop"
$scriptDir = "D:\CPLANG"
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

# Step 1: set up MSVC environment in the same cmd session, run build
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "cmd.exe"
$psi.Arguments = "/c call `"$vcvars`" x64 && cd /d $scriptDir && call build_msvc.bat && exit !ERRORLEVEL!"
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $true

$p = New-Object System.Diagnostics.Process
$p.StartInfo = $psi

# Read streams concurrently to avoid deadlock
$outBuilder = New-Object System.Text.StringBuilder
$errBuilder = New-Object System.Text.StringBuilder

$outEvent = Register-ObjectEvent -InputObject $p -EventName OutputDataReceived -Action {
    $event.MessageData.AppendLine($Event.SourceEventArgs.Data)
} -MessageData $outBuilder

$errEvent = Register-ObjectEvent -InputObject $p -EventName ErrorDataReceived -Action {
    $event.MessageData.AppendLine($Event.SourceEventArgs.Data)
} -MessageData $errBuilder

$null = $p.Start()
$p.BeginOutputReadLine()
$p.BeginErrorReadLine()
$p.WaitForExit()

Unregister-Event -SourceIdentifier $outEvent.Name -ErrorAction SilentlyContinue
Unregister-Event -SourceIdentifier $errEvent.Name -ErrorAction SilentlyContinue

# Output results
$stdout = $outBuilder.ToString().Trim()
$stderr = $errBuilder.ToString().Trim()
if ($stdout) { Write-Output $stdout }
if ($stderr) { Write-Output "STDERR: $stderr" }
Write-Output "BUILD_EXIT_CODE=$($p.ExitCode)"
