#!/bin/bash
set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 配置参数
ANDROID_PLATFORM=${ANDROID_PLATFORM:-"24"}
BUILD_TYPE=${BUILD_TYPE:-"Release"}
ANDROID_STL=${ANDROID_STL:-"c++_shared"}

# 支持的ABI列表
ABIS=("arm64-v8a" "armeabi-v7a")
DEFAULT_ABIS=("arm64-v8a")

# 函数：打印带颜色的消息
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 函数：检查环境
check_environment() {
    print_info "检查构建环境..."

    if [ -z "$ANDROID_NDK_HOME" ]; then
        if [ -n "$ANDROID_HOME" ]; then
            # 尝试从ANDROID_HOME推导NDK路径
            NDK_CANDIDATES=$(find "$ANDROID_HOME/ndk" -maxdepth 1 -type d 2>/dev/null | sort -V | tail -1)
            if [ -n "$NDK_CANDIDATES" ]; then
                export ANDROID_NDK_HOME="$NDK_CANDIDATES"
                print_info "从ANDROID_HOME找到NDK: $ANDROID_NDK_HOME"
            else
                print_error "未找到NDK，请设置ANDROID_NDK_HOME或ANDROID_HOME环境变量"
                exit 1
            fi
        else
            print_error "请设置ANDROID_NDK_HOME或ANDROID_HOME环境变量"
            exit 1
        fi
    fi

    if [ ! -d "$ANDROID_NDK_HOME" ]; then
        print_error "NDK路径不存在: $ANDROID_NDK_HOME"
        exit 1
    fi

    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        print_error "未找到CMake，请安装CMake 3.31+"
        exit 1
    fi

    # 检查Ninja
    if ! command -v ninja &> /dev/null; then
        print_warn "未找到Ninja，将使用默认生成器"
    fi

    print_info "环境检查完成"
    print_info "  NDK路径: $ANDROID_NDK_HOME"
    print_info "  CMake版本: $(cmake --version | head -n1)"
    print_info "  构建类型: $BUILD_TYPE"
    print_info "  Android平台: $ANDROID_PLATFORM"
}

# 函数：构建单个ABI
build_abi() {
    local abi=$1
    print_info "开始构建 $abi..."

    # 创建构建目录
    local build_dir="build-android/$abi"
    mkdir -p "$build_dir"

    # 配置CMake
    cd "$build_dir"

    local cmake_args=(
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake"
        -DANDROID_ABI="$abi"
        -DANDROID_PLATFORM="$ANDROID_PLATFORM"
        -DANDROID_STL="$ANDROID_STL"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DCMAKE_INSTALL_PREFIX="../install/$abi"
        -G "Ninja"
    )

    # 如果vcpkg已配置，添加工具链
    if [ -n "$VCPKG_ROOT" ] && [ -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
        cmake_args+=(-DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake")
        cmake_args+=(-DVCPKG_TARGET_TRIPLET="$abi-android")
    fi

    print_info "配置CMake..."
    cmake "${cmake_args[@]}" ../../android

    # 构建
    print_info "开始编译..."
    if command -v ninja &> /dev/null; then
        ninja
    else
        make -j$(nproc)
    fi

    # 安装
    print_info "安装到输出目录..."
    if command -v ninja &> /dev/null; then
        ninja install
    else
        make install
    fi

    cd ../..
    print_info "$abi 构建完成"
}

# 函数：显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help              显示此帮助信息"
    echo "  -a, --abi ABI           指定ABI架构 (可选，默认: arm64-v8a)"
    echo "  -t, --type TYPE         构建类型 (Debug|Release, 默认: Release)"
    echo "  -p, --platform PLATFORM Android平台版本 (默认: 24)"
    echo "  --all                   构建所有支持的ABI"
    echo "  --clean                 清理构建目录"
    echo ""
    echo "环境变量:"
    echo "  ANDROID_NDK_HOME        Android NDK路径"
    echo "  ANDROID_HOME            Android SDK路径"
    echo "  VCPKG_ROOT              vcpkg根目录 (可选)"
    echo ""
    echo "支持的ABI: ${ABIS[*]}"
    echo ""
    echo "注意: 已移除x86和x86_64架构，因为ARM架构在Android设备中占主导地位"
}

# 主函数
main() {
    # 解析命令行参数
    local build_abis=()
    local clean_build=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -a|--abi)
                build_abis=("$2")
                shift 2
                ;;
            -t|--type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -p|--platform)
                ANDROID_PLATFORM="$2"
                shift 2
                ;;
            --all)
                build_abis=("${ABIS[@]}")
                shift
                ;;
            --clean)
                clean_build=true
                shift
                ;;
            *)
                print_error "未知参数: $1"
                show_help
                exit 1
                ;;
        esac
    done

    # 如果没有指定ABI，使用默认ABI
    if [ ${#build_abis[@]} -eq 0 ]; then
        build_abis=("${DEFAULT_ABIS[@]}")
    fi

    # 检查ABI是否有效
    for abi in "${build_abis[@]}"; do
        if [[ ! " ${ABIS[@]} " =~ " ${abi} " ]]; then
            print_error "不支持的ABI: $abi"
            print_info "支持的ABI: ${ABIS[*]}"
            exit 1
        fi
    done

    # 清理构建目录
    if [ "$clean_build" = true ]; then
        print_info "清理构建目录..."
        rm -rf build-android
    fi

    # 检查环境
    check_environment

    # 构建指定的ABI
    local start_time=$(date +%s)

    for abi in "${build_abis[@]}"; do
        build_abi "$abi"
    done

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    print_info "构建完成！"
    print_info "总耗时: ${duration}秒"

    # 显示输出文件
    print_info "输出文件:"
    for abi in "${build_abis[@]}"; do
        local lib_path="build-android/install/$abi/lib/libEngine.so"
        if [ -f "$lib_path" ]; then
            local size=$(du -h "$lib_path" | cut -f1)
            print_info "  $abi: $lib_path ($size)"
        fi
    done
}

# 执行主函数
main "$@"