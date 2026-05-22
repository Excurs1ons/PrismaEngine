#!/bin/bash
# package-sdk.sh — PrismaEngine SDK 打包脚本
# 在 CI/Release 时运行: 构建引擎 → 收集产物 → 打包 → 生成 Release 归档
#
# 用法:
#   ./package-sdk.sh 0.1.0                              # 全部平台
#   ./package-sdk.sh 0.1.0 --platforms linux,windows     # 指定平台
#   ./package-sdk.sh 0.1.0 --skip-build                  # 跳过构建（只用已有产物）
#
# 输出:
#   dist/PrismaEngine-SDK-<version>-<platform>.tar.gz
#   dist/PrismaEngine-SDK-<version>-<platform>.sha256
#
# 上传到 GitHub Release 后, 用户解压并:
#   cmake -B build -DPrismaEngine_DIR=/path/to/sdk

set -e

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

VERSION="${1:-0.1.0}"
PLATFORMS="linux,windows,android"
OUTPUT_DIR="$PROJECT_ROOT/dist"
SKIP_BUILD=false

shift 2>/dev/null || true
while [[ $# -gt 0 ]]; do
    case $1 in
        --platforms|-p) PLATFORMS="$2"; shift 2 ;;
        --output|-o)    OUTPUT_DIR="$2"; shift 2 ;;
        --skip-build)   SKIP_BUILD=true; shift ;;
        --help|-h)
            echo "用法: $0 [version] [options]"
            echo "  version           SDK 版本 (默认: 0.1.0)"
            echo "  --platforms, -p   平台列表 (默认: linux,windows,android)"
            echo "  --output, -o      输出目录 (默认: dist/)"
            echo "  --skip-build      跳过 cmake 构建"
            exit 0 ;;
        *) echo -e "${RED}未知: $1${NC}"; exit 1 ;;
    esac
done

print_header() { echo -e "\n${CYAN}====================================${NC}\n${CYAN}$1${NC}\n${CYAN}====================================${NC}"; }
print_step()  { echo -e "\n${YELLOW}$1${NC}"; }

print_header "PrismaEngine SDK v${VERSION} Packager"
echo -e "  Platforms: ${PLATFORMS}"
echo -e "  Output:    ${OUTPUT_DIR}"
echo -e "  SkipBuild: ${SKIP_BUILD}"
mkdir -p "$OUTPUT_DIR"

# ──────────────────────────────────────────────
# Step 1: Build Engine for Each Platform
# ──────────────────────────────────────────────
if [ "$SKIP_BUILD" = false ]; then
    print_step "[1/5] Building engine for each platform"
    IFS=',' read -ra PLATFORM_ARRAY <<< "$PLATFORMS"
    for platform in "${PLATFORM_ARRAY[@]}"; do
        platform=$(echo "$platform" | xargs)
        echo "  Building for: $platform"

        case "$platform" in
            linux)
                for preset in linux-x64-debug linux-x64-release linux-arm64-debug; do
                    if cmake --preset "$preset" 2>/dev/null; then
                        cmake --build "build/$preset" --target Engine -j"$(nproc)"
                    fi
                done
                ;;
            windows)
                for preset in windows-x64-debug windows-x64-release; do
                    if cmake --preset "$preset" 2>/dev/null; then
                        cmake --build "build/$preset" --target Engine
                    fi
                done
                ;;
            android)
                if cmake --preset engine-android-arm64-debug 2>/dev/null; then
                    cmake --build "build/engine-android-arm64-debug" --target Engine
                fi
                ;;
        esac
    done
else
    print_step "[1/5] Skip build (--skip-build)"
fi

# ──────────────────────────────────────────────
# Step 2: Package SDK for Each Platform
# ──────────────────────────────────────────────
print_step "[2/5] Packaging SDK per platform"

# Source directories from repo (committed)
REPO_SDK_INCLUDE="$PROJECT_ROOT/sdk/include"
REPO_SDK_CMAKE="$PROJECT_ROOT/sdk/cmake"
REPO_SAMPLES="$PROJECT_ROOT/sdk/samples"
REPO_DOCS="$PROJECT_ROOT/sdk/docs"

IFS=',' read -ra PLATFORM_ARRAY <<< "$PLATFORMS"
for platform in "${PLATFORM_ARRAY[@]}"; do
    platform=$(echo "$platform" | xargs)
    echo "  Packaging for: $platform"

    SDK_DIR="$OUTPUT_DIR/PrismaEngine-SDK-${VERSION}-${platform}"
    rm -rf "$SDK_DIR"
    mkdir -p "$SDK_DIR/include" "$SDK_DIR/lib" "$SDK_DIR/samples" "$SDK_DIR/cmake" "$SDK_DIR/docs"

    # Copy headers from repo
    if [ -d "$REPO_SDK_INCLUDE/PrismaEngine" ]; then
        cp -r "$REPO_SDK_INCLUDE/PrismaEngine" "$SDK_DIR/include/PrismaEngine"
        echo "  Headers: $(find "$SDK_DIR/include/PrismaEngine" -name '*.h' | wc -l) files"
    else
        echo "  WARNING: No headers in sdk/include/ — run sync-sdk-headers.sh first"
    fi

    # Copy samples
    if [ -d "$REPO_SAMPLES" ]; then
        cp -r "$REPO_SAMPLES/"* "$SDK_DIR/samples/"
        echo "  Samples: copied"
    fi

    # Copy docs
    if [ -d "$REPO_DOCS" ]; then
        cp -r "$REPO_DOCS/"* "$SDK_DIR/docs/"
        echo "  Docs: copied"
    fi

    # Collect prebuilt libraries
    case "$platform" in
        linux)
            LIB_DEST="$SDK_DIR/lib/linux"
            mkdir -p "$LIB_DEST"
            for dir in build/linux-x64-debug build/linux-x64-release build/linux-arm64-debug; do
                [ -d "$PROJECT_ROOT/$dir" ] || continue
                find "$PROJECT_ROOT/$dir" \( -name "*.so*" -o -name "*.a" \) -exec cp -L {} "$LIB_DEST/" \; 2>/dev/null || true
            done
            echo "  Libs (linux): $(ls "$LIB_DEST" 2>/dev/null | wc -l) files"
            ;;

        windows)
            LIB_DEST="$SDK_DIR/lib/windows"
            mkdir -p "$LIB_DEST"
            for dir in build/windows-x64-debug build/windows-x64-release; do
                [ -d "$PROJECT_ROOT/$dir" ] || continue
                find "$PROJECT_ROOT/$dir" \( -name "*.dll" -o -name "*.lib" -o -name "*.pdb" \) -exec cp {} "$LIB_DEST/" \; 2>/dev/null || true
            done
            echo "  Libs (windows): $(ls "$LIB_DEST" 2>/dev/null | wc -l) files"
            ;;

        android)
            LIB_DEST="$SDK_DIR/lib/android"
            mkdir -p "$LIB_DEST"
            # Android .so files from Gradle build
            for abi in arm64-v8a armeabi-v7a x86_64; do
                find "$PROJECT_ROOT/projects/android" -path "*/$abi/*.so" -exec cp {} "$LIB_DEST/" \; 2>/dev/null || true
            done
            echo "  Libs (android): $(ls "$LIB_DEST" 2>/dev/null | wc -l) files"
            ;;
    esac

    # Generate platform-specific CMake config
    cat > "$SDK_DIR/cmake/PrismaEngineConfig.cmake" << SDKEOF
# PrismaEngine SDK CMake Configuration (${platform})
# Auto-generated by package-sdk.sh v${VERSION}
# GitHub: https://github.com/Excurs1ons/PrismaEngine/releases

get_filename_component(PRISMAENGINE_SDK_DIR "\${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(PRISMAENGINE_INCLUDE_DIR "\${PRISMAENGINE_SDK_DIR}/include")

# Import prebuilt library
if(NOT TARGET PrismaEngine::Engine)
    add_library(PrismaEngine::Engine SHARED IMPORTED)
    set_target_properties(PrismaEngine::Engine PROPERTIES
        IMPORTED_LOCATION "\${PRISMAENGINE_SDK_DIR}/lib/${platform}/\${CMAKE_SHARED_LIBRARY_SUFFIX}"
        INTERFACE_INCLUDE_DIRECTORIES "\${PRISMAENGINE_INCLUDE_DIR}"
    )
endif()

set(PRISMAENGINE_FOUND TRUE)
set(PRISMAENGINE_VERSION "${VERSION}")

# Helper: prisma_create_app
function(prisma_create_app APP_NAME)
    cmake_parse_arguments(ARG "" "FOLDER" "SOURCES;LIBRARIES" \${ARGN})
    add_executable(\${APP_NAME})
    if(ARG_SOURCES)
        target_sources(\${APP_NAME} PRIVATE \${ARG_SOURCES})
    endif()
    set_target_properties(\${APP_NAME} PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
    )
    target_link_libraries(\${APP_NAME} PRIVATE PrismaEngine::Engine)
    target_include_directories(\${APP_NAME} PRIVATE \${PRISMAENGINE_INCLUDE_DIR})
    if(ARG_LIBRARIES)
        target_link_libraries(\${APP_NAME} PRIVATE \${ARG_LIBRARIES})
    endif()
    if(ARG_FOLDER)
        set_target_properties(\${APP_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY \${ARG_FOLDER})
    endif()
endfunction()
SDKEOF

    # Version config
    cat > "$SDK_DIR/cmake/PrismaEngineConfigVersion.cmake" << SDKEOF
set(PACKAGE_VERSION "${VERSION}")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_VERSION VERSION_EQUAL PACKAGE_FIND_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
endif()
SDKEOF

    echo "  CMake config: generated"
done

# ──────────────────────────────────────────────
# Step 3: Generate SDK README
# ──────────────────────────────────────────────
print_step "[3/5] Generating SDK README"

for platform in "${PLATFORM_ARRAY[@]}"; do
    platform=$(echo "$platform" | xargs)
    SDK_DIR="$OUTPUT_DIR/PrismaEngine-SDK-${VERSION}-${platform}"

    cat > "$SDK_DIR/README.md" << SDKEOF
# PrismaEngine SDK v${VERSION} (${platform})

## Directory Structure
\`\`\`
PrismaEngine-SDK-${VERSION}-${platform}/
├── include/PrismaEngine/    # Public C++ headers (190+ files)
├── lib/${platform}/         # Prebuilt engine libraries
├── samples/                 # Example projects
├── cmake/                   # CMake config (find_package)
└── docs/                    # API reference
\`\`\`

## Quick Start
\`\`\`bash
# Extract SDK, then build your project:
cmake -B build -DPrismaEngine_DIR=/path/to/PrismaEngine-SDK-${VERSION}-${platform}
cmake --build build
\`\`\`

## Build a Sample
\`\`\`bash
cd samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=/path/to/PrismaEngine-SDK-${VERSION}-${platform}
cmake --build build
./build/BasicTriangle
\`\`\`

## Requirements
- CMake 3.20+
- C++20 compiler (GCC 11+, Clang 13+, MSVC 2026+)
- Vulkan SDK 1.3+ (for Vulkan backend)
- SDL3 (bundled in SDK)

## Source
https://github.com/Excurs1ons/PrismaEngine
SDKEOF
done

# ──────────────────────────────────────────────
# Step 4: Create Archives + Checksums
# ──────────────────────────────────────────────
print_step "[4/5] Creating archives and checksums"

for platform in "${PLATFORM_ARRAY[@]}"; do
    platform=$(echo "$platform" | xargs)
    SDK_DIRNAME="PrismaEngine-SDK-${VERSION}-${platform}"

    echo "  Archiving: ${SDK_DIRNAME}"
    cd "$OUTPUT_DIR"

    tar czf "${SDK_DIRNAME}.tar.gz" "$SDK_DIRNAME"
    echo "  Created: ${SDK_DIRNAME}.tar.gz ($(du -h "${SDK_DIRNAME}.tar.gz" | cut -f1))"

    # SHA256
    if command -v sha256sum &>/dev/null; then
        sha256sum "${SDK_DIRNAME}.tar.gz" > "${SDK_DIRNAME}.tar.gz.sha256"
    elif command -v shasum &>/dev/null; then
        shasum -a 256 "${SDK_DIRNAME}.tar.gz" > "${SDK_DIRNAME}.tar.gz.sha256"
    fi
    echo "  Checksum: ${SDK_DIRNAME}.tar.gz.sha256"
done

cd "$PROJECT_ROOT"

# ──────────────────────────────────────────────
# Step 5: Summary
# ──────────────────────────────────────────────
print_header "SDK Packaging Complete!"

echo -e "${GREEN}Output:${NC}"
ls -lh "$OUTPUT_DIR"/*.tar.gz 2>/dev/null
echo ""
echo -e "${GREEN}Upload to GitHub Release:${NC}"
echo "  1. Go to: https://github.com/Excurs1ons/PrismaEngine/releases/new"
echo "  2. Tag: v${VERSION}"
echo "  3. Upload these files:"
for f in "$OUTPUT_DIR"/*.tar.gz "$OUTPUT_DIR"/*.sha256; do
    [ -f "$f" ] && echo "     - $(basename "$f")"
done
echo ""
echo -e "${GREEN}Users then use:${NC}"
echo '  cmake -B build -DPrismaEngine_DIR=/path/to/sdk'
