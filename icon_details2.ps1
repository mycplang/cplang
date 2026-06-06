Add-Type -AssemblyName System.Drawing

$iconDir = "D:\CPLANG\icon"
$files = Get-ChildItem $iconDir -Filter "*128*.ico" | Sort-Object Name

foreach ($f in $files) {
    Write-Output "=== $($f.Name) ==="
    
    try {
        $icon = New-Object System.Drawing.Icon($f.FullName)
        $bmp = $icon.ToBitmap()
        Write-Output "  Size: $($bmp.Width) x $($bmp.Height)"
        
        # Center + corner pixels
        $w = $bmp.Width; $h = $bmp.Height
        $cx = [int]($w/2); $cy = [int]($h/2)
        $pts = @(
            @($w/4, $h/4, "upper-left"), @($w*3/4, $h/4, "upper-right"),
            @($w/4, $h*3/4, "lower-left"), @($w*3/4, $h*3/4, "lower-right"),
            @($cx, $cy, "center")
        )
        foreach ($p in $pts) {
            $c = $bmp.GetPixel([int]$p[0], [int]$p[1])
            Write-Output ("  $($p[2]) (x=$([int]$p[0]),y=$([int]$p[1])): R=$($c.R) G=$($c.G) B=$($c.B)")
        }
        
        # Detect shapes by scanning horizontal lines at 25%, 50%, 75%
        Write-Output "  --- Shape scan (center row y=$cy) ---"
        $leftEdge = -1; $rightEdge = -1
        for ($x = 0; $x -lt $w; $x++) {
            $c = $bmp.GetPixel($x, $cy)
            if ($c.A -gt 128 -and $leftEdge -eq -1) { $leftEdge = $x }
            if ($c.A -gt 128) { $rightEdge = $x }
        }
        if ($leftEdge -ge 0) {
            Write-Output "  Object spans x=$leftEdge to x=$rightEdge (width=$($rightEdge-$leftEdge+1))"
            
            # Top and bottom edges
            $topEdge = -1; $bottomEdge = -1
            for ($y = 0; $y -lt $h; $y++) {
                $c = $bmp.GetPixel($cx, $y)
                if ($c.A -gt 128 -and $topEdge -eq -1) { $topEdge = $y }
                if ($c.A -gt 128) { $bottomEdge = $y }
            }
            Write-Output "  Object spans y=$topEdge to y=$bottomEdge (height=$($bottomEdge-$topEdge+1))"
        }
        
        # Dominant colors
        Write-Output "  --- Colors ---"
        $colors = @{}
        for ($y = 0; $y -lt $h; $y += 6) {
            for ($x = 0; $x -lt $w; $x += 6) {
                $c = $bmp.GetPixel($x, $y)
                if ($c.A -gt 0) {
                    $r = [math]::Floor($c.R / 48) * 48
                    $g = [math]::Floor($c.G / 48) * 48
                    $b = [math]::Floor($c.B / 48) * 48
                    $key = "$r,$g,$b"
                    $colors[$key] = if ($colors.ContainsKey($key)) { $colors[$key] + 1 } else { 1 }
                }
            }
        }
        $sorted = $colors.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 6
        foreach ($e in $sorted) {
            $rgb = $e.Key -split ','
            Write-Output ("  [$($e.Value)px] R=$($rgb[0]) G=$($rgb[1]) B=$($rgb[2])")
        }
        
        $bmp.Dispose(); $icon.Dispose()
    } catch {
        Write-Output "  Error: $_"
    }
    Write-Output ""
}
