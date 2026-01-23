@echo off
REM Prisma Engine Windows Build Script
REM Usage: build-windows.bat [preset] [options...]
REM   preset: windows-x64-debug (default), windows-x64-release, windows-x86-debug, windows-x86-release
REM   options:
REM     clean     - Clean build directory before building
REM     quiet     - Reduce output (only show errors and important messages)
REM     verbose   - Show full build output

setlocal enabledelayedexpansion

REM Default preset
set PRESET=windows-x64-debug
set CLEAN_BUILD=0
set QUIET_MODE=0
set VERBOSE_MODE=0

REM Parse arguments
:parse_args
if "%~1"=="" goto :args_done
if "%~1"=="clean" (
    set CLEAN_BUILD=1
    shift
    goto :parse_args
)
if "%~1"=="--quiet" (
    set QUIET_MODE=1
    shift
    goto :parse_args
)
if "%~1"=="-q" (
    set QUIET_MODE=1
    shift
    goto :parse_args
)
if "%~1"=="--verbose" (
    set VERBOSE_MODE=1
    shift
    goto :parse_args
)
if "%~1"=="-v" (
    set VERBOSE_MODE=1
    shift
    goto :parse_args
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
    echo   clean, --clean         Clean build directory before building
    echo   --quiet, -q            Reduce output (only show errors and progress)
    echo   --verbose, -v          Show full build output
    echo   --help                 Show this help message
    echo.
    echo Examples:
    echo   build-windows.bat windows-x64-debug
    echo   build-windows.bat windows-x64-release clean
    echo   build-windows.bat windows-x64-debug --quiet
    echo.
    exit /b 0
)
REM If it's not a known option, treat it as preset
set PRESET=%~1
shift
goto :parse_args

:args_done

REM Set output level based on mode
if %QUIET_MODE%==1 (
    set "OUTPUT_LEVEL=--quiet"
    set "CMAKE_QUIET=--log-level=ERROR"
) else if %VERBOSE_MODE%==1 (
    set "OUTPUT_LEVEL=--verbose"
    set "CMAKE_QUIET=--log-level=VERBOSE"
) else (
    set "OUTPUT_LEVEL="
    set "CMAKE_QUIET=--log-level=STATUS"
)

REM Only show header if not quiet
if %QUIET_MODE%==0 (
    echo ====================================
    echo Prisma Engine Windows Build Script
    echo ====================================
    echo Preset: %PRESET%
    echo.
)

REM Check if CMake is available
where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    exit /b 1
)

REM Clean build directory if requested
if %CLEAN_BUILD%==1 (
    if %QUIET_MODE%==0 echo Cleaning build directory...
    if exist build\%PRESET% (
        rmdir /s /q build\%PRESET% 2>nul
    )
)

REM Configure
if %QUIET_MODE%==0 echo [1/2] Configuring with preset: %PRESET%
cmake --preset %PRESET% %CMAKE_QUIET%
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM Build
if %QUIET_MODE%==0 echo [2/2] Building with preset: %PRESET%
if %QUIET_MODE%==1 (
    cmake --build --preset %PRESET% --output-mode=errors-only
) else if %VERBOSE_MODE%==1 (
    cmake --build --preset %PRESET% --verbose
) else (
    cmake --build --preset %PRESET%
)
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

REM Success message
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
