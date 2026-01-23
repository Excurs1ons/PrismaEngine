#!/bin/bash
# Prisma Engine Android Build Script (CMake Preset-based)
# Usage: ./build-android.sh [preset] [options...]
#
# Available presets:
#   - android-arm64-v8a-debug (default)
#   - android-arm64-v8a-release
#
# Options:
#   --quiet, -q            Reduce output (only show errors and progress)
#   --verbose, -v          Show full build output
#   --clean                Clean build directory before building
#   --help                 Show this help message

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default values
PRESET="${1:-android-arm64-v8a-debug}"
CLEAN_BUILD=false
QUIET_MODE=false
VERBOSE_MODE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --quiet|-q)
            QUIET_MODE=true
            shift
            ;;
        --verbose|-v)
            VERBOSE_MODE=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --help|-h)
            echo "Usage: ./build-android.sh [preset] [options...]"
            echo ""
            echo "Presets:"
            echo "  android-arm64-v8a-debug   (default)"
            echo "  android-arm64-v8a-release"
            echo ""
            echo "Options:"
            echo "  --quiet, -q            Reduce output (only show errors and progress)"
            echo "  --verbose, -v          Show full build output"
            echo "  --clean                Clean build directory before building"
            echo "  --help                 Show this help message"
            echo ""
            echo "Examples:"
            echo "  ./build-android.sh android-arm64-v8a-debug"
            echo "  ./build-android.sh android-arm64-v8a-release --quiet"
            echo "  ./build-android.sh android-arm64-v8a-debug --clean"
            echo ""
            exit 0
            ;;
        *)
            # If it's not a known option, treat it as preset
            PRESET="$1"
            shift
            ;;
    esac
done

# Set CMake output mode
if [ "$QUIET_MODE" = true ]; then
    CMAKE_LOG_LEVEL="--log-level=ERROR"
    CMAKE_OUTPUT_MODE="--output-mode=errors-only"
elif [ "$VERBOSE_MODE" = true ]; then
    CMAKE_LOG_LEVEL="--log-level=VERBOSE"
    CMAKE_OUTPUT_MODE="--verbose"
else
    CMAKE_LOG_LEVEL="--log-level=STATUS"
    CMAKE_OUTPUT_MODE=""
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

function check_environment() {
    # Check Android NDK
    if [ -z "$ANDROID_NDK_HOME" ]; then
        echo -e "${RED}ERROR: ANDROID_NDK_HOME environment variable not set${NC}"
        echo "Please set ANDROID_NDK_HOME to your Android NDK path"
        echo ""
        echo "Example:"
        echo "  export ANDROID_NDK_HOME=/path/to/android-ndk"
        echo ""
        echo "If you have ANDROID_HOME set, NDK is usually at:"
        echo "  \$ANDROID_HOME/ndk/<version>"
        exit 1
    fi

    # Check CMake
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}ERROR: CMake not found${NC}"
        echo "Please install CMake"
        exit 1
    fi

    # Check Ninja
    if ! command -v ninja &> /dev/null; then
        echo -e "${RED}ERROR: Ninja not found${NC}"
        echo "Please install Ninja: sudo apt install ninja-build"
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

# Script main
print_header "Prisma Engine Android Build Script"
if [ "$QUIET_MODE" = false ]; then
    echo -e "${GREEN}Preset: ${PRESET}${NC}"
    echo -e "${GREEN}Android NDK: ${ANDROID_NDK_HOME}${NC}"
fi

# Check environment
check_environment

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    clean_build
fi

# Map preset to build type and ABI
case "$PRESET" in
    android-arm64-v8a-debug)
        BUILD_TYPE="Debug"
        ANDROID_ABI="arm64-v8a"
        ;;
    android-arm64-v8a-release)
        BUILD_TYPE="Release"
        ANDROID_ABI="arm64-v8a"
        ;;
    *)
        echo -e "${RED}ERROR: Unknown preset: ${PRESET}${NC}"
        echo "Available presets:"
        echo "  - android-arm64-v8a-debug (default)"
        echo "  - android-arm64-v8a-release"
        exit 1
        ;;
esac

BUILD_DIR="build/${PRESET}"

# Configure using preset-style arguments
print_step "[1/2] Configuring Android ${BUILD_TYPE} build (${ANDROID_ABI})"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_SYSTEM_NAME="Android" \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
    -DANDROID_NDK="$ANDROID_NDK_HOME/" \
    -DANDROID_PLATFORM="android-24" \
    -DANDROID_ABI="$ANDROID_ABI" \
    -DCMAKE_ANDROID_STL_TYPE="c++_shared" \
    -DPRISMA_USE_FETCHCONTENT=ON \
    -DPRISMA_ENABLE_RENDER_VULKAN=ON \
    -DPRISMA_ENABLE_RENDER_DX12=OFF \
    -DPRISMA_ENABLE_AUDIO_SDL3=ON \
    -G Ninja \
    $CMAKE_LOG_LEVEL

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: CMake configuration failed${NC}"
    exit 1
fi

# Build
print_step "[2/2] Building ${BUILD_TYPE}"
cmake --build "$BUILD_DIR" -j$(nproc) $CMAKE_OUTPUT_MODE

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Build failed${NC}"
    exit 1
fi

# Success message
if [ "$QUIET_MODE" = true ]; then
    echo -e "${GREEN}Build completed: ${BUILD_DIR}${NC}"
else
    print_header "Build completed successfully!"
    echo -e "${GREEN}Output directory: ${BUILD_DIR}${NC}"
    echo ""
fi
