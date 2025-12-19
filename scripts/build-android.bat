@echo off
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
goto :eof

:check_environment
call :print_info "检查构建环境..."

if "%ANDROID_NDK_HOME%"=="" (
    if not "%ANDROID_HOME%"=="" (
        :: 尝试从ANDROID_HOME推导NDK路径
        for /d %%i in ("%ANDROID_HOME%\ndk\*") do (
            set "NDK_CANDIDATE=%%i"
        )
        if defined NDK_CANDIDATE (
            set "ANDROID_NDK_HOME=!NDK_CANDIDATE!"
            call :print_info "从ANDROID_HOME找到NDK: !ANDROID_NDK_HOME!"
        ) else (
            call :print_error "未找到NDK，请设置ANDROID_NDK_HOME或ANDROID_HOME环境变量"
            exit /b 1
        )
    ) else (
        call :print_error "请设置ANDROID_NDK_HOME或ANDROID_HOME环境变量"
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

:: 创建构建目录
set "build_dir=build-android\%abi%"
if not exist "%build_dir%" mkdir "%build_dir%"

:: 配置CMake
pushd "%build_dir%"

set "cmake_args=-DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake -DANDROID_ABI=%abi% -DANDROID_PLATFORM=%ANDROID_PLATFORM% -DANDROID_STL=%ANDROID_STL% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=..\install\%abi% -G Ninja"

:: 如果vcpkg已配置，添加工具链
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        set "cmake_args=!cmake_args! -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%abi%-android"
    )
)

call :print_info "配置CMake..."
cmake !cmake_args! ..\..\android
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
    if exist "build-android" rmdir /s /q "build-android"
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