@echo off
setlocal

set GH_PATH="C:/Program Files/GitHub CLI/gh.exe"

echo 监控最新构建...
echo ===============

REM 检查 GitHub CLI
if not exist %GH_PATH% (
    echo [ERROR] GitHub CLI 未找到
    pause
    exit /b 1
)

REM 监控最新的构建运行
%GH_PATH% run watch --repo Excurs1ons/PrismaEngine

pause