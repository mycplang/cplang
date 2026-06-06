$dir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
$files = @("mspdbcore.dll", "mspdb140.dll", "mspdbst.dll")
foreach ($f in $files) {
    $path = Join-Path $dir $f
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        Write-Output "OK: $f ($size bytes)"
    } else {
        Write-Output "MISSING: $f"
    }
}
Write-Output "`n=== Also checking Hostx86\x64 versions ==="
$dir86 = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x64"
foreach ($f in $files) {
    $path = Join-Path $dir86 $f
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        Write-Output "OK: $f ($size bytes)"
    } else {
        Write-Output "MISSING: $f"
    }
}
Write-Output "`n=== Trying to run cl.exe /? ==="
$cl = Join-Path $dir "cl.exe"
$p = Start-Process -NoNewWindow -FilePath $cl -ArgumentList "/?" -RedirectStandardOutput "$env:TEMP\cl_help.txt" -RedirectStandardError "$env:TEMP\cl_err.txt" -Wait -PassThru
Write-Output "cl.exe /? exit code: $($p.ExitCode)"
if ($p.ExitCode -eq 0) {
    Write-Output "cl.exe works!"
    Get-Content "$env:TEMP\cl_help.txt" -TotalCount 5
} else {
    Write-Output "cl.exe failed"
    Get-Content "$env:TEMP\cl_err.txt" -TotalCount 5
}
Remove-Item "$env:TEMP\cl_help.txt","$env:TEMP\cl_err.txt" -Force -ErrorAction SilentlyContinue
