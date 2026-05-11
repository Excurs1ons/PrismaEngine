# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Status](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions)
[![Android APK](https://img.shields.io/badge/APK-Download-green.svg?logo=android)](https://github.com/Excurs1ons/PrismaEngine/releases/download/latest/PrismaAndroid.apk)

Prisma Engine is a cross-platform 3D game engine built with modern C++20, focusing on high-performance rendering and modern graphics architectures.

[中文文档](./docs/README_zh.md) | [English](./README.md)

> **Current Status**: Android Vulkan runtime is production-ready. Windows DirectX 12 backend is in active development.
> **Last Updated**: 2026-05-11

## CI/CD Status

| Target | Platform | Status | Trigger |
|--------|----------|--------|---------|
| **CI** | Windows | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-windows.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml) | Push / PR |
| **CI** | Android | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-android.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml) | Push / PR |
| **CI** | Linux | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-linux.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml) | Push / PR |
| **APK** | Android | [![Android Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-android-runtime.yml?branch=main&label=APK)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-android-runtime.yml) | Push / Manual |
| **Engine** | Windows | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-engine.yml) | Push / Manual |
| **Engine** | Android | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-android-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-android-engine.yml) | Push / Manual |
| **Engine** | Linux | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-engine.yml) | Push / Manual |
| **Editor** | Windows | [![Editor](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-editor.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-editor.yml) | Push / Manual |
| **Editor** | Linux | [![Editor](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-editor.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-editor.yml) | Push / Manual |
| **Runtime** | Windows | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-runtime.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-runtime.yml) | Push / Manual |
| **Runtime** | Android | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-android-runtime.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-android-runtime.yml) | Push / Manual |
| **Runtime** | Linux | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-runtime.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-runtime.yml) | Push / Manual |
| **Release** | All | [![Release](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/release.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/release.yml) | Tag (`v*.*.*`) |

## Current Progress

| Module | Status | Description |
|--------|--------|-------------|
| Rendering Architecture | ✅ 85% | Core Pass + Feature system |
| Resource Management | ✅ 95% | Handle<T> system + resource pools |
| Vulkan Backend | ✅ 90% | Robust cross-platform Vulkan implementation |
| DirectX 12 Backend | ⏳ 70% | Primary Windows rendering backend |
| Platform Layer | ✅ 95% | Unified Windows/Linux/Android abstraction |
| Logger System | ✅ 100% | Thread-safe cross-platform logging |
| Audio System | ✅ 50% | XAudio2/SDL3 backends with 3D spatial support |
| Shaders | ✅ 50% | PBR lighting shaders (lit/unlit) |
| Android Runtime | ✅ 90% | Optimized Vulkan runtime (integrated GameActivity) |
| Editor Tools | ⏳ 15% | ImGui-based inspector |
| MCP Protocol | ✅ 80% | 31 tools for AI Agent integration |

**Overall: ~75%**

## Quick Start

Prisma Engine uses **CMake FetchContent** for dependency management by default - no manual library installation required.

### Windows

```bash
# Clone repository
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# Build with CMake Presets
cmake --preset windows-x64-debug
cmake --build build/windows-x64-debug --parallel
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libx11-dev libxrandr-dev libvulkan-dev

# Build
cmake --preset linux-x64-debug
cmake --build build/linux-x64-debug --parallel
```

### Android

```bash
# Open in Android Studio
# Path: projects/android/PrismaAndroid
# Dependencies download automatically via CMake FetchContent
```

## Documentation

- [Documentation Index](docs/Index.md) - **Start here**
- [Architecture Overview](docs/README_zh.md) - Architecture and design
- [Vulkan Integration](docs/VulkanIntegration.md) - Detailed Android implementation
- [RenderGraph Plan](docs/RenderGraph_Migration_Plan.md) - Future rendering roadmap

## Core Features

- **Modern C++20**: Utilizing concepts, coroutines, and designated initializers.
- **Smart Dependency Management**: No manual library installation required; CMake handles everything.
- **Unified Rendering API**: Write once, run on DX12 or Vulkan.
- **Android Deep Optimization**: Zero-latency input via GameActivity and high-performance Vulkan rendering path.
- **MCP Integration**: Full Model Context Protocol support for AI Agent control (31 tools).

## License

MIT License - see [LICENSE](LICENSE) for details.
