$scriptDir = "D:\CPLANG"
$vsRoot = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$vcvars = "$vsRoot\VC\Auxiliary\Build\vcvarsall.bat"

Write-Output "[Step 1] Setting VSROOT and running build_msvc.bat via cmd..."
Write-Output "[vcvars: $vcvars]"

# Run everything in the same cmd session
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "cmd.exe"
$psi.Arguments = "/c call `"$vcvars`" x64 && cd /d $scriptDir && call build_msvc.bat"
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$p = [System.Diagnostics.Process]::Start($psi)
$out = $p.StandardOutput.ReadToEnd()
$err = $p.StandardError.ReadToEnd()
$p.WaitForExit()

Write-Output $out
if ($err) { Write-Output "STDERR: $err" }
Write-Output "[Build exit code: $($p.ExitCode)]"
