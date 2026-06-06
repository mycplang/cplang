$destDir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
New-Item -ItemType Directory -Force -Path $destDir | Out-Null
Copy-Item "D:\CPLANG\llvm-dev\bin\lld-link.exe" "$destDir\link.exe" -Force
if (Test-Path "$destDir\link.exe") {
    Write-Output "OK: link.exe created"
} else {
    Write-Output "FAILED"
}
