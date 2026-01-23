#!/bin/bash
# Prisma Engine Linux Build Script
# Usage: ./build-linux.sh [preset] [options...]
#
# Available presets:
#   - linux-x64-debug (default) - Vulkan backend
#   - linux-x64-release - Vulkan backend
#   - linux-x64-debug-opengl - OpenGL backend
#   - linux-x64-release-opengl - OpenGL backend
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
PRESET="${1:-linux-x64-debug}"
CLEAN_BUILD=false
QUIET_MODE=false
VERBOSE_MODE=false

# Parse arguments
shift_count=0
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
            echo "Usage: ./build-linux.sh [preset] [options...]"
            echo ""
            echo "Presets:"
            echo "  linux-x64-debug           (default, Vulkan)"
            echo "  linux-x64-release         (Vulkan)"
            echo "  linux-x64-debug-opengl    (OpenGL)"
            echo "  linux-x64-release-opengl  (OpenGL)"
            echo ""
            echo "Options:"
            echo "  --quiet, -q            Reduce output (only show errors and progress)"
            echo "  --verbose, -v          Show full build output"
            echo "  --clean                Clean build directory before building"
            echo "  --help                 Show this help message"
            echo ""
            echo "Examples:"
            echo "  ./build-linux.sh linux-x64-debug"
            echo "  ./build-linux.sh linux-x64-release --quiet"
            echo "  ./build-linux.sh linux-x64-debug --clean"
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

# Script main
print_header "Prisma Engine Linux Build Script"
if [ "$QUIET_MODE" = false ]; then
    echo -e "${GREEN}Preset: ${PRESET}${NC}"
fi

# Check CMake
check_cmake

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
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
print_step "[1/2] Configuring Linux ${BUILD_TYPE} build (${RENDER_BACKEND})"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DPRISMA_ENABLE_RENDER_VULKAN="$([ "$RENDER_BACKEND" = "Vulkan" ] && echo "ON" || echo "OFF")" \
    -DPRISMA_ENABLE_RENDER_OPENGL="$([ "$RENDER_BACKEND" = "OpenGL" ] && echo "ON" || echo "OFF")" \
    -DPRISMA_ENABLE_AUDIO_SDL3=ON \
    -DPRISMA_ENABLE_INPUT_SDL3=ON \
    -DPRISMA_USE_FETCHCONTENT=ON \
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
