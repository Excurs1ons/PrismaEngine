#!/bin/bash
# PrismaEngine 平台检测库
# 检测平台、架构、编译器和构建工具

# 检测结果存储在全局变量中
PRISMA_PLATFORM=""      # windows, linux, darwin, android
PRISMA_ARCH=""          # x64, x86, arm64, arm
PRISMA_COMPILER=""      # gcc, clang, msvc
PRISMA_BUILD_TOOL=""    # ninja, make, msbuild
PRISMA_ANDROID_ABI=""   # arm64-v8a, armeabi-v7a

# 颜色定义
PRISMA_COLOR_RED='\033[0;31m'
PRISMA_COLOR_GREEN='\033[0;32m'
PRISMA_COLOR_YELLOW='\033[1;33m'
PRISMA_COLOR_CYAN='\033[0;36m'
PRISMA_COLOR_NC='\033[0m' # No Color

function detect_platform() {
    # 检测操作系统
    case "$(uname -s)" in
        Linux*)  PRISMA_PLATFORM="linux" ;;
        Darwin*) PRISMA_PLATFORM="darwin" ;;
        MSYS*|MINGW*|CYGWIN*) PRISMA_PLATFORM="windows" ;;
        *)       PRISMA_PLATFORM="unknown" ;;
    esac

    # 检测架构
    case "$(uname -m)" in
        x86_64|amd64)  PRISMA_ARCH="x64" ;;
        i686|i386)    PRISMA_ARCH="x86" ;;
        aarch64|arm64) PRISMA_ARCH="arm64" ;;
        armv7l)       PRISMA_ARCH="arm" ;;
        *)            PRISMA_ARCH="unknown" ;;
    esac

    # 检测编译器
    if command -v gcc &> /dev/null; then
        PRISMA_COMPILER="gcc"
    elif command -v clang &> /dev/null; then
        PRISMA_COMPILER="clang"
    elif command -v cl.exe &> /dev/null; then
        PRISMA_COMPILER="msvc"
    fi

    # 检测构建工具
    if command -v ninja &> /dev/null; then
        PRISMA_BUILD_TOOL="ninja"
    elif command -v make &> /dev/null; then
        PRISMA_BUILD_TOOL="make"
    elif command -v msbuild &> /dev/null; then
        PRISMA_BUILD_TOOL="msbuild"
    else
        PRISMA_BUILD_TOOL="unknown"
    fi

    # 检测 Android ABI（如果在 Android 环境中）
    if [ -n "$ANDROID_ABI" ]; then
        PRISMA_ANDROID_ABI="$ANDROID_ABI"
    elif [ "$PRISMA_ARCH" = "arm64" ]; then
        PRISMA_ANDROID_ABI="arm64-v8a"
    elif [ "$PRISMA_ARCH" = "arm" ]; then
        PRISMA_ANDROID_ABI="armeabi-v7a"
    fi
}

function get_default_preset() {
    # 根据平台返回默认预设
    local build_type="${1:-debug}"

    case "$PRISMA_PLATFORM" in
        linux)
            echo "linux-${PRISMA_ARCH}-${build_type}"
            ;;
        darwin)
            echo "macos-${PRISMA_ARCH}-${build_type}"
            ;;
        windows)
            echo "windows-${PRISMA_ARCH}-${build_type}"
            ;;
        android)
            echo "android-${PRISMA_ANDROID_ABI}-${build_type}"
            ;;
        *)
            echo "unknown-${build_type}"
            ;;
    esac
}

function print_platform_info() {
    echo -e "${PRISMA_COLOR_CYAN}Platform Information:${PRISMA_COLOR_NC}"
    echo "  Platform:   $PRISMA_PLATFORM"
    echo "  Architecture: $PRISMA_ARCH"
    echo "  Compiler:     $PRISMA_COMPILER"
    echo "  Build Tool:   $PRISMA_BUILD_TOOL"
    if [ -n "$PRISMA_ANDROID_ABI" ]; then
        echo "  Android ABI:  $PRISMA_ANDROID_ABI"
    fi
}

function print_header() {
    echo ""
    echo -e "${PRISMA_COLOR_CYAN}====================================${PRISMA_COLOR_NC}"
    echo -e "${PRISMA_COLOR_CYAN}$1${PRISMA_COLOR_NC}"
    echo -e "${PRISMA_COLOR_CYAN}====================================${PRISMA_COLOR_NC}"
}

function print_step() {
    echo ""
    echo -e "${PRISMA_COLOR_YELLOW}$1${PRISMA_COLOR_NC}"
}

function print_success() {
    echo -e "${PRISMA_COLOR_GREEN}$1${PRISMA_COLOR_NC}"
}

function print_error() {
    echo -e "${PRISMA_COLOR_RED}ERROR: $1${PRISMA_COLOR_NC}" >&2
}

function print_warning() {
    echo -e "${PRISMA_COLOR_YELLOW}WARNING: $1${PRISMA_COLOR_NC}"
}

# 如果直接运行此脚本，执行检测
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    detect_platform
    print_platform_info
    echo ""
    echo "Default preset (debug): $(get_default_preset debug)"
    echo "Default preset (release): $(get_default_preset release)"
fi
