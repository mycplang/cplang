Add-Type -AssemblyName System.Drawing

$iconDir = "D:\CPLANG\icon"
$files = @(
    "CPIDE图标1 [128x128].ico",
    "CPIDE图标2 [128x128].ico",
    "cp文件图标 [128x128].ico"
)

foreach ($name in $files) {
    $path = Join-Path $iconDir $name
    Write-Output "=== $name ==="
    
    try {
        $icon = [System.Drawing.Icon]::ExtractAssociatedIcon($path)
        if (-not $icon) {
            $icon = New-Object System.Drawing.Icon($path)
        }
        
        # Get the bitmap
        $bmp = $icon.ToBitmap()
        Write-Output "  Size: $($bmp.Width) x $($bmp.Height)"
        Write-Output "  Pixel format: $($bmp.PixelFormat)"
        
        # Sample some pixels to understand the color scheme
        $centerX = $bmp.Width / 2
        $centerY = $bmp.Height / 2
        
        $samples = @(
            @{x=0; y=0; desc="top-left"},
            @{x=[int]($bmp.Width-1); y=0; desc="top-right"},
            @{x=0; y=[int]($bmp.Height-1); desc="bottom-left"},
            @{x=[int]($bmp.Width-1); y=[int]($bmp.Height-1); desc="bottom-right"},
            @{x=[int]$centerX; y=[int]$centerY; desc="center"},
            @{x=[int]($centerX/2); y=[int]($centerY/2); desc="center-top-left quadrant"},
            @{x=[int]($centerX*1.5); y=[int]($centerY/2); desc="center-top-right quadrant"},
            @{x=[int]($centerX/2); y=[int]($centerY*1.5); desc="center-bottom-left quadrant"},
            @{x=[int]($centerX*1.5); y=[int]($centerY*1.5); desc="center-bottom-right quadrant"}
        )
        
        foreach ($s in $samples) {
            if ($s.x -lt $bmp.Width -and $s.y -lt $bmp.Height) {
                $c = $bmp.GetPixel($s.x, $s.y)
                Write-Output ("  Pixel " + $s.desc + ": R=$($c.R) G=$($c.G) B=$($c.B) A=$($c.A)")
            }
        }
        
        # Analyze dominant colors
        Write-Output "  --- Color analysis ---"
        $colorCounts = @{}
        for ($y = 0; $y -lt $bmp.Height; $y += 8) {
            for ($x = 0; $x -lt $bmp.Width; $x += 8) {
                $c = $bmp.GetPixel($x, $y)
                # Quantize to reduce noise
                $r = [math]::Floor($c.R / 32) * 32
                $g = [math]::Floor($c.G / 32) * 32
                $b = [math]::Floor($c.B / 32) * 32
                $key = "$r,$g,$b"
                if ($colorCounts.ContainsKey($key)) {
                    $colorCounts[$key]++
                } else {
                    $colorCounts[$key] = 1
                }
            }
        }
        $sorted = $colorCounts.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 8
        foreach ($entry in $sorted) {
            $rgb = $entry.Key -split ','
            Write-Output ("  Color: R=$($rgb[0]) G=$($rgb[1]) B=$($rgb[2]) - count=$($entry.Value)")
        }
        
        $bmp.Dispose()
        $icon.Dispose()
    } catch {
        Write-Output "  Error: $_"
    }
    Write-Output ""
}
