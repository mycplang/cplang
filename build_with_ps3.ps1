$scriptDir = "D:\CPLANG"
$vsRoot = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$vcvars = "$vsRoot\VC\Auxiliary\Build\vcvarsall.bat"

Write-Output "[Running: vcvarsall + build_msvc.bat]"

$p = Start-Process -NoNewWindow -FilePath "cmd.exe" -ArgumentList "/c", "call", "`"$vcvars`"", "x64", "&&", "cd", "/d", $scriptDir, "&&", "call", "build_msvc.bat" -Wait -PassThru
Write-Output "[Build exit code: $($p.ExitCode)]"
