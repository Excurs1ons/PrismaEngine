#!/bin/bash
# Prisma Engine Linux Build Script
# Usage: ./build-linux.sh [preset] [clean]
#
# Available presets:
#   - linux-x64-debug (default) - Vulkan backend
#   - linux-x64-release - Vulkan backend
#   - linux-x64-debug-opengl - OpenGL backend
#   - linux-x64-release-opengl - OpenGL backend

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default preset
PRESET="${1:-linux-x64-debug}"
CLEAN_BUILD="${2:-}"

function print_header() {
    echo ""
    echo -e "${CYAN}====================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}====================================${NC}"
}

function check_cmake() {
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}ERROR: CMake not found${NC}"
        echo "Please install CMake: sudo apt install cmake"
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
print_header "Prisma Engine Linux Build Script"
echo -e "${GREEN}Preset: ${PRESET}${NC}"

# Check CMake
check_cmake

# Clean build directory if requested
if [ "$CLEAN_BUILD" = "clean" ]; then
    clean_build
fi

# Map preset to actual build directory
case "$PRESET" in
    linux-x64-debug)
        BUILD_TYPE="Debug"
        RENDER_BACKEND="Vulkan"
        BUILD_DIR="build/linux-x64-debug"
        ;;
    linux-x64-release)
        BUILD_TYPE="Release"
        RENDER_BACKEND="Vulkan"
        BUILD_DIR="build/linux-x64-release"
        ;;
    linux-x64-debug-opengl)
        BUILD_TYPE="Debug"
        RENDER_BACKEND="OpenGL"
        BUILD_DIR="build/linux-x64-debug-opengl"
        ;;
    linux-x64-release-opengl)
        BUILD_TYPE="Release"
        RENDER_BACKEND="OpenGL"
        BUILD_DIR="build/linux-x64-release-opengl"
        ;;
    *)
        echo -e "${RED}ERROR: Unknown preset: ${PRESET}${NC}"
        echo "Available presets:"
        echo "  - linux-x64-debug (default)"
        echo "  - linux-x64-release"
        echo "  - linux-x64-debug-opengl"
        echo "  - linux-x64-release-opengl"
        exit 1
        ;;
esac

# Configure
echo ""
echo -e "${YELLOW}[1/2] Configuring Linux ${BUILD_TYPE} build (${RENDER_BACKEND})${NC}"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DPRISMA_ENABLE_RENDER_VULKAN="$([ "$RENDER_BACKEND" = "Vulkan" ] && echo "ON" || echo "OFF")" \
    -DPRISMA_ENABLE_RENDER_OPENGL="$([ "$RENDER_BACKEND" = "OpenGL" ] && echo "ON" || echo "OFF")" \
    -DPRISMA_ENABLE_AUDIO_SDL3=ON \
    -DPRISMA_ENABLE_INPUT_SDL3=ON \
    -DPRISMA_USE_FETCHCONTENT=ON

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
