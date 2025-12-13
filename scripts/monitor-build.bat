@echo off
setlocal

echo GitHub Actions 构建状态监控
echo ============================

REM 设置 GitHub CLI 路径
set GH_PATH="C:/Program Files/GitHub CLI/gh.exe"

REM 检查 GitHub CLI 是否安装
if not exist %GH_PATH% (
    echo [ERROR] GitHub CLI 未安装，请先运行以下命令安装:
    echo winget install GitHub.cli
    pause
    exit /b 1
)

REM 检查是否已登录
%GH_PATH% auth status >nul 2>&1
if errorlevel 1 (
    echo [INFO] 请先登录 GitHub...
    %GH_PATH% auth login
    if errorlevel 1 (
        echo [ERROR] 登录失败
        pause
        exit /b 1
    )
)

REM 运行 PowerShell 监控脚本
powershell -ExecutionPolicy Bypass -File "%~dp0monitor-build.ps1" %*

pause