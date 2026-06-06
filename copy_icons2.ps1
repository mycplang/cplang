$iconDir = "D:\CPLANG\icon"
$files = Get-ChildItem $iconDir -Filter "*.ico"
foreach ($f in $files) {
    Write-Output "$($f.Name)"
}
Write-Output "---"
# Find files by pattern
$pat1 = Get-ChildItem $iconDir | Where-Object { $_.Name -like "*CPIDE*1*256*" }
if ($pat1) {
    Copy-Item $pat1[0].FullName (Join-Path $iconDir "app.ico") -Force
    Write-Output "Copied $($pat1[0].Name) -> app.ico"
}
$pat2 = Get-ChildItem $iconDir | Where-Object { $_.Name -like "*cp*256*" }
if ($pat2) {
    Copy-Item $pat2[0].FullName (Join-Path $iconDir "cpfile.ico") -Force
    Write-Output "Copied $($pat2[0].Name) -> cpfile.ico"
}
Write-Output "app.ico exists: $(Test-Path (Join-Path $iconDir 'app.ico'))"
Write-Output "cpfile.ico exists: $(Test-Path (Join-Path $iconDir 'cpfile.ico'))"
