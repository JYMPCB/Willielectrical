param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("configure", "build", "run", "start", "reconfigure")]
    [string]$Action
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$simDir = Join-Path $repoRoot "lv_port_pc_vscode"
$buildDir = Join-Path $simDir "build_vs"

$cmakeFromEsp = "C:/Users/JYM-DSN/.espressif/tools/cmake/3.30.2/bin/cmake.exe"
$vcpkgToolchain = Join-Path $repoRoot "vcpkg\scripts\buildsystems\vcpkg.cmake"
$vcpkgTriplet = "x64-windows"

if (Test-Path $cmakeFromEsp) {
    $cmakeExe = $cmakeFromEsp
} else {
    $cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmakeCmd) {
        throw "No se encontró CMake. Instalá CMake o habilitalo en PATH."
    }
    $cmakeExe = $cmakeCmd.Source
}

if (-not (Test-Path $simDir)) {
    throw "No se encontró la carpeta del simulador: $simDir"
}

if (-not (Test-Path $vcpkgToolchain)) {
    throw "No se encontró el toolchain de vcpkg: $vcpkgToolchain"
}

Set-Location $simDir
Write-Host "LVGL_SIM action=$Action"
Write-Host "simDir=$simDir"
Write-Host "buildDir=$buildDir"
Write-Host "cmake=$cmakeExe"
Write-Host "vcpkgToolchain=$vcpkgToolchain"

function Invoke-Configure {
    & $cmakeExe `
        -S $simDir `
        -B $buildDir `
        -G "Visual Studio 17 2022" `
        -A x64 `
        -DCMAKE_TOOLCHAIN_FILE="$vcpkgToolchain" `
        -DVCPKG_TARGET_TRIPLET="$vcpkgTriplet"

    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE"
    }
}

function Remove-BuildCache {
    $cacheFile = Join-Path $buildDir "CMakeCache.txt"
    $cmakeFilesDir = Join-Path $buildDir "CMakeFiles"

    if (Test-Path $cacheFile) {
        Remove-Item $cacheFile -Force
    }

    if (Test-Path $cmakeFilesDir) {
        Remove-Item $cmakeFilesDir -Recurse -Force
    }
}

function Ensure-Configured {
    $cacheFile = Join-Path $buildDir "CMakeCache.txt"
    if (-not (Test-Path $cacheFile)) {
        Invoke-Configure
    }
}

function Invoke-Build {
    Ensure-Configured
    & $cmakeExe --build $buildDir --config Debug --target main -j 6
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed with exit code $LASTEXITCODE"
    }
}

function Get-ExePath {
    $candidates = @(
        (Join-Path $simDir "bin\Debug\main.exe"),
        (Join-Path $simDir "bin\main.exe"),
        (Join-Path $buildDir "Debug\main.exe"),
        (Join-Path $buildDir "main.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

switch ($Action) {
    "configure" {
        Invoke-Configure
        break
    }

    "reconfigure" {
        Remove-BuildCache
        Invoke-Configure
        break
    }

    "build" {
        Invoke-Build
        break
    }

    "run" {
        $exe = Get-ExePath
        if (-not $exe) {
            throw "No se encontró el ejecutable. Ejecutá primero configure y build."
        }
        & $exe
        break
    }

    "start" {
        Ensure-Configured
        Invoke-Build
        $exe = Get-ExePath
        if (-not $exe) {
            throw "Build finalizado pero no se encontró el ejecutable."
        }
        & $exe
        break
    }
}