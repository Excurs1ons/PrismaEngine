# Prisma Engine Editor Build Script (PowerShell)
# Usage: .\build-editor.ps1 [-Preset] <preset> [-Clean] [-Quiet] [-Verbose]
#
# Available presets:
#   - editor-windows-x64-debug (default)
#   - editor-windows-x64-release

param(
    [Parameter(Position=0)]
    [ValidateSet("editor-windows-x64-debug", "editor-windows-x64-release")]
    [string]$Preset = "editor-windows-x64-debug",

    [switch]$Clean,

    [switch]$Q,

    [switch]$V,

    [switch]$Help
)

# Aliases for compatibility
$Quiet = $Q
$Verbose = $V

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
    Write-Host "Usage: .\build-editor.ps1 [-Preset] <preset> [options]" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Presets:" -ForegroundColor Yellow
    Write-Host "  editor-windows-x64-debug      (default)"
    Write-Host "  editor-windows-x64-release"
    Write-Host ""
    Write-Host "Options:" -ForegroundColor Yellow
    Write-Host "  -Preset <preset>    Build preset to use"
    Write-Host "  -Clean              Clean build directory before building"
    Write-Host "  -Q                  Quiet mode (only show errors and progress)"
    Write-Host "  -V                  Verbose mode (show full build output)"
    Write-Host "  -Help               Show this help message"
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build-editor.ps1 editor-windows-x64-debug"
    Write-Host "  .\build-editor.ps1 editor-windows-x64-release -Clean"
    Write-Host "  .\build-editor.ps1 editor-windows-x64-debug -Q"
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
Write-Header "Prisma Engine Editor Build Script"
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
$absBuildPath = Convert-Path "build\$Preset"
if ($Quiet) {
    Write-Host "Build completed: $absBuildPath" -ForegroundColor Green
} else {
    Write-Header "Build completed successfully!"
    Write-Host "Output directory: $absBuildPath" -ForegroundColor Green
    Write-Host ""
}