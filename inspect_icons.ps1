$iconDir = "D:\CPLANG\icon"
$files = Get-ChildItem $iconDir -Filter "*.ico"

foreach ($f in $files) {
    Write-Output "=== $($f.Name) ==="
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    
    # ICO header: reserved(2), type(2), count(2)
    $count = [System.BitConverter]::ToUInt16($bytes, 4)
    Write-Output "Images in file: $count"
    
    for ($i = 0; $i -lt $count; $i++) {
        $offset = 6 + $i * 16
        $w = $bytes[$offset]
        $h = $bytes[$offset + 1]
        $colors = $bytes[$offset + 2]
        $bpp = [System.BitConverter]::ToUInt16($bytes, $offset + 6)
        $size = [System.BitConverter]::ToUInt32($bytes, $offset + 8)
        
        if ($w -eq 0) { $w = 256 }
        if ($h -eq 0) { $h = 256 }
        
        Write-Output "  Image $i : ${w}x${h}, bpp=$bpp, size=$size bytes"
    }
    
    # Try to check if it's PNG-compressed or BMP
    $firstEntryOffset = [System.BitConverter]::ToUInt32($bytes, 6 + 12)
    $magic = [System.Text.Encoding]::ASCII.GetString($bytes, $firstEntryOffset, [Math]::Min(4, $bytes.Length - $firstEntryOffset))
    Write-Output "  Entry data starts with: $magic"
    Write-Output ""
}
