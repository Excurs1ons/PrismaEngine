# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64--lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Status](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions)

## Prune Branch

此分支是 Prisma Engine 的简化版本，专注于 SDL3 + Vulkan 渲染路径。

**简化内容：**
- ✅ 支持 **SDL3 + Vulkan** 可适配的平台（按 preset 配置）
- ✅ 渲染后端：仅启用 **Vulkan**
- ✅ 音频后端：仅启用 **SDL3**
- ✅ 编辑器：集成 **ImGui**
- ❌ 移除 DirectX 12 后端
- ❌ 移除 OpenGL 后端
- ❌ 移除 XAudio2 后端
- ❌ 移除 DirectX12 / OpenGL / XAudio2 路径

**目标：** 简化引擎架构，专注 Vulkan 渲染管线开发。

---

Prisma Engine is a cross-platform 3D game engine built with modern C++20, focusing on high-performance rendering and modern graphics architectures.

> **Current Status**: Windows Vulkan backend is production-ready with integrated ImGui editor.

## CI/CD Status

### CI (Continuous Integration)

| Platform | Status | Trigger |
|----------|--------|---------|
| Windows | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-windows.yml?branch=prune&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml) | Push / PR |

### Editor Build

| Platform | Status | Trigger |
|----------|--------|---------|
| Windows | [![Editor](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-editor.yml?branch=prune&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-editor.yml) | Push / Manual |

## Current Progress

| Module | Status | Description |
|--------|--------|-------------|
| ECS Component System | ✅ 80% | High-performance Entity Component System |
| Vulkan Backend | ✅ 90% | Robust Windows implementation |
| Platform Layer | ✅ 95% | Windows abstraction |
| Logger System | ✅ 100% | Thread-safe cross-platform logging |
| Audio System (SDL3) | ✅ 50% | SDL3 backend with 3D spatial support |
| Resource Management | ✅ 75% | Smart asset loading and caching |
| Editor Tools (ImGui) | ⏳ 30% | ImGui integrated inspector |

**Overall: ~70%**

## SDK Usage

Use PrismaEngine SDK to build your game without embedding the engine source code.

### One-Click Project Setup

```bash
# Replace VERSION with SDK version (e.g., 1.0.1)
SDK_VERSION=1.0.1

# Download SDK with project template
curl -sL https://github.com/Excurs1ons/PrismaEngine/releases/download/v${SDK_VERSION}-sdk/PrismaEngine-SDK-${SDK_VERSION}-linux-arm64.tar.gz | tar xz

# Move template to your project directory
mv PrismaEngine-SDK-${SDK_VERSION}-linux-arm64/template/* .
mv PrismaEngine-SDK-${SDK_VERSION}-linux-arm64/template/.* . 2>/dev/null || true
rm -rf PrismaEngine-SDK-${SDK_VERSION}-linux-arm64

# Build your game
cmake -B build
cmake --build build

# Run
./build/MyGame
```

### Manual Integration

If you prefer to set up manually:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyGame VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Download PrismaEngine SDK
set(PRISMA_SDK_VERSION "1.0.1")
set(PRISMA_SDK_URL "https://github.com/Excurs1ons/PrismaEngine/releases/download/v${PRISMA_SDK_VERSION}-sdk/PrismaEngine-SDK-${PRISMA_SDK_VERSION}-linux-arm64.tar.gz")

set(PRISMA_SDK_DIR "${CMAKE_BINARY_DIR}/PrismaEngine-SDK")
if(NOT EXISTS "${PRISMA_SDK_DIR}/cmake/PrismaEngineConfig.cmake")
    message(STATUS "Downloading PrismaEngine SDK...")
    file(DOWNLOAD "${PRISMA_SDK_URL}" "${CMAKE_BINARY_DIR}/PrismaEngine-SDK.tar.gz" SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_BINARY_DIR}/PrismaEngine-SDK.tar.gz"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    file(REMOVE "${CMAKE_BINARY_DIR}/PrismaEngine-SDK.tar.gz")
    file(RENAME "${CMAKE_BINARY_DIR}/PrismaEngine-SDK-${PRISMA_SDK_VERSION}-linux-arm64" "${PRISMA_SDK_DIR}")
endif()

list(APPEND CMAKE_PREFIX_PATH "${PRISMA_SDK_DIR}")
find_package(PrismaEngine REQUIRED)

add_executable(MyGame src/main.cpp src/MyApp.h)
target_include_directories(MyGame PRIVATE ${PRISMA_SDK_DIR}/include src)
target_link_libraries(MyGame PRIVATE PrismaEngine::Engine)
target_link_directories(MyGame PRIVATE "${PRISMA_SDK_DIR}/lib/linux")
```

### SDK Requirements

- CMake 3.20+
- C++20 compiler
- Vulkan SDK (`libvulkan-dev` on Ubuntu/Debian)

---

## Quick Start

### One-Command Build (Auto Preset)

```bash
# Clone repository
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git -b prune
cd PrismaEngine

# Auto-detect platform + architecture and choose preset
./scripts/build.sh

# Optional: choose target/config explicitly
./scripts/build.sh --target editor --config release

# Optional: force a specific preset
./scripts/build.sh --preset editor-linux-arm64-debug --clean
```

### PacMan Build / Run / Package

```bash
# Build PacMan only (isolated output path)
./scripts/build.sh --target pacman --config debug

# Build + run PacMan
./scripts/run-pacman.sh --config debug

# Build + package (tar.gz)
./scripts/package-pacman.sh --config release
```

PacMan standalone output path:
- `build/pacman-linux-x64-{debug|release}/bin/PacManGame`
- `build/pacman-linux-arm64-{debug|release}/bin/PacManGame`

说明：
- PacMan 已从 Editor 构建目录拆分，不再输出到 editor 的构建路径。
- 在无图形设备环境下，PacMan 会自动切换到 console view（文本交互）。
- ARM 平台编译并行度固定上限 `-j4`（脚本自动限制）。

## Features (Prune Branch)

- **Modern C++20**: Utilizing concepts, coroutines, and designated initializers.
- **Smart Dependency Management**: No manual library installation required; CMake handles everything.
- **Vulkan Rendering**: High-performance GPU rendering on Windows x64.
- **SDL3 Platform**: Unified input and audio handling.
- **ImGui Integration**: Built-in editor tools for scene inspection and debugging.

## License

MIT License - see [LICENSE](LICENSE) for details.
