#!/bin/bash
# Prisma Engine Editor Build Script (Linux)
# Usage: ./build-editor.sh [preset] [options...]
#
# Available presets:
#   - editor-linux-arm64-debug (default on ARM64)
#   - editor-linux-arm64-release
#   - editor-linux-x64-debug (default on x64)
#   - editor-linux-x64-release

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default values
ARCH=$(uname -m)
PRESET=""
CLEAN_BUILD=false
QUIET_MODE=false
VERBOSE_MODE=false
PARALLEL_JOBS="4"  # 默认使用 4 个并行任务，避免内存不足

# Parse arguments - 先解析选项，最后才是 preset
while [[ $# -gt 0 ]]; do
    case $1 in
        --quiet|-q)
            QUIET_MODE=true
            ;;
        --verbose|-v)
            VERBOSE_MODE=true
            ;;
        --clean)
            CLEAN_BUILD=true
            ;;
        --jobs|-j)
            shift
            PARALLEL_JOBS="$1"
            # 验证输入是数字
            if ! [[ "$PARALLEL_JOBS" =~ ^[0-9]+$ ]] || [ "$PARALLEL_JOBS" -lt 1 ]; then
                echo -e "${RED}ERROR: --jobs requires a positive number${NC}"
                exit 1
            fi
            ;;
        --help|-h)
            echo "Usage: ./build-editor.sh [preset] [options...]"
            echo ""
            echo "Presets:"
            echo "  editor-linux-arm64-debug      (default on ARM64)"
            echo "  editor-linux-arm64-release"
            echo "  editor-linux-x64-debug       (default on x64)"
            echo "  editor-linux-x64-release"
            echo ""
            echo "Options:"
            echo "  --quiet, -q            Reduce output"
            echo "  --verbose, -v          Show full build output"
            echo "  --clean                Clean build directory before building"
            echo "  --jobs, -j <N>         Number of parallel jobs (default: 4)"
            echo "  --help, -h             Show this help message"
            echo ""
            echo "Examples:"
            echo "  ./build-editor.sh editor-linux-arm64-debug"
            echo "  ./build-editor.sh editor-linux-arm64-release --clean"
            echo "  ./build-editor.sh --quiet editor-linux-arm64-debug"
            echo "  ./build-editor.sh -q editor-linux-x64-debug"
            echo "  ./build-editor.sh --jobs 2 editor-linux-arm64-debug"
            echo "  ./build-editor.sh -j 2 editor-linux-arm64-debug"
            echo ""
            exit 0
            ;;
        -*)
            echo "Unknown option: $1"
            echo "Use --help to see available options"
            exit 1
            ;;
        *)
            # 第一个非选项参数就是 preset
            if [ -z "$PRESET" ]; then
                PRESET="$1"
            else
                echo "Error: Too many arguments"
                exit 1
            fi
            ;;
    esac
    shift
done

# 如果没有指定 preset，使用默认值
if [ -z "$PRESET" ]; then
    if [ "$ARCH" = "aarch64" ]; then
        PRESET="editor-linux-arm64-debug"
    else
        PRESET="editor-linux-x64-debug"
    fi
fi

# Set CMake output mode
if [ "$QUIET_MODE" = true ]; then
    CMAKE_LOG_LEVEL="--log-level=ERROR"
    BUILD_QUIET_FLAG="--"
elif [ "$VERBOSE_MODE" = true ]; then
    CMAKE_LOG_LEVEL="--log-level=VERBOSE"
    BUILD_QUIET_FLAG="--verbose"
else
    CMAKE_LOG_LEVEL="--log-level=STATUS"
    BUILD_QUIET_FLAG=""
fi

function print_header() {
    if [ "$QUIET_MODE" = false ]; then
        echo ""
        echo -e "${CYAN}====================================${NC}"
        echo -e "${CYAN}$1${NC}"
        echo -e "${CYAN}====================================${NC}"
    fi
}

function print_step() {
    if [ "$QUIET_MODE" = false ]; then
        echo ""
        echo -e "${YELLOW}$1${NC}"
    fi
}

function check_cmake() {
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}ERROR: CMake not found${NC}"
        echo "Please install CMake: sudo apt install cmake"
        exit 1
    fi
}

function clean_build() {
    if [ "$QUIET_MODE" = false ]; then
        echo ""
        echo -e "${YELLOW}Cleaning build directory...${NC}"
    fi
    BUILD_DIR="build/${PRESET}"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        if [ "$QUIET_MODE" = false ]; then
            echo -e "Removed: ${BUILD_DIR}"
        fi
    fi
}

function get_preset_config() {
    case "$PRESET" in
        editor-linux-arm64-debug)
            BUILD_TYPE="Debug"
            BUILD_SHARED="ON"
            ;;
        editor-linux-arm64-release)
            BUILD_TYPE="Release"
            BUILD_SHARED="OFF"
            ;;
        editor-linux-x64-debug)
            BUILD_TYPE="Debug"
            BUILD_SHARED="ON"
            ;;
        editor-linux-x64-release)
            BUILD_TYPE="Release"
            BUILD_SHARED="OFF"
            ;;
        *)
            echo -e "${RED}ERROR: Unknown preset: ${PRESET}${NC}"
            echo "Available presets:"
            echo "  - editor-linux-arm64-debug"
            echo "  - editor-linux-arm64-release"
            echo "  - editor-linux-x64-debug"
            echo "  - editor-linux-x64-release"
            exit 1
            ;;
    esac
}

# Script main
print_header "Prisma Engine Editor Build Script (Linux)"
if [ "$QUIET_MODE" = false ]; then
    echo -e "${GREEN}Preset: ${PRESET}${NC}"
    echo -e "${GREEN}Architecture: ${ARCH}${NC}"
    echo -e "${GREEN}Parallel jobs: ${PARALLEL_JOBS}${NC}"
fi

# Check CMake
check_cmake

# Get preset config
get_preset_config

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    clean_build
fi

# Configure
print_step "[1/2] Configuring with preset: ${PRESET}"
cmake --preset "$PRESET" $CMAKE_LOG_LEVEL

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: CMake configuration failed${NC}"
    exit 1
fi

# Build
print_step "[2/2] Building with preset: ${PRESET}"
cmake --build --preset "$PRESET" -j${PARALLEL_JOBS} $BUILD_QUIET_FLAG

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Build failed${NC}"
    exit 1
fi

# Success message
if [ "$QUIET_MODE" = true ]; then
    echo -e "${GREEN}Build completed: build/${PRESET}${NC}"
else
    print_header "Build completed successfully!"
    echo -e "${GREEN}Output directory: build/${PRESET}${NC}"
    echo -e "${GREEN}Editor binary: build/${PRESET}/bin/PrismaRuntime${NC}"
    echo -e "${GREEN}Editor library: build/${PRESET}/lib/libPrismaEditor.so${NC}"
    echo ""
fi
