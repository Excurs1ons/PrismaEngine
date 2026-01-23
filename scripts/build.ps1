# Prisma Engine Unified Build Script (PowerShell)
# This script detects the platform and calls the appropriate build script

param(
    [Parameter(Position=0)]
    [string]$Preset,

    [Parameter(Position=1)]
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

function Show-Usage {
    Write-Host "Usage: .\build.ps1 [-Preset] <preset> [-Clean]" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Windows Presets:" -ForegroundColor Yellow
    Write-Host "  windows-x64-debug      (default)"
    Write-Host "  windows-x64-release"
    Write-Host "  windows-x86-debug"
    Write-Host "  windows-x86-release"
    Write-Host ""
    Write-Host "Android Presets:" -ForegroundColor Yellow
    Write-Host "  android-arm64-v8a-debug"
    Write-Host "  android-arm64-v8a-release"
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 windows-x64-debug"
    Write-Host "  .\build.ps1 windows-x64-release -Clean"
    Write-Host "  .\build.ps1 android-arm64-v8a-debug"
    Write-Host ""
}

if ([string]::IsNullOrWhiteSpace($Preset)) {
    Show-Usage
    exit 0
}

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "Prisma Engine Build Script" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

# Check if it's an Android preset
if ($Preset -match "android") {
    Write-Host "Detected Android preset, calling Android build script..." -ForegroundColor Yellow
    & "$ScriptDir\build-android.ps1" -Preset $Preset -Clean:$Clean
} else {
    Write-Host "Detected Windows preset, calling Windows build script..." -ForegroundColor Yellow
    & "$ScriptDir\build-windows.ps1" -Preset $Preset -Clean:$Clean
}
