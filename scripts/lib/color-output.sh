#!/bin/bash
# PrismaEngine 彩色输出库
# 提供统一的彩色输出函数

# 颜色定义
export PRISMA_COLOR_RED='\033[0;31m'
export PRISMA_COLOR_GREEN='\033[0;32m'
export PRISMA_COLOR_YELLOW='\033[1;33m'
export PRISMA_COLOR_BLUE='\033[0;34m'
export PRISMA_COLOR_MAGENTA='\033[0;35m'
export PRISMA_COLOR_CYAN='\033[0;36m'
export PRISMA_COLOR_WHITE='\033[1;37m'
export PRISMA_COLOR_GRAY='\033[0;37m'
export PRISMA_COLOR_NC='\033[0m' # No Color

# 粗体颜色
export PRISMA_COLOR_BOLD_RED='\033[1;31m'
export PRISMA_COLOR_BOLD_GREEN='\033[1;32m'
export PRISMA_COLOR_BOLD_YELLOW='\033[1;33m'
export PRISMA_COLOR_BOLD_BLUE='\033[1;34m'
export PRISMA_COLOR_BOLD_MAGENTA='\033[1;35m'
export PRISMA_COLOR_BOLD_CYAN='\033[1;36m'

# 背景色
export PRISMA_COLOR_BG_RED='\033[41m'
export PRISMA_COLOR_BG_GREEN='\033[42m'
export PRISMA_COLOR_BG_YELLOW='\033[43m'
export PRISMA_COLOR_BG_BLUE='\033[44m'

# 检测终端是否支持颜色
function color_supported() {
    # 检查 NO_COLOR 环境变量
    if [ -n "$NO_COLOR" ]; then
        return 1
    fi

    # 检查是否在终端中
    if [ ! -t 1 ]; then
        return 1
    fi

    return 0
}

# 根据终端支持情况选择是否使用颜色
function print_color() {
    local color="$1"
    shift
    local message="$*"

    if color_supported; then
        echo -e "${color}${message}${PRISMA_COLOR_NC}"
    else
        echo "$message"
    fi
}

# 彩色输出函数
function print_red() {
    print_color "$PRISMA_COLOR_RED" "$@"
}

function print_green() {
    print_color "$PRISMA_COLOR_GREEN" "$@"
}

function print_yellow() {
    print_color "$PRISMA_COLOR_YELLOW" "$@"
}

function print_blue() {
    print_color "$PRISMA_COLOR_BLUE" "$@"
}

function print_magenta() {
    print_color "$PRISMA_COLOR_MAGENTA" "$@"
}

function print_cyan() {
    print_color "$PRISMA_COLOR_CYAN" "$@"
}

function print_gray() {
    print_color "$PRISMA_COLOR_GRAY" "$@"
}

# 特殊输出函数
function print_header() {
    print_color "$PRISMA_COLOR_CYAN" ""
    print_color "$PRISMA_COLOR_CYAN" "===================================="
    print_color "$PRISMA_COLOR_CYAN" "$1"
    print_color "$PRISMA_COLOR_CYAN" "===================================="
}

function print_step() {
    print_color "$PRISMA_COLOR_YELLOW" ""
    print_color "$PRISMA_COLOR_YELLOW" "[$1]"
}

function print_success() {
    print_color "$PRISMA_COLOR_GREEN" "✓ $1"
}

function print_error() {
    print_color "$PRISMA_COLOR_RED" "✗ ERROR: $1" >&2
}

function print_warning() {
    print_color "$PRISMA_COLOR_YELLOW" "⚠ WARNING: $1"
}

function print_info() {
    print_color "$PRISMA_COLOR_BLUE" "ℹ $1"
}

# 进度条函数
function print_progress() {
    local current=$1
    local total=$2
    local width=50

    if color_supported; then
        local percent=$((current * 100 / total))
        local filled=$((current * width / total))
        local empty=$((width - filled))

        printf "\r["
        printf "%${filled}s" | tr ' ' '='
        printf "%${empty}s" | tr ' ' ' '
        printf "] %d%%" $percent
    fi

    if [ $current -eq $total ]; then
        echo ""
    fi
}

# 状态指示器
function spin() {
    local spinner="/-\|"
    local i=0

    while kill -0 $1 2>/dev/null; do
        i=$(( (i + 1) %4 ))
        if color_supported; then
            printf "\r${spinner:$i:1} $2"
        fi
        sleep 0.1
    done
}

# 表格输出
function print_table() {
    local delimiter="${1:-|}"
    local header_line=1

    IFS=$'\n' read -d '' -ra lines
    for line in "${lines[@]}"; do
        if [ $header_line -eq 1 ]; then
            print_cyan "$line"
            header_line=0
        else
            echo "$line"
        fi
    done
}

# 调试输出（仅在 DEBUG 模式下显示）
function print_debug() {
    if [ "$DEBUG" = "1" ]; then
        print_color "$PRISMA_COLOR_GRAY" "[DEBUG] $*" >&2
    fi
}

# 导出所有函数供其他脚本使用
export -f color_supported
export -f print_color
export -f print_red
export -f print_green
export -f print_yellow
export -f print_blue
export -f print_magenta
export -f print_cyan
export -f print_gray
export -f print_header
export -f print_step
export -f print_success
export -f print_error
export -f print_warning
export -f print_info
export -f print_progress
export -f print_debug
