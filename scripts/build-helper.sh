#!/bin/bash
# PrismaEngine 构建辅助脚本
# 用于快速构建引擎和示例项目

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

function show_usage() {
    echo "PrismaEngine 构建辅助工具"
    echo ""
    echo "用法: ./build-helper.sh [命令] [选项]"
    echo ""
    echo "命令:"
    echo "  all                 构建所有组件（引擎 + 编辑器 + 示例）"
    echo "  engine              仅构建引擎"
    echo "  editor              构建引擎和编辑器"
    echo "  samples             构建示例项目"
    echo "  sdk                 打包 SDK"
    echo "  clean               清理构建目录"
    echo "  help                显示此帮助"
    echo ""
    echo "选项:"
    echo "  --preset <name>     构建预设 (默认: linux-x64-debug)"
    echo "  --clean             清理后重新构建"
    echo ""
    echo "示例:"
    echo "  ./build-helper.sh all"
    echo "  ./build-helper.sh all --preset linux-x64-release"
    echo "  ./build-helper.sh engine --clean"
}

function build_engine() {
    local preset="${1:-linux-x64-debug}"
    local clean="${2:-false}"

    echo -e "${CYAN}=== 构建引擎 ===${NC}"
    bash "$SCRIPT_DIR/build-linux.sh" "$preset" $([ "$clean" == "true" ] && echo "--clean")
}

function build_editor() {
    local preset="${1:-linux-x64-debug}"
    local clean="${2:-false}"

    echo -e "${CYAN}=== 构建编辑器 ===${NC}"
    # 编辑器与引擎一起构建
    build_engine "$preset" "$clean"
}

function build_samples() {
    local preset="${1:-linux-x64-debug}"
    local clean="${2:-false}"

    echo -e "${CYAN}=== 构建示例项目 ===${NC}"

    local build_dir="$PROJECT_ROOT/build/$preset"
    if [ "$clean" == "true" ]; then
        echo "清理示例项目..."
        find "$PROJECT_ROOT/sdk/samples" -name "build" -type d -exec rm -rf {} + 2>/dev/null || true
    fi

    # 构建所有示例
    for sample_dir in "$PROJECT_ROOT/sdk/samples"/*; do
        if [ -d "$sample_dir" ] && [ -f "$sample_dir/CMakeLists.txt" ]; then
            local sample_name="$(basename "$sample_dir")"
            echo -e "${YELLOW}构建 $sample_name...${NC}"

            local sample_build="$sample_dir/build"
            mkdir -p "$sample_build"

            cmake -B "$sample_build" \
                -DCMAKE_BUILD_TYPE=Debug \
                -DPrismaEngine_DIR="$build_dir" \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

            cmake --build "$sample_build"
        fi
    done
}

function build_sdk() {
    echo -e "${CYAN}=== 打包 SDK ===${NC}"
    bash "$SCRIPT_DIR/package-sdk.sh" "0.1.0"
}

function clean_all() {
    echo -e "${CYAN}=== 清理构建目录 ===${NC}"
    rm -rf "$PROJECT_ROOT/build"
    find "$PROJECT_ROOT/sdk/samples" -name "build" -type d -exec rm -rf {} + 2>/dev/null || true
    echo -e "${GREEN}清理完成${NC}"
}

# 主逻辑
COMMAND="${1:-help}"
shift 2>/dev/null || true

PRESET="linux-x64-debug"
CLEAN_BUILD=false

# 解析选项
while [[ $# -gt 0 ]]; do
    case $1 in
        --preset)
            PRESET="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        *)
            echo -e "${YELLOW}未知选项: $1${NC}"
            show_usage
            exit 1
            ;;
    esac
done

case "$COMMAND" in
    all)
        echo -e "${CYAN}构建所有组件 (预设: $PRESET)${NC}"
        build_engine "$PRESET" "$CLEAN_BUILD"
        build_samples "$PRESET" "$CLEAN_BUILD"
        echo -e "${GREEN}=== 构建完成 ===${NC}"
        ;;
    engine)
        build_engine "$PRESET" "$CLEAN_BUILD"
        ;;
    editor)
        build_editor "$PRESET" "$CLEAN_BUILD"
        ;;
    samples)
        build_samples "$PRESET" "$CLEAN_BUILD"
        ;;
    sdk)
        build_sdk
        ;;
    clean)
        clean_all
        ;;
    help|--help|-h)
        show_usage
        ;;
    *)
        echo -e "${YELLOW}未知命令: $COMMAND${NC}"
        show_usage
        exit 1
        ;;
esac
