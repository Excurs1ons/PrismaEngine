# Prisma Engine - Windows Environment Setup Script
# Usage: .\scripts\setup-env.ps1 [-SkipVulkan] [-SkipCMake] [-SkipVS] [-Force]
#
# This script installs and configures external dependencies that cannot be
# fetched by CMake FetchContent (Vulkan SDK, CMake, Visual Studio, etc.)

param(
    [switch]$SkipVulkan,
    [switch]$SkipCMake,
    [switch]$SkipVS,
    [switch]$Force,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Helper Functions
# ============================================================================

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "====================================" -ForegroundColor Cyan
    Write-Host " $Message" -ForegroundColor Cyan
    Write-Host "====================================" -ForegroundColor Cyan
}

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "[*] $Message" -ForegroundColor Yellow
}

function Write-OK {
    param([string]$Message)
    Write-Host "    [OK] $Message" -ForegroundColor Green
}

function Write-Skip {
    param([string]$Message)
    Write-Host "    [SKIP] $Message" -ForegroundColor DarkGray
}

function Write-Warn {
    param([string]$Message)
    Write-Host "    [WARN] $Message" -ForegroundColor DarkYellow
}

function Write-Fail {
    param([string]$Message)
    Write-Host "    [FAIL] $Message" -ForegroundColor Red
}

function Test-CommandExists {
    param([string]$Command)
    try {
        Get-Command $Command -ErrorAction Stop | Out-Null
        return $true
    } catch {
        return $false
    }
}

function Test-WingetAvailable {
    return (Test-CommandExists "winget")
}

function Show-Help {
    Write-Host "Prisma Engine - Windows Environment Setup" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage: .\scripts\setup-env.ps1 [options]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "This script installs external dependencies that CMake FetchContent"
    Write-Host "cannot manage (Vulkan SDK, build tools, etc.)."
    Write-Host ""
    Write-Host "Options:" -ForegroundColor Yellow
    Write-Host "  -SkipVulkan   Skip Vulkan SDK installation"
    Write-Host "  -SkipCMake    Skip CMake installation check"
    Write-Host "  -SkipVS       Skip Visual Studio check"
    Write-Host "  -Force        Re-install even if already present"
    Write-Host "  -Help         Show this help message"
    Write-Host ""
    Write-Host "What this script installs:" -ForegroundColor Yellow
    Write-Host "  1. Vulkan SDK       - Required for Vulkan rendering backend"
    Write-Host "  2. CMake            - Build system (>= 3.31)"
    Write-Host "  3. Visual Studio    - C++ compiler (checked, not auto-installed)"
    Write-Host ""
    Write-Host "What CMake FetchContent handles (NOT installed by this script):" -ForegroundColor DarkGray
    Write-Host "  GLM, nlohmann/json, stb, tinyxml2, zstd, SDL3, Vulkan-Headers,"
    Write-Host "  VMA, vk-bootstrap, ImGui"
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow    Write-Host "  .\scripts\setup-env.ps1              # Full setup"
    Write-Host "  .\scripts\setup-env.ps1 -SkipVS      # Skip Visual Studio check"
    Write-Host "  .\scripts\setup-env.ps1 -Force        # Re-install everything"
    Write-Host ""
}

# ============================================================================
# Main
# ============================================================================

if ($Help) {
    Show-Help
    exit 0
}

Write-Header "Prisma Engine - Windows Environment Setup"

# Check winget availability
if (-not (Test-WingetAvailable)) {
    Write-Fail "winget is not available. Please install App Installer from the Microsoft Store."
    Write-Host "    https://aka.ms/getwinget" -ForegroundColor Gray
    exit 1
}
Write-OK "winget is available"

$needsRestart = $false

# ============================================================================
# 1. Vulkan SDK
# ============================================================================

if (-not $SkipVulkan) {
    Write-Step "Checking Vulkan SDK..."

    $vulkanSDK = $env:VULKAN_SDK
    $vulkanInstalled = $false

    if ($vulkanSDK -and (Test-Path "$vulkanSDK\Lib\vulkan-1.lib") -and -not $Force) {
        Write-OK "Vulkan SDK found at: $vulkanSDK"
        $vulkanInstalled = $true
    } else {
        # Check common install locations
        $vulkanBase = "C:\VulkanSDK"
        if (Test-Path $vulkanBase) {
            $latestVersion = Get-ChildItem $vulkanBase -Directory | Sort-Object Name -Descending | Select-Object -First 1
            if ($latestVersion -and (Test-Path "$($latestVersion.FullName)\Lib\vulkan-1.lib") -and -not $Force) {
                Write-OK "Vulkan SDK found at: $($latestVersion.FullName)"
                Write-Warn "VULKAN_SDK environment variable is not set. Setting it now..."

                [System.Environment]::SetEnvironmentVariable("VULKAN_SDK", $latestVersion.FullName, "User")
                $env:VULKAN_SDK = $latestVersion.FullName
                Write-OK "VULKAN_SDK set to: $($latestVersion.FullName)"
                $vulkanInstalled = $true
                $needsRestart = $true
            }
        }

        if (-not $vulkanInstalled) {
            Write-Host "    Installing Vulkan SDK via winget..." -ForegroundColor Yellow
            winget install KhronosGroup.VulkanSDK --accept-source-agreements --accept-package-agreements
            if ($LASTEXITCODE -ne 0) {
                Write-Fail "Failed to install Vulkan SDK"
                Write-Host "    Please install manually from: https://vulkan.lunarg.com/sdk/home" -ForegroundColor Gray
            } else {
                Write-OK "Vulkan SDK installed successfully"

                # Find and set VULKAN_SDK
                $vulkanBase = "C:\VulkanSDK"
                if (Test-Path $vulkanBase) {
                    $latestVersion = Get-ChildItem $vulkanBase -Directory | Sort-Object Name -Descending | Select-Object -First 1
                    if ($latestVersion) {
                        [System.Environment]::SetEnvironmentVariable("VULKAN_SDK", $latestVersion.FullName, "User")
                        $env:VULKAN_SDK = $latestVersion.FullName
                        Write-OK "VULKAN_SDK set to: $($latestVersion.FullName)"
                        $needsRestart = $true
                    }
                }
            }
        }
    }
} else {
    Write-Skip "Vulkan SDK (skipped by user)"
}

# ============================================================================
# 2. CMake
# ============================================================================

if (-not $SkipCMake) {
    Write-Step "Checking CMake..."

    if (Test-CommandExists "cmake") {
        $cmakeVersion = (cmake --version | Select-Object -First 1) -replace "cmake version ", ""
        $major, $minor, $patch = $cmakeVersion.Split(".")

        if ([int]$major -gt 3 -or ([int]$major -eq 3 -and [int]$minor -ge 31)) {
            if (-not $Force) {
                Write-OK "CMake $cmakeVersion found (>= 3.31 required)"
            } else {
                Write-Host "    Upgrading CMake via winget..." -ForegroundColor Yellow
                winget upgrade Kitware.CMake --accept-source-agreements --accept-package-agreements
                Write-OK "CMake updated"
            }
        } else {
            Write-Warn "CMake $cmakeVersion found, but >= 3.31 is required"
            Write-Host "    Upgrading CMake via winget..." -ForegroundColor Yellow
            winget install Kitware.CMake --accept-source-agreements --accept-package-agreements
            if ($LASTEXITCODE -ne 0) {
                Write-Fail "Failed to install CMake. Please install manually from: https://cmake.org/download/"
            } else {
                Write-OK "CMake installed/upgraded successfully"
                $needsRestart = $true
            }
        }
    } else {
        Write-Host "    Installing CMake via winget..." -ForegroundColor Yellow
        winget install Kitware.CMake --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -ne 0) {
            Write-Fail "Failed to install CMake. Please install manually from: https://cmake.org/download/"
        } else {
            Write-OK "CMake installed successfully"
            $needsRestart = $true
        }
    }
} else {
    Write-Skip "CMake (skipped by user)"
}

# ============================================================================
# 3. Visual Studio / MSVC Compiler
# ============================================================================

if (-not $SkipVS) {
    Write-Step "Checking Visual Studio / MSVC..."

    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsInstalls = & $vsWhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
        if ($vsInstalls -and $vsInstalls.Count -gt 0) {
            $latestVS = $vsInstalls | Sort-Object -Property installationVersion -Descending | Select-Object -First 1
            Write-OK "Visual Studio found: $($latestVS.displayName) ($($latestVS.installationVersion))"
            Write-OK "Path: $($latestVS.installationPath)"
        } else {
            Write-Warn "Visual Studio is installed but C++ Desktop workload is missing"
            Write-Host "    Please run Visual Studio Installer and add:" -ForegroundColor Gray
            Write-Host "      - 'Desktop development with C++' workload" -ForegroundColor Gray
            Write-Host "      - Ensure 'MSVC v143' toolset is selected" -ForegroundColor Gray
        }
    } else {
        Write-Warn "Visual Studio not found"
        Write-Host "    Please install Visual Studio 2026 with the 'Desktop development with C++' workload" -ForegroundColor Gray
        Write-Host "    Download: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Gray
        Write-Host ""
        Write-Host "    Or install via winget:" -ForegroundColor Gray
        Write-Host "      winget install Microsoft.VisualStudio.2026.Community" -ForegroundColor DarkGray
        Write-Host "    Then add C++ workload via Visual Studio Installer." -ForegroundColor Gray
    }
} else {
    Write-Skip "Visual Studio (skipped by user)"
}

# ============================================================================
# 4. Git (sanity check)
# ============================================================================

Write-Step "Checking Git..."
if (Test-CommandExists "git") {
    $gitVersion = (git --version) -replace "git version ", ""
    Write-OK "Git $gitVersion found"
} else {
    Write-Warn "Git not found. Installing via winget..."
    winget install Git.Git --accept-source-agreements --accept-package-agreements
    if ($LASTEXITCODE -eq 0) {
        Write-OK "Git installed successfully"
        $needsRestart = $true
    } else {
        Write-Fail "Failed to install Git. Please install manually from: https://git-scm.com/"
    }
}

# ============================================================================
# Summary
# ============================================================================

Write-Header "Setup Complete"

Write-Host ""
Write-Host "  External dependencies (installed by this script):" -ForegroundColor White
Write-Host "    - Vulkan SDK       (rendering backend)" -ForegroundColor Gray
Write-Host "    - CMake >= 3.31    (build system)" -ForegroundColor Gray
Write-Host "    - Visual Studio    (C++ compiler)" -ForegroundColor Gray
Write-Host "    - Git              (source control)" -ForegroundColor Gray
Write-Host ""
Write-Host "  CMake FetchContent dependencies (auto-downloaded on first build):" -ForegroundColor White
Write-Host "    GLM, nlohmann/json, stb, tinyxml2, zstd, SDL3," -ForegroundColor DarkGray
Write-Host "    Vulkan-Headers, VMA, vk-bootstrap, ImGui" -ForegroundColor DarkGray
Write-Host ""

if ($needsRestart) {
    Write-Warn "Environment variables were changed. Please restart your terminal/IDE"
    Write-Warn "for changes to take effect, then run:"
    Write-Host ""
    Write-Host "  cmake --preset engine-windows-x64-debug" -ForegroundColor White
    Write-Host "  cmake --build --preset engine-windows-x64-debug" -ForegroundColor White
} else {
    Write-Host "  Ready to build:" -ForegroundColor Green
    Write-Host "    cmake --preset engine-windows-x64-debug" -ForegroundColor White
    Write-Host "    cmake --build --preset engine-windows-x64-debug" -ForegroundColor White
}

Write-Host ""
