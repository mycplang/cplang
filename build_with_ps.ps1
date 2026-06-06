# Build cplang.exe using PowerShell's process execution (bypasses exec_shell's cmd issues)

$scriptDir = "D:\CPLANG"
$vsRoot = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$vcvars = "$vsRoot\VC\Auxiliary\Build\vcvarsall.bat"

Write-Output "[Step 1] Setting up MSVC environment..."

# Capture vcvars environment by running cmd /c and parsing 'set' output
$envVars = & cmd /c "call `"$vcvars`" x64 >nul 2>&1 && set"
foreach ($line in $envVars) {
    $idx = $line.IndexOf('=')
    if ($idx -gt 0) {
        $name = $line.Substring(0, $idx)
        $value = $line.Substring($idx + 1)
        [Environment]::SetEnvironmentVariable($name, $value, "Process")
    }
}

Write-Output "[Step 2] Testing cl.exe..."
$testResult = & "cmd.exe" "/c" "echo int main(){return 0;}>_test_msvc.c & cl.exe /nologo /c _test_msvc.c /Fo_test_msvc.obj & del _test_msvc.c _test_msvc.obj 2>nul & echo OK"
Write-Output "cl test: $testResult"

Write-Output "[Step 3] Running build_msvc.bat..."
$proc = Start-Process -NoNewWindow -FilePath "cmd.exe" -ArgumentList "/c", "cd /d $scriptDir && call build_msvc.bat" -Wait -PassThru
Write-Output "Build exit code: $($proc.ExitCode)"
