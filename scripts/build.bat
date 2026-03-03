@echo off
REM Prisma Engine Unified Build Script (Windows)
REM This script detects the platform and calls the appropriate build script

setlocal enabledelayedexpansion

REM Get the directory of this script
set SCRIPT_DIR=%~dp0

REM Parse arguments
set PRESET=%~1
set CLEAN_BUILD=%~2

echo ====================================
echo Prisma Engine Build Script
echo ====================================
echo.

REM Check preset and route to appropriate script
if "%PRESET%"=="" (
    echo Usage: build.bat [preset] [clean]
    echo.
    echo New Preset Format: {target}-{platform}-{arch}-{build_type}
    echo.
    echo Windows Engine Presets:
    echo   engine-windows-x64-debug      ^(default^)
    echo   engine-windows-x64-release
    echo.
    echo Windows Editor Presets:
    echo   editor-windows-x64-debug
    echo   editor-windows-x64-release
    echo.
    echo Windows Runtime Presets:
    echo   runtime-windows-x64-debug
    echo   runtime-windows-x64-release
    echo.
    echo Linux Engine Presets:
    echo   engine-linux-x64-debug
    echo   engine-linux-x64-release
    echo   engine-linux-arm64-debug
    echo   engine-linux-arm64-release
    echo.
    echo Linux Editor Presets:
    echo   editor-linux-x64-debug
    echo   editor-linux-x64-release
    echo.
    echo Android Engine Presets:
    echo   engine-android-arm64-debug
    echo   engine-android-arm64-release
    echo.
    echo Android Runtime Presets:
    echo   runtime-android-arm64-debug
    echo   runtime-android-arm64-release
    echo.
    echo Examples:
    echo   build.bat engine-windows-x64-debug
    echo   build.bat editor-windows-x64-debug clean
    echo   build.bat engine-android-arm64-debug
    echo.
    goto :eof
)

REM Check if it's an Android preset
echo %PRESET% | findstr /i "android" >nul
if %errorlevel% == 0 (
    echo Detected Android preset, calling Android build script...
    call "%SCRIPT_DIR%build-android.bat" %PRESET% %CLEAN_BUILD%
    goto :eof
)

REM Otherwise, use Windows build script
echo Detected Windows preset, calling Windows build script...
call "%SCRIPT_DIR%build-windows.bat" %PRESET% %CLEAN_BUILD%

endlocal
