# Prisma Engine Windows Build Script (PowerShell)
# Usage: .\build-windows.ps1 -Preset <preset> [-Clean]
#
# Available presets:
#   - windows-x64-debug (default)
#   - windows-x64-release
#   - windows-x86-debug
#   - windows-x86-release

param(
    [Parameter(Position=0)]
    [ValidateSet("windows-x64-debug", "windows-x64-release", "windows-x86-debug", "windows-x86-release")]
    [string]$Preset = "windows-x64-debug",

    [switch]$Clean
)

$ErrorActionPreference = "Stop"

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "====================================" -ForegroundColor Cyan
    Write-Host $Message -ForegroundColor Cyan
    Write-Host "====================================" -ForegroundColor Cyan
}

function Test-CMake {
    try {
        cmake --version | Out-Null
        return $true
    } catch {
        return $false
    }
}

# Script main
Write-Header "Prisma Engine Windows Build Script"
Write-Host "Preset: $Preset" -ForegroundColor Green

# Check CMake
if (-not (Test-CMake)) {
    Write-Host "ERROR: CMake not found in PATH" -ForegroundColor Red
    Write-Host "Please install CMake and add it to your PATH"
    exit 1
}

# Clean build directory if requested
if ($Clean) {
    Write-Host ""
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    $buildDir = "build\$Preset"
    if (Test-Path $buildDir) {
        Remove-Item -Path $buildDir -Recurse -Force
        Write-Host "Removed: $buildDir" -ForegroundColor Gray
    }
}

# Configure
Write-Host ""
Write-Host "[1/2] Configuring with preset: $Preset" -ForegroundColor Yellow
cmake --preset $Preset
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    exit 1
}

# Build
Write-Host ""
Write-Host "[2/2] Building with preset: $Preset" -ForegroundColor Yellow
cmake --build --preset $Preset
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    exit 1
}

Write-Header "Build completed successfully!"
Write-Host "Output directory: build\$Preset" -ForegroundColor Green
Write-Host ""
