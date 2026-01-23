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
    echo Windows Presets:
    echo   windows-x64-debug      ^(default^)
    echo   windows-x64-release
    echo   windows-x86-debug
    echo   windows-x86-release
    echo.
    echo Android Presets:
    echo   android-arm64-v8a-debug
    echo   android-arm64-v8a-release
    echo.
    echo Examples:
    echo   build.bat windows-x64-debug
    echo   build.bat windows-x64-release clean
    echo   build.bat android-arm64-v8a-debug
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
