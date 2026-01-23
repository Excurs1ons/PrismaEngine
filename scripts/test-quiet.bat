@echo off
echo Starting test...
echo Arg 1: %~1
echo Arg 2: %~2
echo.
cmake --version
echo.
echo Preset: windows-x64-debug
echo Building with quiet mode...
cmake --build --preset windows-x64-debug -- /v:minimal
echo.
echo Test completed.
