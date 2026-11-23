$ErrorActionPreference = 'Stop'

if (-not (Get-Command adb -ErrorAction SilentlyContinue)) {
    Write-Host '[LOI] Khong tim thay adb. Cai Android platform-tools va them adb vao PATH.'
    exit 1
}

$command = $args -join ' '
if ([string]::IsNullOrEmpty($command)) {
    $command = 'id'
}

$target = $null
foreach ($line in (& adb devices)) {
    $parts = $line.Trim() -split '\s+'
    if ($parts.Count -lt 2 -or $parts[1] -ne 'device') {
        continue
    }
    $product = (& adb -s $parts[0] shell getprop ro.product.device).Trim()
    if ($product -eq 'coral') {
        $target = $parts[0]
        break
    }
}

if (-not $target) {
    Write-Host '[LOI] Khong tim thay Pixel 4 XL (coral) dang bat ADB.'
    exit 1
}

$bytes = [Text.Encoding]::UTF8.GetBytes($command)
$hex = ($bytes | ForEach-Object { $_.ToString('x2') }) -join ''
& adb -s $target shell /data/local/tmp/coral_root su --hex $hex
exit $LASTEXITCODE
