# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64--lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Status](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions)

## Prune Branch

此分支是 Prisma Engine 的简化版本，专注于 Windows x64 平台。

**简化内容：**
- ✅ 仅支持 **Windows x64** 平台
- ✅ 渲染后端：仅启用 **Vulkan**
- ✅ 音频后端：仅启用 **SDL3**
- ✅ 编辑器：集成 **ImGui**
- ❌ 移除 DirectX 12 后端
- ❌ 移除 OpenGL 后端
- ❌ 移除 XAudio2 后端
- ❌ 移除 Linux/Android/WebAssembly 支持

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

## Quick Start

### Windows x64

```bash
# Clone repository
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git -b prune
cd PrismaEngine

# Build using CMake Presets
cmake --preset windows-x64-debug
cmake --build build/windows-x64-debug --parallel

# Run Editor
./build/windows-x64-debug/editor/PrismaEditor.exe
```

## Features (Prune Branch)

- **Modern C++20**: Utilizing concepts, coroutines, and designated initializers.
- **Smart Dependency Management**: No manual library installation required; CMake handles everything.
- **Vulkan Rendering**: High-performance GPU rendering on Windows x64.
- **SDL3 Platform**: Unified input and audio handling.
- **ImGui Integration**: Built-in editor tools for scene inspection and debugging.

## License

MIT License - see [LICENSE](LICENSE) for details.
