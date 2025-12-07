@echo off
setlocal enabledelayedexpansion

echo PrismaEngine Setup Script
echo ========================

:: Check if Git is installed
git --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Git is not installed or not in PATH.
    echo Please install Git and make sure it's available in your PATH.
    pause
    exit /b 1
)

echo [INFO] Git version: && git --version

:: Check if CMake is installed
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake is not installed or not in PATH.
    echo Please install CMake and make sure it's available in your PATH.
    pause
    exit /b 1
)

echo [INFO] CMake version: && cmake --version

echo.

:: Check if vcpkg submodule directory exists and is not empty
if not exist "vcpkg\.git" (
    echo [INFO] vcpkg submodule directory not found. Initializing submodule...
    git submodule init
    if errorlevel 1 (
        echo [ERROR] Failed to initialize vcpkg submodule.
        pause
        exit /b 1
    )
    git submodule update
    if errorlevel 1 (
        echo [ERROR] Failed to update vcpkg submodule.
        pause
        exit /b 1
    )
    if not exist "vcpkg\.git" (
        echo [ERROR] vcpkg submodule initialization failed.
        pause
        exit /b 1
    )
) else (
    echo [INFO] vcpkg submodule directory found.
)

:: Check if vcpkg executable exists, if not build it
if not exist "vcpkg\vcpkg.exe" (
    echo [INFO] vcpkg executable not found. Building vcpkg...
    cd vcpkg
    call bootstrap-vcpkg.bat
    if errorlevel 1 (
        echo [ERROR] Failed to build vcpkg.
        cd ..
        pause
        exit /b 1
    )
    if not exist "vcpkg\vcpkg.exe" (
        echo [ERROR] vcpkg build failed.
        cd ..
        pause
        exit /b 1
    )
    cd ..
) else (
    echo [INFO] vcpkg executable found.
)

:: Install dependencies using vcpkg
echo [INFO] Installing dependencies from vcpkg.json...
cd vcpkg
call vcpkg.exe install --feature-flags=manifests
if errorlevel 1 (
    echo [ERROR] Failed to install dependencies with vcpkg.
    cd ..
    pause
    exit /b 1
)
cd ..

:: Set up cmake with vcpkg toolchain
echo [INFO] Setting up CMake with vcpkg toolchain...
mkdir build 2>nul
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
if errorlevel 1 (
    echo [ERROR] Failed to configure project with CMake.
    cd ..
    pause
    exit /b 1
)
cd ..

echo.
echo [SUCCESS] Project setup completed!
echo To build the project, run:
echo   cd build
echo   cmake --build .
echo.