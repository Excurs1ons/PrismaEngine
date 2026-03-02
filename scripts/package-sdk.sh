#!/bin/bash
# PrismaEngine SDK 打包脚本
# 用法: ./package-sdk.sh [version] [options]
#
# 示例:
#   ./package-sdk.sh 0.1.0
#   ./package-sdk.sh 0.1.0 --platforms linux,windows,android

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 默认值
VERSION="${1:-0.1.0}"
PLATFORMS="linux,windows,android"
OUTPUT_DIR="$PROJECT_ROOT/dist"
CLEAN_BUILD=false

# 解析参数
shift 2>/dev/null || true
while [[ $# -gt 0 ]]; do
    case $1 in
        --platforms|-p)
            PLATFORMS="$2"
            shift 2
            ;;
        --output|-o)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --help|-h)
            echo "用法: $0 [version] [options]"
            echo ""
            echo "参数:"
            echo "  version              SDK 版本号 (默认: 0.1.0)"
            echo ""
            echo "选项:"
            echo "  --platforms, -p      要打包的平台 (默认: linux,windows,android)"
            echo "  --output, -o         输出目录 (默认: dist/)"
            echo "  --clean              清理后重新构建"
            echo "  --help, -h           显示此帮助"
            echo ""
            echo "示例:"
            echo "  $0 0.1.0"
            echo "  $0 0.1.0 --platforms linux,windows"
            echo "  $0 0.1.0 -o /tmp/sdk-output"
            exit 0
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            exit 1
            ;;
    esac
done

function print_header() {
    echo ""
    echo -e "${CYAN}====================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}====================================${NC}"
}

function print_step() {
    echo ""
    echo -e "${YELLOW}$1${NC}"
}

print_header "PrismaEngine SDK 打包工具"
echo -e "${GREEN}版本: ${VERSION}${NC}"
echo -e "${GREEN}平台: ${PLATFORMS}${NC}"
echo -e "${GREEN}输出: ${OUTPUT_DIR}${NC}"

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# SDK 目录结构
SDK_DIR="$OUTPUT_DIR/PrismaEngine-SDK-${VERSION}"
INCLUDE_DIR="$SDK_DIR/include"
LIB_DIR="$SDK_DIR/lib"
SAMPLES_DIR="$SDK_DIR/samples"
CMAKE_DIR="$SDK_DIR/cmake"
DOCS_DIR="$SDK_DIR/docs"

print_step "[1/6] 创建 SDK 目录结构"
mkdir -p "$INCLUDE_DIR/PrismaEngine"
mkdir -p "$LIB_DIR"
mkdir -p "$SAMPLES_DIR"
mkdir -p "$CMAKE_DIR"
mkdir -p "$DOCS_DIR"

print_step "[2/6] 复制公共头文件"
echo "复制引擎头文件..."
find "$PROJECT_ROOT/src/engine" -name "*.h" -type f | while read -r header; do
    # 保持目录结构
    relative_path="${header#$PROJECT_ROOT/src/engine/}"
    dest_dir="$INCLUDE_DIR/PrismaEngine/$(dirname "$relative_path")"
    mkdir -p "$dest_dir"
    cp "$header" "$dest_dir/"
done

echo "复制编辑器头文件..."
find "$PROJECT_ROOT/src/editor" -name "*.h" -type f | while read -r header; do
    relative_path="${header#$PROJECT_ROOT/src/editor/}"
    dest_dir="$INCLUDE_DIR/PrismaEngine/Editor/$(dirname "$relative_path")"
    mkdir -p "$dest_dir"
    cp "$header" "$dest_dir/"
done

# 创建统一导出头文件
cat > "$INCLUDE_DIR/PrismaEngine.h" << 'EOF'
#pragma once

// PrismaEngine SDK 统一头文件

// 核心系统
#include "PrismaEngine/Engine.h"
#include "PrismaEngine/Logger.h"
#include "PrismaEngine/Platform.h"

// 应用程序接口
#include "PrismaEngine/IApplication.h"

// 渲染系统
#include "PrismaEngine/graphic/RenderSystemNew.h"

// 资源管理
#include "PrismaEngine/core/AssetManager.h"

// 音频系统
#include "PrismaEngine/audio/AudioAPI.h"
#include "PrismaEngine/audio/IAudioDevice.h"

// 输入系统
#include "PrismaEngine/input/InputManager.h"

// 编辑器 (可选)
#ifdef PRISMA_EDITOR
#include "PrismaEngine/Editor/Editor.h"
#endif
EOF

print_step "[3/6] 收集预编译库"
IFS=',' read -ra PLATFORM_ARRAY <<< "$PLATFORMS"
for platform in "${PLATFORM_ARRAY[@]}"; do
    platform=$(echo "$platform" | xargs) # 去除空格
    echo "处理平台: $platform"

    case "$platform" in
        linux)
            BUILD_DIRS=("build/linux-x64-debug" "build/linux-x64-release")
            for build_dir in "${BUILD_DIRS[@]}"; do
                if [ -d "$PROJECT_ROOT/$build_dir" ]; then
                    echo "  复制 $build_dir"
                    mkdir -p "$LIB_DIR/linux/x64"
                    # 复制静态库和动态库
                    find "$PROJECT_ROOT/$build_dir" -name "*.a" -exec cp {} "$LIB_DIR/linux/x64/" \; 2>/dev/null || true
                    find "$PROJECT_ROOT/$build_dir" -name "*.so*" -exec cp {} "$LIB_DIR/linux/x64/" \; 2>/dev/null || true
                fi
            done
            ;;
        windows)
            BUILD_DIRS=("build/windows-x64-debug" "build/windows-x64-release")
            for build_dir in "${BUILD_DIRS[@]}"; do
                if [ -d "$PROJECT_ROOT/$build_dir" ]; then
                    echo "  复制 $build_dir"
                    mkdir -p "$LIB_DIR/windows/x64"
                    find "$PROJECT_ROOT/$build_dir" -name "*.lib" -exec cp {} "$LIB_DIR/windows/x64/" \; 2>/dev/null || true
                    find "$PROJECT_ROOT/$build_dir" -name "*.dll" -exec cp {} "$LIB_DIR/windows/x64/" \; 2>/dev/null || true
                fi
            done
            ;;
        android)
            BUILD_DIRS=("projects/android/PrismaAndroid/build/outputs")
            if [ -d "$PROJECT_ROOT/$BUILD_DIRS" ]; then
                echo "  复制 Android 库"
                mkdir -p "$LIB_DIR/android"
                find "$PROJECT_ROOT/$BUILD_DIRS" -name "*.aar" -exec cp {} "$LIB_DIR/android/" \; 2>/dev/null || true
                find "$PROJECT_ROOT/$BUILD_DIRS" -name "*.so" -exec cp {} "$LIB_DIR/android/" \; 2>/dev/null || true
            fi
            ;;
        *)
            echo -e "${YELLOW}  警告: 未知平台 $platform${NC}"
            ;;
    esac
done

print_step "[4/6] 生成 CMake 配置文件"
cat > "$CMAKE_DIR/PrismaEngineConfig.cmake" << EOF
# PrismaEngine SDK CMake 配置文件
# 版本: ${VERSION}

@PACKAGE_INIT@

# 查找依赖
include(CMakeFindDependencyMacro)

# 查找 Vulkan (如果启用)
if(PRISMA_ENABLE_RENDER_VULKAN)
    find_dependency(Vulkan REQUIRED)
endif()

# 查找 SDL3
find_dependency(SDL3 REQUIRED)

# 包含目标
include("\${CMAKE_CURRENT_LIST_DIR}/PrismaEngineTargets.cmake")

check_required_components(PrismaEngine)
EOF

cat > "$CMAKE_DIR/PrismaEngineConfigVersion.cmake" << EOF
set(PACKAGE_VERSION "${VERSION}")

if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    if(PACKAGE_VERSION VERSION_EQUAL PACKAGE_FIND_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
endif()
EOF

print_step "[5/6] 复制示例项目"
if [ -d "$PROJECT_ROOT/sdk/samples" ]; then
    cp -r "$PROJECT_ROOT/sdk/samples/"* "$SAMPLES_DIR/"
else
    echo "  警告: 未找到示例项目目录"
fi

print_step "[6/6] 生成文档"
cat > "$DOCS_DIR/QuickStart.md" << EOF
# PrismaEngine SDK 快速入门指南

## 版本 ${VERSION}

### 环境要求

- CMake 3.20+
- C++20 编译器
  - Windows: MSVC 2022+
  - Linux: GCC 11+ 或 Clang 13+
  - Android: NDK r25+
- Vulkan SDK (如果使用 Vulkan 后端)
- SDL3 (包含在 SDK 中)

### 安装

1. 解压 SDK 到任意目录
2. 设置环境变量 \`PrismaEngine_DIR\` 指向 SDK 目录
   - Linux/macOS: \`export PrismaEngine_DIR=/path/to/sdk\`
   - Windows: \`set PrismaEngine_DIR=C:\\path\\to\\sdk\`

### 创建项目

创建 \`CMakeLists.txt\`:

\`\`\`cmake
cmake_minimum_required(VERSION 3.20)
project(MyGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PrismaEngine REQUIRED)

add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE PrismaEngine::Engine)
\`\`\`

创建 \`src/main.cpp\`:

\`\`\`cpp
#include <PrismaEngine/PrismaEngine.h>

using namespace PrismaEngine;

class MyGame : public IApplication<MyGame> {
public:
    bool Initialize() override {
        LOG_INFO("Game", "游戏初始化");
        return true;
    }

    int Run() override {
        // 游戏主循环
        return 0;
    }

    void Shutdown() override {
        LOG_INFO("Game", "游戏关闭");
    }
};

int main() {
    MyGame game;
    game.Initialize();
    return game.Run();
}
\`\`\`

### 构建和运行

\`\`\`bash
mkdir build && cd build
cmake ..
cmake --build .
./MyGame  # Linux/macOS
# 或
MyGame.exe  # Windows
\`\`\`

### 示例项目

查看 \`samples/\` 目录获取更多示例：
- BasicTriangle - 最小的可运行示例
- AssetLoading - 资源加载演示
- InputHandling - 输入处理演示

### 更多信息

- API 参考: \`docs/APIReference.md\`
- 平台支持: \`docs/PlatformSupport.md\`
EOF

# 创建 README
cat > "$SDK_DIR/README.md" << EOF
# PrismaEngine SDK v${VERSION}

## 目录结构

\`\`\`
PrismaEngine-SDK-${VERSION}/
├── include/           # 头文件
│   └── PrismaEngine/
├── lib/               # 预编译库
│   ├── linux/
│   ├── windows/
│   └── android/
├── samples/           # 示例项目
├── cmake/             # CMake 配置文件
└── docs/              # 文档
\`\`\`

## 快速开始

参见 \`docs/QuickStart.md\` 获取详细说明。

## 许可证

本 SDK 使用以下许可证：
- PrismaEngine: MIT License
- 第三方依赖: 各自的许可证

## 支持

- GitHub: https://github.com/Excurs1ons/PrismaEngine
- 文档: https://prismaengine.dev (待上线)
EOF

print_header "SDK 打包完成！"
echo -e "${GREEN}输出目录: ${SDK_DIR}${NC}"
echo ""
echo "下一步："
echo "  1. 测试 SDK: cd ${SAMPLES_DIR}/BasicTriangle && cmake -B build -DPrismaEngine_DIR=${SDK_DIR}"
echo "  2. 分发 SDK: tar czf ${OUTPUT_DIR}/PrismaEngine-SDK-${VERSION}.tar.gz -C ${OUTPUT_DIR} PrismaEngine-SDK-${VERSION}"
echo ""
