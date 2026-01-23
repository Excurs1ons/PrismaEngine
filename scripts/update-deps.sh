#!/bin/bash
# Prisma Engine 依赖更新脚本
# 用于安全地更新 FetchContent 依赖并验证兼容性

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

function print_header() {
    echo ""
    echo -e "${CYAN}====================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}====================================${NC}"
}

function print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

function print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

function print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 获取项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSIONS_FILE="${PROJECT_ROOT}/cmake/DependencyVersions.cmake"

print_header "Prisma Engine 依赖更新工具"

# 检查 DependencyVersions.cmake 是否存在
if [ ! -f "$VERSIONS_FILE" ]; then
    print_error "未找到版本锁定文件: $VERSIONS_FILE"
    exit 1
fi

# 显示当前锁定的版本
print_info "当前锁定的依赖版本:"
echo ""
grep -E "^set\(PRISMA_DEP_.*_VERSION" "$VERSIONS_FILE" | sed 's/set(PRISMA_DEP_/  /' | sed 's/_VERSION /: /' | sed 's/"$//' | sed 's/" "/ = /'
echo ""

# 解析命令行参数
DEP_NAME=""
NEW_VERSION=""
FORCE_UPDATE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  -h, --help              显示此帮助信息"
            echo "  -l, --list              列出所有依赖及其当前版本"
            echo "  -u, --update DEP VER    更新指定依赖到新版本"
            echo "  -c, --check             检查依赖是否有更新可用"
            echo "  -t, --test              清理构建并测试编译"
            echo "  -f, --force             强制更新（即使版本相同）"
            echo ""
            echo "示例:"
            echo "  $0 --list                          # 列出所有依赖"
            echo "  $0 --update GLM 1.0.2              # 更新 GLM 到 1.0.2"
            echo "  $0 --update SDL3 release-3.2.28    # 更新 SDL3 到 release-3.2.28"
            echo "  $0 --update STB <commit-sha>       # 更新 STB 到指定 commit"
            echo "  $0 --check                         # 检查可用更新"
            echo "  $0 --test                          # 测试编译"
            echo ""
            exit 0
            ;;
        -l|--list)
            print_info "所有依赖版本:"
            echo ""
            grep -E "^# .* - " "$VERSIONS_FILE" | grep -E "GitHub:" -B1 | sed 's/^# //' | sed 's/ #.*//' | sed 's/:$//'
            echo ""
            grep -E "^set\(PRISMA_DEP_.*_VERSION" "$VERSIONS_FILE" | while read -r line; do
                dep=$(echo "$line" | sed 's/set(PRISMA_DEP_//' | sed 's/_VERSION.*//')
                version=$(echo "$line" | sed 's/.*"\(.*\)".*/\1/')
                printf "  %-20s %s\n" "$dep:" "$version"
            done
            echo ""
            exit 0
            ;;
        -u|--update)
            DEP_NAME="$2"
            NEW_VERSION="$3"
            shift 3
            ;;
        -c|--check)
            print_info "检查依赖更新..."
            echo ""
            print_warn "此功能需要网络连接和 GitHub API"
            print_warn "目前需要手动检查，请访问:"
            echo ""
            grep "GitHub:" "$VERSIONS_FILE" | sed 's/^# GitHub: /  /'
            echo ""
            exit 0
            ;;
        -t|--test)
            print_info "测试编译..."
            cd "$PROJECT_ROOT"
            print_info "清理构建目录..."
            rm -rf build/
            print_info "配置项目..."
            cmake --preset windows-x64-debug || cmake -B build/windows-x64-debug
            print_info "编译..."
            cmake --build build/windows-x64-debug
            print_info "编译测试完成!"
            exit 0
            ;;
        -f|--force)
            FORCE_UPDATE=true
            shift
            ;;
        *)
            print_error "未知参数: $1"
            echo "使用 -h 查看帮助"
            exit 1
            ;;
    esac
done

# 如果没有指定更新，显示帮助
if [ -z "$DEP_NAME" ]; then
    echo "使用 -h 查看帮助信息"
    exit 0
fi

# 转换依赖名称为大写（用于变量名）
DEP_VAR=$(echo "$DEP_NAME" | tr '[:lower:]' '[:upper:]')
CURRENT_VERSION=$(grep "^set(PRISMA_DEP_${DEP_VAR}_VERSION" "$VERSIONS_FILE" | sed 's/.*"\(.*\)".*/\1/')

if [ -z "$CURRENT_VERSION" ]; then
    print_error "未找到依赖: $DEP_NAME"
    print_info "可用的依赖:"
    grep -E "^set\(PRISMA_DEP_.*_VERSION" "$VERSIONS_FILE" | sed 's/set(PRISMA_DEP_//' | sed 's/_VERSION.*//'
    exit 1
fi

print_info "更新依赖: $DEP_NAME"
print_info "当前版本: $CURRENT_VERSION"
print_info "新版本: $NEW_VERSION"

# 检查版本是否相同
if [ "$CURRENT_VERSION" = "$NEW_VERSION" ] && [ "$FORCE_UPDATE" = false ]; then
    print_warn "版本相同，无需更新"
    print_info "使用 --force 强制更新"
    exit 0
fi

# 确认更新
echo ""
read -p "确认更新? [y/N] " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_info "已取消"
    exit 0
fi

# 备份版本文件
BACKUP_FILE="${VERSIONS_FILE}.backup"
cp "$VERSIONS_FILE" "$BACKUP_FILE"
print_info "已备份版本文件到: $BACKUP_FILE"

# 更新版本
print_info "更新版本文件..."
sed -i "s/set(PRISMA_DEP_${DEP_VAR}_VERSION \".*\"/set(PRISMA_DEP_${DEP_VAR}_VERSION \"${NEW_VERSION}\"/" "$VERSIONS_FILE"

# 更新最后验证日期
sed -i "s/set(PRISMA_DEP_LAST_VERIFIED \".*\"/set(PRISMA_DEP_LAST_VERIFIED \"$(date +%Y-%m-%d)\"/" "$VERSIONS_FILE"

print_info "版本已更新!"
print_info "下一步:"
echo "  1. 运行清理构建: $0 --test"
echo "  2. 检查编译警告和错误"
echo "  3. 运行测试套件"
echo "  4. 如果一切正常，提交更改"
echo "  5. 如果有问题，恢复备份: cp $BACKUP_FILE $VERSIONS_FILE"
echo ""
