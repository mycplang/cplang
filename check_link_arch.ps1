$msvc = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin"
$paths = @(
    "Hostx64\x86\link.exe",
    "Hostx64\x64\link.exe",
    "Hostx86\x64\link.exe"
)
foreach ($p in $paths) {
    $full = Join-Path $msvc $p
    if (Test-Path $full) {
        $bytes = [System.IO.File]::ReadAllBytes($full)
        $e_lfanew = [BitConverter]::ToUInt32($bytes, 0x3C)
        $machine = [BitConverter]::ToUInt16($bytes, $e_lfanew + 4)
        if ($machine -eq 0x8664) { $arch = "x64" }
        elseif ($machine -eq 0x014C) { $arch = "x86" }
        else { $arch = "unknown (0x$($machine.ToString('X4')))" }
        Write-Output "$p : $arch ($((Get-Item $full).Length) bytes)"
    }
}
