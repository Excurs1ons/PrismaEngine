#!/bin/bash
# Prisma Engine Linux Build Script
# Usage: ./build-linux.sh [preset] [options...]
#
# New Preset Format: {target}-{platform}-{arch}-{build_type}
#
# Available presets:
#   - engine-linux-x64-debug (default)
#   - engine-linux-x64-release
#   - engine-linux-arm64-debug
#   - engine-linux-arm64-release
#   - editor-linux-x64-debug
#   - editor-linux-x64-release
#   - runtime-linux-x64-debug
#   - runtime-linux-x64-release
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
PRESET="${1:-engine-linux-x64-debug}"
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
            echo "Preset Format: {target}-{platform}-{arch}-{build_type}"
            echo ""
            echo "Available Presets:"
            echo ""
            echo "Engine Presets:"
            echo "  engine-linux-x64-debug           (default)"
            echo "  engine-linux-x64-release"
            echo "  engine-linux-arm64-debug"
            echo "  engine-linux-arm64-release"
            echo ""
            echo "Editor Presets:"
            echo "  editor-linux-x64-debug"
            echo "  editor-linux-x64-release"
            echo ""
            echo "Runtime Presets:"
            echo "  runtime-linux-x64-debug"
            echo "  runtime-linux-x64-release"
            echo ""
            echo "Options:"
            echo "  --quiet, -q            Reduce output (only show errors and progress)"
            echo "  --verbose, -v          Show full build output"
            echo "  --clean                Clean build directory before building"
            echo "  --help                 Show this help message"
            echo ""
            echo "Examples:"
            echo "  ./build-linux.sh engine-linux-x64-debug"
            echo "  ./build-linux.sh editor-linux-x64-debug --quiet"
            echo "  ./build-linux.sh engine-linux-arm64-debug --clean"
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

# Map preset to build directory and configure preset
case "$PRESET" in
    engine-linux-x64-debug|engine-linux-x64-release|engine-linux-arm64-debug|engine-linux-arm64-release|editor-linux-x64-debug|editor-linux-x64-release|runtime-linux-x64-debug|runtime-linux-x64-release)
        BUILD_DIR="build/${PRESET}"
        ;;
    *)
        echo -e "${RED}ERROR: Unknown preset: ${PRESET}${NC}"
        echo "Available presets:"
        echo "  - engine-linux-x64-debug (default)"
        echo "  - engine-linux-x64-release"
        echo "  - engine-linux-arm64-debug"
        echo "  - engine-linux-arm64-release"
        echo "  - editor-linux-x64-debug"
        echo "  - editor-linux-x64-release"
        echo "  - runtime-linux-x64-debug"
        echo "  - runtime-linux-x64-release"
        exit 1
        ;;
esac

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

# Configure
print_step "[1/2] Configuring with preset: ${PRESET}"
cmake --preset "$PRESET" $CMAKE_LOG_LEVEL

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: CMake configuration failed${NC}"
    exit 1
fi

# Build
print_step "[2/2] Building with preset: ${PRESET}"
cmake --build --preset "$PRESET" $CMAKE_OUTPUT_MODE

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
