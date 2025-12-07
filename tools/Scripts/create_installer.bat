@echo off
REM 构建并打包Prisma Engine

REM 设置变量
set BUILD_DIR=%~dp0..\..\build
set INSTALL_DIR=%~dp0..\..\install
set INNO_SETUP_SCRIPT=%~dp0PrismaEngine.iss

REM 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

REM 使用CMake配置项目
cd /d "%BUILD_DIR%"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%"

REM 构建项目
cmake --build . --config Release

REM 安装文件到指定目录
cmake --install . --config Release

REM 如果有Inno Setup，创建安装程序
if exist "%INNO_SETUP_SCRIPT%" (
    ISCC.exe "%INNO_SETUP_SCRIPT%"
    echo 安装程序创建完成
) else (
    echo 要创建Windows安装程序，请安装Inno Setup并创建相应的.iss脚本
)

echo 打包完成，文件位于: %INSTALL_DIR%