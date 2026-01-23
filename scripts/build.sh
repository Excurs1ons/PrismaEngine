#!/bin/bash
# Prisma Engine Unified Build Script (Linux/macOS)
# This script detects the platform and calls the appropriate build script

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Default preset
PRESET="${1:-}"
CLEAN_BUILD="${2:-}"

function show_usage() {
    echo -e "${CYAN}Usage: ./build.sh [preset] [clean]${NC}"
    echo ""
    echo -e "${YELLOW}Linux Presets:${NC}"
    echo "  linux-x64-debug           (default, Vulkan)"
    echo "  linux-x64-release         (Vulkan)"
    echo "  linux-x64-debug-opengl    (OpenGL)"
    echo "  linux-x64-release-opengl  (OpenGL)"
    echo ""
    echo -e "${YELLOW}Android Presets:${NC}"
    echo "  android-arm64-v8a-debug"
    echo "  android-arm64-v8a-release"
    echo ""
    echo -e "${YELLOW}Examples:${NC}"
    echo "  ./build.sh linux-x64-debug"
    echo "  ./build.sh linux-x64-release clean"
    echo "  ./build.sh android-arm64-v8a-debug"
    echo ""
}

if [ -z "$PRESET" ]; then
    show_usage
    exit 0
fi

echo -e "${CYAN}====================================${NC}"
echo -e "${CYAN}Prisma Engine Build Script${NC}"
echo -e "${CYAN}====================================${NC}"
echo ""

# Check if it's an Android preset
if [[ "$PRESET" == android* ]]; then
    echo -e "${YELLOW}Detected Android preset, calling Android build script...${NC}"
    bash "$SCRIPT_DIR/build-android.sh" "$PRESET" "$CLEAN_BUILD"
else
    echo -e "${YELLOW}Detected Linux preset, calling Linux build script...${NC}"
    bash "$SCRIPT_DIR/build-linux.sh" "$PRESET" "$CLEAN_BUILD"
fi
