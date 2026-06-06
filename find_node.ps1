$paths = @(
    "C:\Program Files\nodejs\node.exe",
    "C:\Program Files (x86)\nodejs\node.exe",
    "${env:APPDATA}\npm\node.exe",
    "${env:ProgramFiles}\nodejs\node.exe",
    "${env:ProgramFiles(x86)}\nodejs\node.exe"
)
foreach ($p in $paths) {
    if (Test-Path $p) {
        Write-Output "FOUND: $p"
    }
}
Write-Output "---"
Write-Output "PATH check:"
$env:Path -split ';' | Where-Object { $_ -match 'node' } | ForEach-Object { Write-Output $_ }
