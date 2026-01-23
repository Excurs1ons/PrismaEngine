@echo off
REM Prisma Engine Windows Build Script
REM Usage: build-windows.bat [preset] [clean]
REM   preset: windows-x64-debug (default), windows-x64-release, windows-x86-debug, windows-x86-release
REM   clean: add "clean" as second argument to clean build directory first

setlocal enabledelayedexpansion

REM Default preset
set PRESET=windows-x64-debug
set CLEAN_BUILD=0

REM Parse arguments
if "%~1" neq "" (
    set PRESET=%~1
)
if "%~2"=="clean" (
    set CLEAN_BUILD=1
)

echo ====================================
echo Prisma Engine Windows Build Script
echo ====================================
echo Preset: %PRESET%
echo.

REM Check if CMake is available
where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    exit /b 1
)

REM Clean build directory if requested
if %CLEAN_BUILD%==1 (
    echo Cleaning build directory...
    if exist build\%PRESET% (
        rmdir /s /q build\%PRESET%
    )
)

REM Configure
echo.
echo [1/2] Configuring with preset: %PRESET%
cmake --preset %PRESET%
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM Build
echo.
echo [2/2] Building with preset: %PRESET%
cmake --build --preset %PRESET%
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo ====================================
echo Build completed successfully!
echo ====================================
echo Output directory: build\%PRESET%
echo.

endlocal
