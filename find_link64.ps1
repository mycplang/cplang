$msvc = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
Get-ChildItem -Path "$msvc\bin" -Recurse -Filter "link.exe" | Select-Object FullName, Length | ForEach-Object {
    Write-Output "$($_.FullName) ($($_.Length) bytes)"
}

Write-Output "---"
Write-Output "Also checking all .exe in Hostx64\x64:"
$host64 = "$msvc\bin\Hostx64\x64"
Get-ChildItem $host64 -Filter "*.exe" | ForEach-Object { Write-Output "  $($_.Name) ($($_.Length) bytes)" }
