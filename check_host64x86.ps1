$dir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x86"
Write-Output "Files in Hostx64\x86:"
Get-ChildItem $dir -Filter "*.dll" | Select-Object Name, Length
Get-ChildItem $dir -Filter "*.exe" | Select-Object Name, Length
