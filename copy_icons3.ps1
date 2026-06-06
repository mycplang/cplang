$iconDir = "D:\CPLANG\icon"

# Find by exact prefixes
$files = Get-ChildItem $iconDir
$pat1 = $files | Where-Object { $_.Name.StartsWith("CPIDE图标1") -and $_.Name.Contains("256") }
$pat2 = $files | Where-Object { $_.Name.StartsWith("cp") -and $_.Name.Contains("256") }

if ($pat1) {
    Copy-Item $pat1[0].FullName (Join-Path $iconDir "app.ico") -Force
    Write-Output "Copied $($pat1[0].Name) -> app.ico"
}
if ($pat2) {
    Copy-Item $pat2[0].FullName (Join-Path $iconDir "cpfile.ico") -Force
    Write-Output "Copied $($pat2[0].Name) -> cpfile.ico"
}

$test1 = Test-Path (Join-Path $iconDir "app.ico")
$test2 = Test-Path (Join-Path $iconDir "cpfile.ico")
Write-Output "app.ico: $test1  cpfile.ico: $test2"
if (-not $test1 -or -not $test2) { exit 1 }
