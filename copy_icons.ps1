$iconDir = "D:\CPLANG\icon"
Copy-Item (Join-Path $iconDir "CPIDE图标1 [256x256].ico") (Join-Path $iconDir "app.ico") -Force
Copy-Item (Join-Path $iconDir "cp文件图标[256x256].ico") (Join-Path $iconDir "cpfile.ico") -Force
Write-Output "Copied app.ico: $(Test-Path (Join-Path $iconDir 'app.ico'))"
Write-Output "Copied cpfile.ico: $(Test-Path (Join-Path $iconDir 'cpfile.ico'))"
