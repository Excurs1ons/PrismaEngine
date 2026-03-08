# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Status](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions)
[![Android APK](https://img.shields.io/badge/APK-Download-green.svg?logo=android)](https://github.com/Excurs1ons/PrismaEngine/releases/download/latest/PrismaAndroid.apk)

Prisma Engine is a cross-platform 3D game engine built with modern C++20, focusing on high-performance rendering and modern graphics architectures.

English | [简体中文](docs/README_zh.md)

> **Current Status**: Android Vulkan runtime is production-ready, Windows DirectX 12 backend is in advanced development.

## CI/CD Status

### CI (Continuous Integration)

| Platform | Status | Trigger |
|----------|--------|---------|
| Windows | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-windows.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml) | Push / PR |
| Android | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-android.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml) | Push / PR |
| Linux | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-linux.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml) | Push / PR |

### Engine Build

| Platform | Status | Trigger |
|----------|--------|---------|
| Windows | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-engine.yml) | Push / Manual |
| Android | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-android-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-android-engine.yml) | Push / Manual |
| Linux | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-engine.yml) | Push / Manual |

### Editor Build

| Platform | Status | Trigger |
|----------|--------|---------|
| Windows | [![Editor](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-editor.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-editor.yml) | Push / Manual |
| Linux | [![Editor](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-editor.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-editor.yml) | Push / Manual |

### Runtime Build

| Platform | Status | Trigger |
|----------|--------|---------|
| Windows | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-runtime.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-runtime.yml) | Push / Manual |
| Android | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-android-runtime.yml?branch=main&label=APK)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-android-runtime.yml) | Push / Manual |
| Linux | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-runtime.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-runtime.yml) | Push / Manual |

### Package & Release

| Target | Status | Trigger |
|--------|--------|---------|
| Package Linux | [![Package Linux](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/package-linux.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/package-linux.yml) | Manual |
| Package Windows | [![Package Windows](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/package-windows.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/package-windows.yml) | Manual |
| Release | [![Release](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/release.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/release.yml) | Tag (`v*.*.*`) |

## Current Progress

| Module | Status | Description |
|--------|--------|-------------|
| ECS Component System | ✅ 80% | High-performance Entity Component System |
| DirectX 12 Backend | ✅ 70% | Modern DX12 rendering path |
| Vulkan Backend | ✅ 90% | Robust cross-platform implementation |
| Platform Layer | ✅ 95% | Unified Windows/Linux/Android abstraction |
| Logger System | ✅ 100% | Thread-safe cross-platform logging |
| Audio System | ✅ 50% | XAudio2/SDL3 backends with 3D spatial support |
| Resource Management | ✅ 75% | Smart asset loading and caching |
| Android Runtime | ✅ 90% | Optimized Vulkan runtime with GameActivity |
| Editor Tools | ⏳ 15% | ImGui integrated basic inspector |

**Overall: ~65%**

## Quick Start

Prisma Engine now uses **CMake FetchContent** by default, making dependency management completely automatic.

### Windows

```bash
# Clone repository
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# Build using CMake Presets
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
# Location: projects/android/PrismaAndroid
# Dependencies are automatically downloaded via CMake FetchContent
```

## Documentation

- [Documentation Index](docs/Index.md) - **Start Here**
- [Architecture Optimization](docs/ArchitectureOptimization.md) - Recent design improvements
- [Vulkan Integration](docs/VulkanIntegration.md) - Detailed Android implementation
- [RenderGraph Plan](docs/RenderGraph_Migration_Plan.md) - Future rendering roadmap

## Features

- **Modern C++20**: Utilizing concepts, coroutines, and designated initializers.
- **Smart Dependency Management**: No manual library installation required; CMake handles everything.
- **Unified Rendering API**: Write once, run on DX12 or Vulkan.
- **Android Optimized**: Zero-latency input via GameActivity and high-performance Vulkan path.

## License

MIT License - see [LICENSE](LICENSE) for details.
