#!/bin/bash
# Prisma Engine Android Build Script (CMake Preset-based)
# Usage: ./build-android.sh [preset] [clean]
#
# Available presets:
#   - android-arm64-v8a-debug (default)
#   - android-arm64-v8a-release

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default preset
PRESET="${1:-android-arm64-v8a-debug}"
CLEAN_BUILD="${2:-}"

function print_header() {
    echo ""
    echo -e "${CYAN}====================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}====================================${NC}"
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
    echo ""
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    BUILD_DIR="build/${PRESET}"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        echo -e "Removed: ${BUILD_DIR}"
    fi
}

# Script main
print_header "Prisma Engine Android Build Script"
echo -e "${GREEN}Preset: ${PRESET}${NC}"
echo -e "${GREEN}Android NDK: ${ANDROID_NDK_HOME}${NC}"

# Check environment
check_environment

# Clean build directory if requested
if [ "$CLEAN_BUILD" = "clean" ]; then
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
echo ""
echo -e "${YELLOW}[1/2] Configuring Android ${BUILD_TYPE} build (${ANDROID_ABI})${NC}"
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
    -G Ninja

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: CMake configuration failed${NC}"
    exit 1
fi

# Build
echo ""
echo -e "${YELLOW}[2/2] Building ${BUILD_TYPE}${NC}"
cmake --build "$BUILD_DIR" -j$(nproc)

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Build failed${NC}"
    exit 1
fi

print_header "Build completed successfully!"
echo -e "${GREEN}Output directory: ${BUILD_DIR}${NC}"
echo ""
