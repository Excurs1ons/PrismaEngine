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

# 1. 配置 CMake with SDK mode enabled
Write-Host "`n[1/4] Configuring CMake with SDK mode..." -ForegroundColor Yellow
$Preset = "engine-windows-x64-" + $Config.ToLower()
$BuildDir = "build/$Preset"

# Enable SDK build and PrismaCraft integration
cmake --preset $Preset -DPRISMA_BUILD_SDK=ON -DCMAKE_INSTALL_PREFIX=$InstallDir

# 2. 构建 Engine 和 PrismaCraft
Write-Host "`n[2/4] Building Engine and PrismaCraft..." -ForegroundColor Yellow
cmake --build $BuildDir --config $Config --parallel

# 3. 手动打包 (不使用复杂的 export)
Write-Host "`n[3/4] Packaging SDK..." -ForegroundColor Yellow

# 复制头文件
$IncludeDir = "$InstallDir\include\PrismaEngine"
New-Item -ItemType Directory -Path $IncludeDir -Force | Out-Null

# 复制 Engine 头文件
Get-ChildItem -Path "src\engine" -Filter "*.h" -Recurse | Where-Object { $_.FullName -notmatch "adapters|drivers|platform" } | ForEach-Object {
    $DestDir = $IncludeDir + $_.FullName.Replace("src\engine", "").Replace($_.Name, "")
    if (!(Test-Path $DestDir)) { New-Item -ItemType Directory -Path $DestDir -Force | Out-Null }
    Copy-Item $_.FullName -Destination $DestDir -Force
}

# 复制库文件
$LibDir = "$InstallDir\lib"
New-Item -ItemType Directory -Path $LibDir -Force | Out-Null

if (Test-Path "$BuildDir\lib\Debug\PrismaEngine.lib") {
    Copy-Item "$BuildDir\lib\Debug\PrismaEngine.lib" -Destination $LibDir -Force
}
if (Test-Path "$BuildDir\bin\Debug\PrismaEngine.dll") {
    Copy-Item "$BuildDir\bin\Debug\PrismaEngine.dll" -Destination "$InstallDir\bin" -Force
}

# 复制 PrismaCraft
if (Test-Path "$BuildDir\projects\PrismaCraft\libPrismaCraft.lib") {
    Copy-Item "$BuildDir\projects\PrismaCraft\libPrismaCraft.lib" -Destination $LibDir -Force
}

# 复制 CMake 配置文件
$SdkCMakeDir = "$InstallDir\cmake\PrismaEngine"
New-Item -ItemType Directory -Path $SdkCMakeDir -Force | Out-Null
Copy-Item "cmake\PrismaEngineConfig.cmake.in" -Destination $SdkCMakeDir -Force

# 4. 验证
Write-Host "`n[4/4] Verifying SDK layout..." -ForegroundColor Yellow
$Success = $true

if (!(Test-Path "$InstallDir\lib\PrismaEngine.lib")) {
    Write-Host "Warning: Engine lib not found" -ForegroundColor Yellow
    $Success = $false
}

if ($Success) {
    Write-Host "SDK successfully packaged to $InstallDir" -ForegroundColor Green
} else {
    Write-Host "Warning: SDK packaging completed with warnings" -ForegroundColor Yellow
}

Write-Host "`nSDK packaging complete!" -ForegroundColor Cyan
