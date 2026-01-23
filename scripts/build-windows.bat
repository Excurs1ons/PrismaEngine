@echo off
setlocal enabledelayedexpansion

REM Default values
set "PRESET=windows-x64-debug"
set "CLEAN_BUILD=0"
set "QUIET_MODE=0"
set "VERBOSE_MODE=0"

REM Parse all arguments
:parse_loop
if "%~1"=="" goto :parse_done
if /i "%~1"=="clean" (
    set "CLEAN_BUILD=1"
    shift
    goto :parse_loop
)
if "%~1"=="--quiet" (
    set "QUIET_MODE=1"
    shift
    goto :parse_loop
)
if "%~1"=="-q" (
    set "QUIET_MODE=1"
    shift
    goto :parse_loop
)
if "%~1"=="--verbose" (
    set "VERBOSE_MODE=1"
    shift
    goto :parse_loop
)
if "%~1"=="-v" (
    set "VERBOSE_MODE=1"
    shift
    goto :parse_loop
)
if "%~1"=="--help" (
    echo Usage: build-windows.bat [preset] [options...]
    echo.
    echo Presets:
    echo   windows-x64-debug      ^(default^)
    echo   windows-x64-release
    echo   windows-x86-debug
    echo   windows-x86-release
    echo.
    echo Options:
    echo   clean                  Clean build directory before building
    echo   --quiet, -q            Quiet mode ^(only show errors^)
    echo   --verbose, -v          Verbose mode
    echo   --help                 Show this help
    echo.
    echo Examples:
    echo   build-windows.bat windows-x64-debug
    echo   build-windows.bat windows-x64-release clean
    echo   build-windows.bat windows-x64-debug -q
    echo.
    exit /b 0
)
REM Treat as preset name
set "PRESET=%~1"
shift
goto :parse_loop

:parse_done

REM Set CMake options
if %QUIET_MODE%==1 (
    set "CMAKE_LOG_LEVEL=--log-level=ERROR"
    set "MSBUILD_VERBOSITY=/v:minimal"
) else if %VERBOSE_MODE%==1 (
    set "CMAKE_LOG_LEVEL=--log-level=VERBOSE"
    set "MSBUILD_VERBOSITY=/v:detailed"
) else (
    set "CMAKE_LOG_LEVEL=--log-level=STATUS"
    set "MSBUILD_VERBOSITY=/v:normal"
)

REM Show header only if not quiet
if %QUIET_MODE%==0 (
    echo ====================================
    echo Prisma Engine Windows Build Script
    echo ====================================
    echo Preset: %PRESET%
    echo.
)

REM Check CMake
where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    exit /b 1
)
echo CMake found, proceeding with build...

REM Clean if requested
if %CLEAN_BUILD%==1 (
    if %QUIET_MODE%==0 echo Cleaning build directory...
    if exist "build\%PRESET%" rmdir /s /q "build\%PRESET%" 2>nul
)

REM Configure
if %QUIET_MODE%==0 echo [1/2] Configuring with preset: %PRESET%
cmake --preset %PRESET% %CMAKE_LOG_LEVEL%
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM Build
if %QUIET_MODE%==0 echo [2/2] Building with preset: %PRESET%
echo Executing: cmake --build --preset %PRESET% -- %MSBUILD_VERBOSITY%
cmake --build --preset %PRESET% -- %MSBUILD_VERBOSITY%
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)
echo Build command completed successfully.

REM Success
if %QUIET_MODE%==0 (
    echo.
    echo ====================================
    echo Build completed successfully!
    echo ====================================
    echo Output directory: build\%PRESET%
    echo.
) else (
    echo Build completed: build\%PRESET%
)

endlocal
