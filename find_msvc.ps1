Write-Output "=== VSWhere ==="
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -property installationPath
    Write-Output "VS Path: $vsPath"
    $vsVer = & $vswhere -latest -property catalog_productLineVersion
    Write-Output "VS Version: $vsVer"
} else {
    Write-Output "vswhere not found"
}

Write-Output "`n=== Checking MSVC ==="
$msvcDirs = Get-ChildItem "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC" -ErrorAction SilentlyContinue
if ($msvcDirs) {
    foreach ($d in $msvcDirs) {
        Write-Output "Found MSVC: $($d.FullName)"
        $cl = Get-ChildItem "$($d.FullName)\bin\Hostx64\x64\cl.exe" -ErrorAction SilentlyContinue
        if ($cl) {
            Write-Output "  cl.exe: $($cl.FullName)"
        } else {
            Write-Output "  cl.exe NOT found in bin\Hostx64\x64"
        }
    }
} else {
    Write-Output "No MSVC under standard path"
}

Write-Output "`n=== Checking vcvarsall ==="
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
if (Test-Path $vcvars) {
    Write-Output "vcvarsall found at: $vcvars"
} else {
    Write-Output "vcvarsall NOT found at: $vcvars"
}

Write-Output "`n=== Environment LIB ==="
$lib = [Environment]::GetEnvironmentVariable("LIB","Process")
Write-Output "LIB=$lib"

Write-Output "`n=== Environment INCLUDE ==="
$inc = [Environment]::GetEnvironmentVariable("INCLUDE","Process")
Write-Output "INCLUDE=$inc"

Write-Output "`n=== Environment PATH (vcvars related) ==="
$path = [Environment]::GetEnvironmentVariable("PATH","Process")
$path.Split(';') | Where-Object { $_ -match 'VC|MSVC|Visual Studio|Windows Kits' } | ForEach-Object { Write-Output $_ }

Write-Output "`n=== Checking C:\cplang junction ==="
$item = Get-Item "C:\cplang" -ErrorAction SilentlyContinue
if ($item) {
    Write-Output "C:\cplang exists: $($item.Attributes)"
} else {
    Write-Output "C:\cplang not found"
}
