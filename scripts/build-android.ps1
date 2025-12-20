# Android构建脚本 (PowerShell版本)
param(
    [string]$abi = "arm64-v8a",
    [string]$type = "Release",
    [string]$platform = "24",
    [switch]$all,
    [switch]$clean,
    [switch]$help
)

# 颜色定义
$Colors = @{
    Red = "Red"
    Green = "Green"
    Yellow = "Yellow"
    White = "White"
}

# 支持的ABI列表
$Abis = @("arm64-v8a", "armeabi-v7a")
$DefaultAbis = "arm64-v8a"

function Write-ColorText {
    param(
        [string]$Text,
        [string]$Color = "White"
    )
    Write-Host $Text -ForegroundColor $Colors[$Color]
}

function Write-Info {
    param([string]$Message)
    Write-ColorText "[INFO] $Message" "Green"
}

function Write-Warn {
    param([string]$Message)
    Write-ColorText "[WARN] $Message" "Yellow"
}

function Write-Error {
    param([string]$Message)
    Write-ColorText "[ERROR] $Message" "Red"
}

function Show-Help {
    Write-Host "用法: $($MyInvocation.MyCommand.Name) [选项]"
    Write-Host ""
    Write-Host "选项:"
    Write-Host "  -a, --abi ABI           指定ABI架构 (可选，默认: arm64-v8a)"
    Write-Host "  -t, --type TYPE         构建类型 (Debug|Release, 默认: Release)"
    Write-Host "  -p, --platform PLATFORM Android平台版本 (默认: 24)"
    Write-Host "  --all                   构建所有支持的ABI"
    Write-Host "  --clean                 清理构建目录"
    Write-Host ""
    Write-Host "环境变量:"
    Write-Host "  ANDROID_NDK_HOME        Android NDK路径"
    Write-Host "  ANDROID_HOME            Android SDK路径"
    Write-Host "  VCPKG_ROOT              vcpkg根目录 (可选)"
    Write-Host ""
    Write-Host "支持的ABI: $($Abis -join ', ')"
    Write-Host ""
    Write-Host "注意: 已移除x86和x86_64架构，因为ARM架构在Android设备中占主导地位"
}

function Show-NDKSetupHelp {
    Write-Host ""
    Write-ColorText "===== Android NDK 环境设置指南 =====" "Green"
    Write-Host ""
    Write-ColorText "方法1: 使用Android Studio安装NDK" "Yellow"
    Write-Host "1. 打开Android Studio"
    Write-Host "2. 打开 File > Settings > Appearance & Behavior > System Settings > Android SDK"
    Write-Host "3. 切换到 'SDK Tools' 选项卡"
    Write-Host "4. 勾选 'NDK (Side by side)' 和 'CMake'"
    Write-Host "5. 点击 'Apply' 或 'OK' 进行安装"
    Write-Host ""
    Write-ColorText "方法2: 手动下载NDK" "Yellow"
    Write-Host "下载地址: https://developer.android.com/ndk/downloads"
    Write-Host "推荐版本: NDK r27 或更高版本"
    Write-Host ""
    Write-ColorText "设置环境变量" "Yellow"
    Write-Host "Windows PowerShell:"
    Write-Host '  $env:ANDROID_NDK_HOME = "C:\Users\' + $env:USERNAME + '\AppData\Local\Android\Sdk\ndk\27.0.12077973"'
    Write-Host '  $env:ANDROID_HOME = "C:\Users\' + $env:USERNAME + '\AppData\Local\Android\Sdk"'
    Write-Host ""
    Write-Host "注意: ANDROID_HOME应该指向Sdk目录，不是Android目录"
    Write-Host ""
    Write-Host "或设置系统环境变量:"
    Write-Host "1. 右键'此电脑' > '属性'"
    Write-Host "2. 点击'高级系统设置'"
    Write-Host "3. 点击'环境变量'"
    Write-Host "4. 在'系统变量'中添加:"
    Write-Host "   - ANDROID_NDK_HOME = [您的NDK路径]"
    Write-Host "   - ANDROID_HOME = [您的SDK路径]"
    Write-Host ""
    Write-ColorText "常见NDK安装路径:" "Yellow"
    Write-Host "  - `$env:LOCALAPPDATA\Android\Sdk\ndk\27.0.12077973"
    Write-Host "  - C:\Android\Sdk\ndk\27.0.12077973"
    Write-Host "  - D:\Android\ndk\27.0.12077973"
    Write-Host ""
    Write-ColorText "设置完成后，请重新运行此脚本" "Green"
    Write-Host ""

    Write-Host "按任意键继续..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}

function Check-Environment {
    Write-Info "检查构建环境..."

    # 如果没有设置ANDROID_NDK_HOME但设置了ANDROID_HOME，尝试查找NDK
    if (-not $env:ANDROID_NDK_HOME -and $env:ANDROID_HOME) {
        $ndkBasePath = "$env:ANDROID_HOME\ndk"

        # 首先尝试常见的NDK版本路径
        $commonVersions = @("27.0.12077973", "27.0.11902837", "26.1.10909125", "25.2.9519653")
        $foundNdk = $null

        foreach ($version in $commonVersions) {
            $testPath = "$ndkBasePath\$version"
            if (Test-Path $testPath) {
                $foundNdk = $testPath
                break
            }
        }

        # 如果常见版本没找到，查找所有NDK目录并选择最新的
        if (-not $foundNdk) {
            $ndkPaths = Get-ChildItem -Path $ndkBasePath -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending
            if ($ndkPaths) {
                $foundNdk = $ndkPaths[0].FullName
                Write-Warn "找到多个NDK版本，使用最新的: $($ndkPaths[0].Name)"
            }
        }

        if ($foundNdk) {
            # 验证NDK目录结构
            $ndkMarker = "$foundNdk\build\cmake\android.toolchain.cmake"
            if (Test-Path $ndkMarker) {
                $env:ANDROID_NDK_HOME = $foundNdk
                Write-Info "从ANDROID_HOME自动检测到NDK: $env:ANDROID_NDK_HOME"
            } else {
                Write-Warn "找到NDK目录但缺少必要文件: $foundNdk"
                Show-NDKSetupHelp
                exit 1
            }
        } else {
            Write-Warn "在ANDROID_HOME下未找到NDK: $ndkBasePath"
            Write-Warn "请确保已通过Android Studio SDK Manager安装NDK"
            Show-NDKSetupHelp
            exit 1
        }
    }

    # 如果还是没有NDK路径
    if (-not $env:ANDROID_NDK_HOME) {
        if (-not $env:ANDROID_HOME) {
            Write-Warn "未设置ANDROID_NDK_HOME和ANDROID_HOME环境变量"
        } else {
            Write-Warn "未设置ANDROID_NDK_HOME，且无法从ANDROID_HOME自动检测"
        }
        Show-NDKSetupHelp
        Write-Error "请设置环境变量后重试"
        exit 1
    }

    # 验证NDK路径是否存在
    if (-not (Test-Path $env:ANDROID_NDK_HOME)) {
        Write-Error "NDK路径不存在: $env:ANDROID_NDK_HOME"
        Write-Error "请检查NDK是否正确安装"
        exit 1
    }

    # 验证NDK完整性
    $ndkToolchain = "$env:ANDROID_NDK_HOME\build\cmake\android.toolchain.cmake"
    if (-not (Test-Path $ndkToolchain)) {
        Write-Error "NDK安装不完整，缺少工具链文件: $ndkToolchain"
        Write-Error "请重新安装NDK"
        exit 1
    }

    # 检查CMake
    try {
        $cmakeVersion = cmake --version 2>&1 | Select-String "cmake version"
        if ($cmakeVersion) {
            Write-Info "环境检查完成"
            Write-Info "  NDK路径: $env:ANDROID_NDK_HOME"
            Write-Info "  $($cmakeVersion.Line)"
            Write-Info "  构建类型: $type"
            Write-Info "  Android平台: $platform"
        } else {
            throw
        }
    } catch {
        Write-Error "未找到CMake，请安装CMake 3.31+"
        exit 1
    }

    # 检查Ninja
    try {
        ninja --version | Out-Null
    } catch {
        Write-Warn "未找到Ninja，将使用默认生成器"
    }
}

function Build-Abi {
    param([string]$AbiName)

    Write-Info "开始构建 $AbiName..."

    # 构建目录名格式：android-{abi}-{type}
    $buildSuffix = if ($type -eq "Debug") { "debug" } else { "release" }
    $buildDir = "..\build\android-$AbiName-$buildSuffix"

    # 创建构建目录
    if (!(Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
    }

    Push-Location $buildDir

    # 使用项目的vcpkg
    $projectRoot = (Resolve-Path "..\..").Path
    $vcpkgRoot = "$projectRoot\vcpkg"
    $vcpkgExe = "$vcpkgRoot\vcpkg.exe"
    $vcpkgToolchain = "$vcpkgRoot\scripts\buildsystems\vcpkg.cmake"

    # 配置CMake
    $cmakeArgs = @(
        "-DCMAKE_TOOLCHAIN_FILE=$env:ANDROID_NDK_HOME\build\cmake\android.toolchain.cmake",
        "-DANDROID_ABI=$AbiName",
        "-DANDROID_PLATFORM=$platform",
        "-DANDROID_STL=c++_shared",
        "-DCMAKE_BUILD_TYPE=$type",
        "-DCMAKE_INSTALL_PREFIX=..\..\build\install\$AbiName",
        # Android设备选项
        "-DPRISMA_ENABLE_AUDIO_XAUDIO2=OFF",
        "-DPRISMA_ENABLE_AUDIO_SDL3=ON",
        "-DPRISMA_ENABLE_RENDER_DX12=OFF",
        "-DPRISMA_ENABLE_RENDER_VULKAN=ON",
        # 排除Editor (Windows only)
        "-DPRISMA_BUILD_EDITOR=OFF",
        "-G", "Ninja"
    )

    # 暂时不使用vcpkg工具链，只使用Android NDK工具链
    # 注意：vcpkg已安装的库可以通过CMAKE_PREFIX_PATH找到
    $cmakeArgs += @(
        "-DCMAKE_PREFIX_PATH=$projectRoot\vcpkg_installed\arm64-android",
        "-DCMAKE_MODULE_PATH=$projectRoot\vcpkg_installed\arm64-android/share",
        "-DVCPKG_MANIFEST_MODE=OFF"
    )

    # 强制检查并安装Android依赖
    Write-Info "检查Android依赖..."
    $androidInstalledDir = "$projectRoot\vcpkg_installed\arm64-android"

    Write-Info "vcpkg路径: $vcpkgExe"
    Write-Info "Android安装目录: $androidInstalledDir"
    Write-Info "目录存在: $(Test-Path $androidInstalledDir)"

    if (Test-Path $vcpkgExe) {
        # 总是尝试安装或更新依赖
        Write-Info "安装Android依赖（arm64-android）..."
        Push-Location $projectRoot

        # 显示vcpkg命令
        $installCmd = "& '$vcpkgExe' install --triplet arm64-android"
        Write-Info "执行: $installCmd"

        & $vcpkgExe install --triplet arm64-android
        $exitCode = $LASTEXITCODE
        Pop-Location

        if ($exitCode -ne 0) {
            Write-Error "vcpkg依赖安装失败，退出码: $exitCode"
            # 继续构建，让用户看到错误
        } else {
            Write-Info "Android依赖安装成功"
        }
    } else {
        Write-Warn "未找到vcpkg，跳过依赖安装"
    }

    Write-Info "配置CMake..."
    # 使用项目根目录作为源码目录
    $projectRoot = (Resolve-Path "..\..").Path
    $result = cmake $cmakeArgs $projectRoot
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Error "CMake配置失败"
        exit 1
    }

    # 构建
    Write-Info "开始编译..."
    $result = ninja -v 2>&1
    $buildOutput = $result -join "`n"
    Write-Host $buildOutput

    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Error "编译失败"
        # 显示最后几行错误信息
        Write-Host "最后10行输出:" -ForegroundColor Yellow
        $buildOutput -split "`n" | Select-Object -Last 10 | ForEach-Object { Write-Host $_ }
        exit 1
    }

    # 安装
    Write-Info "安装到输出目录..."
    $result = ninja install
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Error "安装失败"
        exit 1
    }

    Pop-Location
    Write-Info "$AbiName 构建完成"
}

# 主程序
if ($help) {
    Show-Help
    exit 0
}

# 清理构建目录
if ($clean) {
    Write-Info "清理构建目录..."
    if (Test-Path "..\build") {
        $androidBuilds = Get-ChildItem -Path "..\build" -Directory -Name "android-*" -ErrorAction SilentlyContinue
        foreach ($dir in $androidBuilds) {
            Write-Info "删除: build\$dir"
            Remove-Item -Path "..\build\$dir" -Recurse -Force
        }
    }
}

# 检查环境
Check-Environment

# 确定要构建的ABI
if ($all) {
    $buildAbis = $Abis
} else {
    $buildAbis = @($abi)
}

# 记录开始时间
$startTime = Get-Date

# 构建指定的ABI
foreach ($a in $buildAbis) {
    Build-Abi $a
}

# 记录结束时间
$endTime = Get-Date
$duration = $endTime - $startTime

Write-Info "构建完成！耗时: $($duration.TotalSeconds.ToString('0.00'))秒"

# 显示输出文件
Write-Info "输出文件:"
foreach ($a in $buildAbis) {
    $libPath = "..\build\install\$a\lib\libEngine.so"
    if (Test-Path $libPath) {
        $fileInfo = Get-Item $libPath
        Write-Info "  $a`: build\install\$a\lib\libEngine.so (大小: $([math]::Round($fileInfo.Length / 1MB, 2)) MB)"
    }
}