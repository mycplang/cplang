Add-Type -AssemblyName System.Drawing

$iconDir = "D:\CPLANG\icon"
$files = Get-ChildItem $iconDir -Filter "*128*.ico" | Sort-Object Name

foreach ($f in $files) {
    Write-Output "=== $($f.Name) ==="
    $icon = New-Object System.Drawing.Icon($f.FullName)
    $bmp = $icon.ToBitmap()
    $w = $bmp.Width; $h = $bmp.Height
    
    # Find background color (most common pixel at edges)
    $edgePixels = @{}
    for ($x = 0; $x -lt $w; $x++) {
        $c = $bmp.GetPixel($x, 0)
        $key = "$($c.R),$($c.G),$($c.B)"
        if ($edgePixels.ContainsKey($key)) { $edgePixels[$key]++ } else { $edgePixels[$key] = 1 }
        $c = $bmp.GetPixel($x, $h-1)
        $key = "$($c.R),$($c.G),$($c.B)"
        if ($edgePixels.ContainsKey($key)) { $edgePixels[$key]++ } else { $edgePixels[$key] = 1 }
    }
    $bgKey = ($edgePixels.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 1).Key
    $bgParts = $bgKey -split ','
    $bgR = [int]$bgParts[0]; $bgG = [int]$bgParts[1]; $bgB = [int]$bgParts[2]
    Write-Output "  Background: RGB($bgR,$bgG,$bgB)"
    
    # Find foreground bounding box (pixels different from background)
    $minX = $w; $maxX = 0; $minY = $h; $maxY = 0
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $c = $bmp.GetPixel($x, $y)
            $dr = [math]::Abs($c.R - $bgR); $dg = [math]::Abs($c.G - $bgG); $db = [math]::Abs($c.B - $bgB)
            if ($dr -gt 10 -or $dg -gt 10 -or $db -gt 10) {
                if ($x -lt $minX) { $minX = $x }
                if ($x -gt $maxX) { $maxX = $x }
                if ($y -lt $minY) { $minY = $y }
                if ($y -gt $maxY) { $maxY = $y }
            }
        }
    }
    Write-Output "  Shape bounds: ($minX,$minY) to ($maxX,$maxY)  size=$($maxX-$minX+1)x$($maxY-$minY+1)"
    
    # Scan center horizontal line for gaps (identifying multi-part shapes)
    $cyText = [int]($h/2)
    $segments = @()
    $inShape = $false; $segStart = 0
    for ($x = 0; $x -lt $w; $x++) {
        $c = $bmp.GetPixel($x, $cyText)
        $dr = [math]::Abs($c.R - $bgR); $dg = [math]::Abs($c.G - $bgG); $db = [math]::Abs($c.B - $bgB)
        $isShape = ($dr -gt 10 -or $dg -gt 10 -or $db -gt 10)
        if ($isShape -and -not $inShape) { $inShape = $true; $segStart = $x }
        if (-not $isShape -and $inShape) {
            $segments += @{start=$segStart; end=$($x-1); w=$($x-$segStart)}
            $inShape = $false
        }
    }
    if ($inShape) { $segments += @{start=$segStart; end=$($w-1); w=$($w-$segStart)} }
    
    if ($segments.Count -gt 1) {
        Write-Output "  Segments at mid-y=$cyText :"
        foreach ($seg in $segments) {
            Write-Output "    x=$($seg.start)-$($seg.end) (w=$($seg.w))"
        }
    } else {
        Write-Output "  Single shape at mid-y: x=$($segments[0].start)-$($segments[0].end)"
    }
    
    # For cp file icon, also check for document shape
    if ($f.Name -match "cp") {
        Write-Output "  --- Cp icon detail ---"
        # Check top-left corner area for folded corner
        $foldPixels = 0
        for ($y = 0; $y -lt 30; $y++) {
            for ($x = $w-30; $x -lt $w; $x++) {
                $c = $bmp.GetPixel($x, $y)
                $dr = [math]::Abs($c.R - $bgR); $dg = [math]::Abs($c.G - $bgG); $db = [math]::Abs($c.B - $bgB)
                if ($dr -gt 10 -or $dg -gt 10 -or $db -gt 10) { $foldPixels++ }
            }
        }
        Write-Output "  Top-right corner (30x30) non-bg pixels: $foldPixels / 900"
    }
    
    $bmp.Dispose(); $icon.Dispose()
    Write-Output ""
}
