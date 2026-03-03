param(
    [string]$Version,
    [string]$RepoOwner = "jympcb",
    [string]$RepoName = "Willielectrical",
    [string]$AssetName = "will.bin",
    [string]$Notes = "Auto-generated OTA manifest",
    [ValidateSet("pages", "release")]
    [string]$Delivery = "pages"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$appGlobals = Join-Path $root "components/app/src/app_globals.cpp"
$builtBin = Join-Path $root "build/Will.bin"
$pagesBin = Join-Path $root "ota/will.bin"
$manifestPaths = @(
    (Join-Path $root "ota/latest.json"),
    (Join-Path $root "docs/ota/latest.json")
)

if (-not $Version -or $Version.Trim().Length -eq 0) {
    if (-not (Test-Path $appGlobals)) {
        throw "No se encontró app_globals.cpp en: $appGlobals"
    }

    $content = Get-Content -Path $appGlobals -Raw
    $match = [regex]::Match($content, '#define\s+FW_VERSION\s+"([0-9]+\.[0-9]+\.[0-9]+)"')
    if (-not $match.Success) {
        throw "No pude extraer FW_VERSION desde $appGlobals"
    }
    $Version = $match.Groups[1].Value
}

if ($Delivery -eq "release") {
    $binUrl = "https://github.com/$RepoOwner/$RepoName/releases/download/v$Version/$AssetName"
}
else {
    if (Test-Path $builtBin) {
        Copy-Item -Path $builtBin -Destination $pagesBin -Force
        Write-Host "Copiado bin OTA: $pagesBin"
    } else {
        Write-Warning "No se encontró $builtBin. Se mantiene/espera ota/will.bin existente."
    }

    $binUrl = "https://$RepoOwner.github.io/$RepoName/ota/$AssetName"
}

$manifestObj = [ordered]@{
    version = $Version
    bin_url = $binUrl
    notes = $Notes
}

$json = $manifestObj | ConvertTo-Json -Depth 3

foreach ($manifest in $manifestPaths) {
    $dir = Split-Path -Parent $manifest
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir | Out-Null
    }

    Set-Content -Path $manifest -Value $json -Encoding UTF8
    Write-Host "Actualizado: $manifest"
}

Write-Host "Version: $Version"
Write-Host "bin_url: $binUrl"
