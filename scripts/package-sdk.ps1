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

# 1. 配置 CMake (暂时禁用 install 以解决 CI 问题)
Write-Host "`n[1/3] Configuring CMake..." -ForegroundColor Yellow
$Preset = "engine-windows-x64-" + $Config.ToLower()
$BuildDir = "build/$Preset"

cmake --preset $Preset -DCMAKE_INSTALL_PREFIX=$InstallDir

# 2. 构建
Write-Host "`n[2/3] Building..." -ForegroundColor Yellow
cmake --build $BuildDir --config $Config --parallel

# 3. 验证
Write-Host "`n[3/3] Verifying SDK layout..." -ForegroundColor Yellow
if (Test-Path "$BuildDir\lib\Debug\PrismaEngine.lib") {
    Write-Host "Engine library built successfully!" -ForegroundColor Green
} else {
    Write-Host "Warning: Engine library not found at expected location" -ForegroundColor Yellow
}

Write-Host "`nSDK build complete!" -ForegroundColor Cyan
