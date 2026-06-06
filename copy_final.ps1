$iconDir = "D:\CPLANG\icon"
$files = Get-ChildItem -LiteralPath $iconDir

Write-Output "Files in icon dir:"
$files | ForEach-Object { Write-Output "  [$($files.IndexOf($_))] $($_.Name) ($($_.Length) bytes)" }

# CPIDE图标1 [256x256].ico - should be index 1 (sorted: 128 then 256)
$appSrc = $files | Where-Object { $_.Name -like "*CPIDE*1*256*" } | Select-Object -First 1
$cpSrc = $files | Where-Object { $_.Name -like "*cp*256*" -and $_.Name -notlike "*CPIDE*" } | Select-Object -First 1

if (-not $cpSrc) {
    # Fallback: just get any file that starts with "cp" and has 256
    $cpSrc = $files | Where-Object { $_.Name -like "cp*256*" } | Select-Object -First 1
}

Write-Output "app source: $($appSrc.Name)"
Write-Output "cp source: $($cpSrc.Name)"

Copy-Item -LiteralPath $appSrc.FullName -Destination (Join-Path $iconDir "app.ico") -Force
Copy-Item -LiteralPath $cpSrc.FullName -Destination (Join-Path $iconDir "cpfile.ico") -Force

$r1 = Get-Item (Join-Path $iconDir "app.ico") -ErrorAction SilentlyContinue
$r2 = Get-Item (Join-Path $iconDir "cpfile.ico") -ErrorAction SilentlyContinue
Write-Output "app.ico: $($r1.Length) bytes"
Write-Output "cpfile.ico: $($r2.Length) bytes"
if ($r1.Length -gt 10000 -and $r2.Length -gt 10000) { Write-Output "OK" } else { Write-Output "FAILED"; exit 1 }
