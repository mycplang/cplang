$dir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
Write-Output "=== Files in $dir ==="
Get-ChildItem $dir -Filter "mspdb*" -Name
Write-Output "`n=== Has cl.exe? ==="
Write-Output (Test-Path "$dir\cl.exe")
Write-Output "`n=== Has link.exe? ==="
Write-Output (Test-Path "$dir\link.exe")
