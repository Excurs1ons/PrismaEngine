#!/bin/bash
# Prisma Engine 依赖状态检查脚本
# 检查 FetchContent 缓存状态和依赖完整性

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
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
BUILD_DIR="${PROJECT_ROOT}/build"
DEPS_BASE_DIR="${BUILD_DIR}/_deps"

print_header "Prisma Engine 依赖状态检查"

# 检查构建目录
if [ ! -d "$BUILD_DIR" ]; then
    print_error "构建目录不存在: $BUILD_DIR"
    print_info "请先运行配置脚本"
    exit 1
fi

print_info "依赖缓存位置: $DEPS_BASE_DIR"
echo ""

# 列出已下载的依赖
if [ -d "$DEPS_BASE_DIR" ]; then
    print_info "已缓存的依赖:"
    echo ""
    for dep_dir in "$DEPS_BASE_DIR"/*; do
        if [ -d "$dep_dir" ]; then
            dep_name=$(basename "$dep_dir")
            src_dir="${dep_dir}/sub"

            if [ -d "$src_dir" ]; then
                # 获取 Git 信息
                if [ -d "${src_dir}/.git" ]; then
                    cd "$src_dir"
                    commit=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
                    branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
                    tag=$(git describe --tags --exact-match 2>/dev/null || echo "")

                    # 获取目录大小
                    size=$(du -sh "$dep_dir" 2>/dev/null | cut -f1)

                    echo -e "  ${CYAN}${dep_name}${NC}"
                    echo "    Commit:  $commit"
                    echo "    Branch:  $branch"
                    [ -n "$tag" ] && echo "    Tag:     $tag"
                    echo "    Size:    $size"
                    echo ""
                fi
            fi
        fi
    done
else
    print_warn "未找到依赖缓存目录"
    print_info "可能还没有配置项目"
fi

# 检查版本锁定文件
VERSIONS_FILE="${PROJECT_ROOT}/cmake/DependencyVersions.cmake"
if [ -f "$VERSIONS_FILE" ]; then
    print_header "版本锁定状态"
    print_info "最后验证: $(grep 'PRISMA_DEP_LAST_VERIFIED' "$VERSIONS_FILE" | sed 's/.*"\(.*\)".*/\1/')"
    print_info "依赖矩阵版本: $(grep 'PRISMA_DEP_MATRIX_VERSION' "$VERSIONS_FILE" | sed 's/.*"\(.*\)".*/\1/')"
    echo ""

    # 检查是否有使用 master 分支的依赖
    print_warn "检查使用分支名称的依赖:"
    MASTER_DEPS=$(grep -E 'set\(PRISMA_DEP_.*_VERSION "(master|main|docking)"' "$VERSIONS_FILE" || true)
    if [ -n "$MASTER_DEPS" ]; then
        echo "$MASTER_DEPS" | sed 's/^/  /'
        echo ""
        print_warn "建议: 将这些依赖锁定到具体的 commit SHA"
    else
        print_info "所有依赖都已锁定到具体版本"
    fi
fi

# 显示更新策略
print_header "更新策略配置"
if grep -q "FETCHCONTENT_UPDATES_DISCONNECTED ON" "${PROJECT_ROOT}/cmake/FetchThirdPartyDeps.cmake"; then
    print_info "自动更新: ${YELLOW}已禁用${NC} (FETCHCONTENT_UPDATES_DISCONNECTED=ON)"
    print_info "依赖版本将被锁定，除非手动更新"
else
    print_warn "自动更新: ${GREEN}已启用${NC} (FETCHCONTENT_UPDATES_DISCONNECTED=OFF)"
    print_warn "警告: 依赖版本可能会在重新配置时更新"
fi

# 提供操作建议
print_header "操作建议"
echo ""
echo "更新依赖版本:"
echo "  ./scripts/update-deps.sh --list                    # 列出所有依赖"
echo "  ./scripts/update-deps.sh --update GLM 1.0.2        # 更新指定依赖"
echo ""
echo "测试依赖更新:"
echo "  ./scripts/update-deps.sh --test                   # 清理并重新编译"
echo ""
echo "清理依赖缓存:"
echo "  rm -rf build/_deps                                # 删除所有缓存"
echo "  rm -rf build/_deps/SDL3-src                       # 删除特定依赖"
echo ""
echo "强制重新下载:"
echo "  cmake --preset windows-x64-debug -DFETCHCONTENT_UPDATES_DISCONNECTED=OFF"
echo ""
