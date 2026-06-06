$scriptDir = "D:\CPLANG"
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

# Source vcvarsall and capture env
$envSnapshot = & {
    cmd /c "call `"$vcvars`" x64 >nul 2>&1 && set"
}

# Parse env output
foreach ($line in $envSnapshot) {
    $parts = $line.Split('=', 2)
    if ($parts.Count -eq 2) {
        [Environment]::SetEnvironmentVariable($parts[0], $parts[1], "Process")
    }
}

# Now run the build
Set-Location $scriptDir
$proc = Start-Process -NoNewWindow -FilePath "cmd.exe" -ArgumentList "/c build_msvc.bat" -Wait -PassThru
Write-Output "Build exit code: $($proc.ExitCode)"
