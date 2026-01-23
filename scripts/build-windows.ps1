# Prisma Engine Windows Build Script (PowerShell)
# Usage: .\build-windows.ps1 [-Preset] <preset> [-Clean] [-Quiet] [-Verbose]
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

    [switch]$Clean,

    [switch]$Quiet,

    [switch]$Verbose,

    [switch]$Help
)

$ErrorActionPreference = "Stop"

function Write-Header {
    param([string]$Message)
    if (-not $Quiet) {
        Write-Host ""
        Write-Host "====================================" -ForegroundColor Cyan
        Write-Host $Message -ForegroundColor Cyan
        Write-Host "====================================" -ForegroundColor Cyan
    }
}

function Write-Step {
    param([string]$Message)
    if (-not $Quiet) {
        Write-Host ""
        Write-Host "[$Message]" -ForegroundColor Yellow
    }
}

function Test-CMake {
    try {
        cmake --version | Out-Null
        return $true
    } catch {
        return $false
    }
}

function Show-Help {
    Write-Host "Usage: .\build-windows.ps1 [-Preset] <preset> [options]" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Presets:" -ForegroundColor Yellow
    Write-Host "  windows-x64-debug      (default)"
    Write-Host "  windows-x64-release"
    Write-Host "  windows-x86-debug"
    Write-Host "  windows-x86-release"
    Write-Host ""
    Write-Host "Options:" -ForegroundColor Yellow
    Write-Host "  -Preset <preset>    Build preset to use"
    Write-Host "  -Clean              Clean build directory before building"
    Write-Host "  -Quiet, -Q          Reduce output (only show errors and progress)"
    Write-Host "  -Verbose, -V        Show full build output"
    Write-Host "  -Help               Show this help message"
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build-windows.ps1 windows-x64-debug"
    Write-Host "  .\build-windows.ps1 windows-x64-release -Clean"
    Write-Host "  .\build-windows.ps1 windows-x64-debug -Quiet"
    Write-Host ""
}

if ($Help) {
    Show-Help
    exit 0
}

# Set CMake log level
if ($Quiet) {
    $CmakeLogLevel = "--log-level=ERROR"
    $CmakeOutputMode = "--output-mode=errors-only"
} elseif ($Verbose) {
    $CmakeLogLevel = "--log-level=VERBOSE"
    $CmakeOutputMode = "--verbose"
} else {
    $CmakeLogLevel = "--log-level=STATUS"
    $CmakeOutputMode = ""
}

# Script main
Write-Header "Prisma Engine Windows Build Script"
if (-not $Quiet) {
    Write-Host "Preset: $Preset" -ForegroundColor Green
}

# Check CMake
if (-not (Test-CMake)) {
    Write-Host "ERROR: CMake not found in PATH" -ForegroundColor Red
    Write-Host "Please install CMake and add it to your PATH"
    exit 1
}

# Clean build directory if requested
if ($Clean) {
    if (-not $Quiet) {
        Write-Host ""
        Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    }
    $buildDir = "build\$Preset"
    if (Test-Path $buildDir) {
        Remove-Item -Path $buildDir -Recurse -Force
        if (-not $Quiet) {
            Write-Host "Removed: $buildDir" -ForegroundColor Gray
        }
    }
}

# Configure
Write-Step "1/2] Configuring with preset: $Preset"
cmake --preset $Preset $CmakeLogLevel
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    exit 1
}

# Build
Write-Step "2/2] Building with preset: $Preset"
$buildArgs = @("--build", "--preset", $Preset)
if ($CmakeOutputMode) {
    $buildArgs += $CmakeOutputMode
}
& cmake $buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    exit 1
}

# Success message
if ($Quiet) {
    Write-Host "Build completed: build\$Preset" -ForegroundColor Green
} else {
    Write-Header "Build completed successfully!"
    Write-Host "Output directory: build\$Preset" -ForegroundColor Green
    Write-Host ""
}
