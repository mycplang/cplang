Add-Type -AssemblyName System.Drawing

$iconDir = "D:\CPLANG\icon"
$files = Get-ChildItem $iconDir -Filter "*128*.ico" | Sort-Object Name

foreach ($f in $files) {
    Write-Output "=== $($f.Name) ==="
    
    try {
        $icon = New-Object System.Drawing.Icon($f.FullName)
        $bmp = $icon.ToBitmap()
        Write-Output "  Size: $($bmp.Width) x $($bmp.Height)"
        
        $w = $bmp.Width; $h = $bmp.Height
        $cx = [int]($w / 2); $cy = [int]($h / 2)
        
        # Center + corner pixels
        $pts = @()
        $pts += @([int]($w/4), [int]($h/4), "upper-left")
        $pts += @([int]($w*3/4), [int]($h/4), "upper-right")
        $pts += @([int]($w/4), [int]($h*3/4), "lower-left")
        $pts += @([int]($w*3/4), [int]($h*3/4), "lower-right")
        $pts += @($cx, $cy, "center")
        
        Write-Output "  --- Key pixels ---"
        for ($i = 0; $i -lt $pts.Count; $i += 3) {
            $c = $bmp.GetPixel($pts[$i], $pts[$i+1])
            Write-Output ("  $($pts[$i+2]) (x=$($pts[$i]),y=$($pts[$i+1])): R=$($c.R) G=$($c.G) B=$($c.B) A=$($c.A)")
        }
        
        # Shape scan at center row
        Write-Output "  --- Shape scan ---"
        $leftEdge = -1; $rightEdge = -1
        for ($x = 0; $x -lt $w; $x++) {
            $c = $bmp.GetPixel($x, $cy)
            if ($c.A -gt 128 -and $leftEdge -eq -1) { $leftEdge = $x }
            if ($c.A -gt 128) { $rightEdge = $x }
        }
        if ($leftEdge -ge 0) {
            Write-Output "  Object x=$leftEdge to $rightEdge (w=$($rightEdge-$leftEdge+1))"
            $topEdge = -1; $bottomEdge = -1
            for ($y = 0; $y -lt $h; $y++) {
                $c = $bmp.GetPixel($cx, $y)
                if ($c.A -gt 128 -and $topEdge -eq -1) { $topEdge = $y }
                if ($c.A -gt 128) { $bottomEdge = $y }
            }
            Write-Output "  Object y=$topEdge to $bottomEdge (h=$($bottomEdge-$topEdge+1))"
        }
        
        # Dominant colors (opaque only)
        Write-Output "  --- Colors (opaque) ---"
        $colors = @{}
        for ($y = 0; $y -lt $h; $y += 6) {
            for ($x = 0; $x -lt $w; $x += 6) {
                $c = $bmp.GetPixel($x, $y)
                if ($c.A -gt 0) {
                    $r = [math]::Floor($c.R / 48) * 48
                    $g = [math]::Floor($c.G / 48) * 48
                    $b = [math]::Floor($c.B / 48) * 48
                    $key = "$r,$g,$b"
                    if ($colors.ContainsKey($key)) { $colors[$key]++ } else { $colors[$key] = 1 }
                }
            }
        }
        $sorted = $colors.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 6
        foreach ($e in $sorted) {
            $rgb = $e.Key -split ','
            Write-Output ("  [$($e.Value)px] RGB($($rgb[0]),$($rgb[1]),$($rgb[2]))")
        }
        
        $bmp.Dispose(); $icon.Dispose()
    } catch {
        Write-Output "  Error: $_"
    }
    Write-Output ""
}
