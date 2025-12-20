@echo off
:: 检测是否在PowerShell中运行
echo %PSCommandPath% | findstr /i powershell >nul
if %errorlevel% == 0 (
    :: 在PowerShell中，使用Write-Host输出彩色
    set "USE_POWERSHELL_COLORS=1"
) else (
    :: 在CMD中，设置UTF-8并使用ANSI颜色
    chcp 65001 >nul 2>&1
    set "USE_POWERSHELL_COLORS=0"
)

setlocal enabledelayedexpansion

:: 颜色输出定义
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "NC=[0m"

:: 配置参数
if "%ANDROID_PLATFORM%"=="" set "ANDROID_PLATFORM=24"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"
if "%ANDROID_STL%"=="" set "ANDROID_STL=c++_shared"

:: 支持的ABI列表
set "ABIS=arm64-v8a armeabi-v7a"
set "DEFAULT_ABIS=arm64-v8a"

:: 函数：打印消息
:print_info
echo %GREEN%[INFO]%NC% %~1
goto :eof

:print_warn
echo %YELLOW%[WARN]%NC% %~1
goto :eof

:print_error
echo %RED%[ERROR]%NC% %~1
goto :eof

:show_ndk_setup_help
echo.
echo %GREEN%===== Android NDK 环境设置指南 =====%NC%
echo.
echo %YELLOW%方法1: 使用Android Studio安装NDK%NC%
echo 1. 打开Android Studio
echo 2. 打开 File ^> Settings ^> Appearance & Behavior ^> System Settings ^> Android SDK
echo 3. 切换到 "SDK Tools" 选项卡
echo 4. 勾选 "NDK (Side by side)" 和 "CMake"
echo 5. 点击 "Apply" 或 "OK" 进行安装
echo.
echo %YELLOW%方法2: 手动下载NDK%NC%
echo 下载地址: https://developer.android.com/ndk/downloads
echo 推荐版本: NDK r27 或更高版本
echo.
echo %YELLOW%设置环境变量%NC%
echo Windows 命令提示符:
echo   set ANDROID_NDK_HOME=C:\Users\%USERNAME%\AppData\Local\Android\Sdk\ndk\27.0.12077973
echo   set ANDROID_HOME=C:\Users\%USERNAME%\AppData\Local\Android\Sdk
echo.
echo 注意: ANDROID_HOME应该指向Sdk目录，不是Android目录
echo.
echo 或设置系统环境变量:
echo 1. 右键"此电脑" ^> "属性"
echo 2. 点击"高级系统设置"
echo 3. 点击"环境变量"
echo 4. 在"系统变量"中添加:
echo    - ANDROID_NDK_HOME = [您的NDK路径]
echo    - ANDROID_HOME = [您的SDK路径]
echo.
echo %YELLOW%常见NDK安装路径:%NC%
echo   - %%LOCALAPPDATA%%\Android\Sdk\ndk\27.0.12077973
echo   - C:\Android\Sdk\ndk\27.0.12077973
echo   - D:\Android\ndk\27.0.12077973
echo.
echo %GREEN%设置完成后，请重新运行此脚本%NC%
echo.
pause
goto :eof

:show_help
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   -h, --help              显示此帮助信息
echo   -a, --abi ABI           指定ABI架构 (可选，默认: arm64-v8a)
echo   -t, --type TYPE         构建类型 (Debug^|Release, 默认: Release)
echo   -p, --platform PLATFORM Android平台版本 (默认: 24)
echo   --all                   构建所有支持的ABI
echo   --clean                 清理构建目录
echo.
echo 环境变量:
echo   ANDROID_NDK_HOME        Android NDK路径
echo   ANDROID_HOME            Android SDK路径
echo   VCPKG_ROOT              vcpkg根目录 (可选)
echo.
echo 支持的ABI: %ABIS%
echo.
echo 注意: 已移除x86和x86_64架构，因为ARM架构在Android设备中占主导地位
echo.
echo 如果遇到NDK未找到的错误，请参考上面的设置指南
goto :eof

:check_environment
call :print_info "检查构建环境..."

if "%ANDROID_NDK_HOME%"=="" (
    if not "%ANDROID_HOME%"=="" (
        :: 首先尝试常见的NDK版本路径
        set "FOUND_NDK="
        for %%v in (27.0.12077973 27.0.11902837 26.1.10909125 25.2.9519653) do (
            if exist "%ANDROID_HOME%\ndk\%%v" (
                set "FOUND_NDK=%ANDROID_HOME%\ndk\%%v"
                goto :ndk_found
            )
        )

        :ndk_found
        if defined FOUND_NDK (
            set "ANDROID_NDK_HOME=!FOUND_NDK!"
            call :print_info "从ANDROID_HOME自动检测到NDK: !ANDROID_NDK_HOME!"
        ) else (
            :: 如果常见版本没找到，查找所有NDK目录并选择最新的
            for /f "delims=" %%i in ('dir "%ANDROID_HOME%\ndk" /b /ad 2^>nul ^| sort /r') do (
                set "ANDROID_NDK_HOME=%ANDROID_HOME%\ndk\%%i"
                call :print_warn "找到多个NDK版本，使用最新的: %%i"
                goto :ndk_check_done
            )

            :ndk_check_done
            if not exist "!ANDROID_NDK_HOME!\build\cmake\android.toolchain.cmake" (
                call :print_warn "在ANDROID_HOME下未找到有效的NDK"
                call :print_warn "请确保已通过Android Studio SDK Manager安装NDK"
                call :show_ndk_setup_help
                pause
                exit /b 1
            ) else (
                call :print_info "从ANDROID_HOME找到NDK: !ANDROID_NDK_HOME!"
            )
        )
    ) else (
        call :print_warn "未设置ANDROID_NDK_HOME和ANDROID_HOME环境变量"
        call :show_ndk_setup_help
        pause
        exit /b 1
    )
)

if not exist "%ANDROID_NDK_HOME%" (
    call :print_error "NDK路径不存在: %ANDROID_NDK_HOME%"
    exit /b 1
)

:: 检查CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    call :print_error "未找到CMake，请安装CMake 3.31+"
    exit /b 1
)

:: 检查Ninja
ninja --version >nul 2>&1
if errorlevel 1 (
    call :print_warn "未找到Ninja，将使用默认生成器"
)

call :print_info "环境检查完成"
call :print_info "  NDK路径: %ANDROID_NDK_HOME%"
for /f "tokens=*" %%i in ('cmake --version ^| findstr /r "cmake version"') do set "CMAKE_VERSION=%%i"
call :print_info "  %CMAKE_VERSION%"
call :print_info "  构建类型: %BUILD_TYPE%"
call :print_info "  Android平台: %ANDROID_PLATFORM%"
goto :eof

:build_abi
set "abi=%~1"
call :print_info "开始构建 %abi%..."

:: 构建目录名格式：android-{abi}-{type}
if "%BUILD_TYPE%"=="Debug" (
    set "build_suffix=debug"
) else (
    set "build_suffix=release"
)
set "build_dir=..\build\android-%abi%-%build_suffix%"

:: 创建构建目录
if not exist "%build_dir%" mkdir "%build_dir%"

:: 配置CMake
pushd "%build_dir%"

set "cmake_args=-DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake -DANDROID_ABI=%abi% -DANDROID_PLATFORM=%ANDROID_PLATFORM% -DANDROID_STL=%ANDROID_STL% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=..\..\build\install\%abi% -G Ninja"

:: 如果vcpkg已配置，添加工具链
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        set "cmake_args=!cmake_args! -DVCPKG_TARGET_TRIPLET=%abi%-android"
    )
)

call :print_info "配置CMake..."
cmake !cmake_args! ..\android
if errorlevel 1 (
    popd
    call :print_error "CMake配置失败"
    exit /b 1
)

:: 构建
call :print_info "开始编译..."
ninja
if errorlevel 1 (
    popd
    call :print_error "编译失败"
    exit /b 1
)

:: 安装
call :print_info "安装到输出目录..."
ninja install
if errorlevel 1 (
    popd
    call :print_error "安装失败"
    exit /b 1
)

popd
call :print_info "%abi% 构建完成"
goto :eof

:main
:: 解析命令行参数
set "build_abis="
set "clean_build=false"

:parse_args
if "%~1"=="" goto :args_done

if "%~1"=="-h" goto :show_help
if "%~1"=="--help" goto :show_help
if "%~1"=="-a" (
    set "build_abis=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--abi" (
    set "build_abis=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="-t" (
    set "BUILD_TYPE=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--type" (
    set "BUILD_TYPE=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="-p" (
    set "ANDROID_PLATFORM=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--platform" (
    set "ANDROID_PLATFORM=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--all" (
    set "build_abis=%ABIS%"
    shift
    goto :parse_args
)
if "%~1"=="--clean" (
    set "clean_build=true"
    shift
    goto :parse_args
)

call :print_error "未知参数: %~1"
call :show_help
exit /b 1

:args_done
:: 如果没有指定ABI，使用默认ABI
if "%build_abis%"=="" set "build_abis=%DEFAULT_ABIS%"

:: 清理构建目录
if "%clean_build%"=="true" (
    call :print_info "清理构建目录..."
    if exist "..\build" (
        cd ..\
        for /d %%i in (build\android-*) do (
            call :print_info "删除: %%i"
            rmdir /s /q "%%i" 2>nul
        )
        cd scripts
    )
)

:: 检查环境
call :check_environment

:: 记录开始时间
for /f "tokens=2 delims==" %%i in ('wmic os get localdatetime /value') do set "datetime=%%i"
set "start_time=%datetime%"

:: 构建指定的ABI
for %%a in (%build_abis%) do call :build_abi %%a

:: 记录结束时间
for /f "tokens=2 delims==" %%i in ('wmic os get localdatetime /value') do set "datetime=%%i"
set "end_time=%datetime%"

call :print_info "构建完成！"

:: 显示输出文件
call :print_info "输出文件:"
for %%a in (%build_abis%) do (
    set "lib_path=build-android\install\%%a\lib\libEngine.so"
    if exist "!lib_path!" (
        for %%i in ("!lib_path!") do set "file_size=%%~zi"
        call :print_info "  %%a: !lib_path!"
    )
)

goto :eof

:show_help
call :show_help
exit /b 0

:: 执行主函数
call :main %*