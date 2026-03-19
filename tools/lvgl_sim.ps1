param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("configure", "build", "run", "start")]
    [string]$Action
)

$ErrorActionPreference = "Continue"

$repoRoot = Split-Path -Parent $PSScriptRoot
$simDir = Join-Path $repoRoot "lv_port_pc_vscode"
$cmakeFromEsp = "C:/Users/JYM-DSN/.espressif/tools/cmake/3.30.2/bin/cmake.exe"
$buildDir = "build_vs"

if (Test-Path $cmakeFromEsp) {
    $cmakeExe = $cmakeFromEsp
} else {
    $cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmakeCmd) {
        throw "No se encontró CMake. Instalá CMake o habilitalo en PATH."
    }
    $cmakeExe = $cmakeCmd.Source
}

Set-Location $simDir
Write-Host "LVGL_SIM action=$Action buildDir=$buildDir"

function Invoke-Configure {
    & $cmakeExe -S . -B $buildDir -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE"
    }
}

function Ensure-Configured {
    $cacheFile = Join-Path $simDir "$buildDir/CMakeCache.txt"
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
        (Join-Path $simDir "bin/Debug/main.exe"),
        (Join-Path $simDir "bin/main.exe"),
        (Join-Path $simDir "$buildDir/Debug/main.exe"),
        (Join-Path $simDir "$buildDir/main.exe")
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
    "build" {
        Invoke-Build
        break
    }
    "run" {
        $exe = Get-ExePath
        if (-not $exe) {
            throw "No se encontró el ejecutable (bin/main.exe ni bin/Debug/main.exe). Ejecutá primero configure y build."
        }
        & $exe
        break
    }
    "start" {
        Invoke-Configure
        Invoke-Build
        $exe = Get-ExePath
        if (-not $exe) {
            throw "Build finalizado pero no se encontró el ejecutable (bin/main.exe ni bin/Debug/main.exe)."
        }
        & $exe
        break
    }
}
