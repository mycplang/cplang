$p = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe"
Write-Output "link.exe exists: $(Test-Path $p)"
Write-Output "size: $((Get-Item $p -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Length))"
