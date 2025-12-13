@echo off
setlocal enabledelayedexpansion

set GH_PATH="C:/Program Files/GitHub CLI/gh.exe"

echo.
echo ==========================================
echo   Prisma Engine GitHub Actions 状态
echo ==========================================
echo.

REM 检查 GitHub CLI
if not exist %GH_PATH% (
    echo [ERROR] GitHub CLI 未安装
    echo 请运行: winget install GitHub.cli
    pause
    exit /b 1
)

REM 获取最新的构建状态
for /f "tokens=*" %%i in ('%GH_PATH% run list --repo Excurs1ons/PrismaEngine --limit 1 --json status,conclusion,displayTitle ^| %GH_PATH% api --jq ".[0]" 2^>nul') do set result=%%i

REM 解析 JSON（简化版）
echo %result% | findstr /C:"\"status\":\"in_progress\"" >nul
if !errorlevel! equ 0 (
    echo 状态: [进行中] - 蓝色
    color 0B
    goto :show_details
)

echo %result% | findstr /C:"\"conclusion\":\"success\"" >nul
if !errorlevel! equ 0 (
    echo 状态: [成功] - 绿色
    color 0A
    goto :show_details
)

echo %result% | findstr /C:"\"conclusion\":\"failure\"" >nul
if !errorlevel! equ 0 (
    echo 状态: [失败] - 红色
    color 0C
    goto :show_details
)

echo 状态: [等待中] - 黄色
color 0E

:show_details
echo.
echo 最新构建:
echo ----------------------------------------
%GH_PATH% run list --repo Excurs1ons/PrismaEngine --limit 3 --branch main
echo.

echo 快捷操作:
echo ----------------------------------------
echo 1. 监控当前构建:     watch-build.bat
echo 2. 查看详细日志:     gh run view
echo 3. 打开构建页面:     gh browse --actions

pause