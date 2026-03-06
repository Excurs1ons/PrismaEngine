# PrismaEngine SDK 打包脚本 (Windows PowerShell)
# 用法: .\scripts\package-sdk.ps1 -Version "0.1.0" -Config "Debug"

param(
    [string]$Version = "0.1.0",
    [string]$Config = "Debug",
    [string]$InstallDir = "$PSScriptRoot\..\sdk\out"
)

$ErrorActionPreference = "Stop"

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "PrismaEngine SDK Packaging Tool" -ForegroundColor Cyan
Write-Host "Version: $Version" -ForegroundColor Green
Write-Host "Config: $Config" -ForegroundColor Green
Write-Host "Install Dir: $InstallDir" -ForegroundColor Green
Write-Host "====================================" -ForegroundColor Cyan

# 确保目录存在
if (!(Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force
}

# 1. 配置 CMake
Write-Host "`n[1/3] Configuring CMake..." -ForegroundColor Yellow
$Preset = "engine-windows-x64-" + $Config.ToLower()
$BuildDir = "build/$Preset"

cmake --preset $Preset -DPRISMA_ENABLE_INSTALL=ON -DCMAKE_INSTALL_PREFIX=$InstallDir

# 2. 构建并安装
Write-Host "`n[2/3] Building and Installing..." -ForegroundColor Yellow
cmake --build $BuildDir --config $Config --target install --parallel

# 3. 验证
Write-Host "`n[3/3] Verifying SDK layout..." -ForegroundColor Yellow
if (Test-Path "$InstallDir\include\PrismaEngine\Engine.h") {
    Write-Host "SDK successfully installed to $InstallDir" -ForegroundColor Green
} else {
    Write-Host "Error: SDK installation failed!" -ForegroundColor Red
    exit 1
}

Write-Host "`nSDK packaging complete!" -ForegroundColor Cyan
