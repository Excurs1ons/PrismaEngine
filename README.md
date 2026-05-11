# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Status](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions)
[![Android APK](https://img.shields.io/badge/APK-Download-green.svg?logo=android)](https://github.com/Excurs1ons/PrismaEngine/releases/download/latest/PrismaAndroid.apk)
[![CodeWiki](https://img.shields.io/badge/CodeWiki-Google-4285F4?style=flat-square)](https://codewiki.google/github.com/Excurs1ons/PrismaEngine)
[![DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Excurs1ons/PrismaEngine)
[![Zread](https://img.shields.io/badge/Ask_Zread-_.svg?style=flat&color=00b0aa&labelColor=000000&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuOTYxNTYgMS42MDAxSDIuMjQxNTZDMS44ODgxIDEuNjAwMSAxLjYwMTU2IDEuODg2NjQgMS42MDE1NiAyLjI0MDFWNC45NjAxQzEuNjAxNTYgNS4zMTM1NiAxLjg4ODEgNS42MDAxIDIuMjQxNTYgNS42MDAxSDQuOTYxNTZDNS4zMTUwMiA1LjYwMDEgNS42MDE1NiA1LjMxMzU2IDUuNjAxNTYgNC45NjAxVjIuMjQwMUM1LjYwMTU2IDEuODg2NjQgNS4zMTUwMiAxLjYwMDEgNC45NjE1NiAxLjYwMDFaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00Ljk2MTU2IDEwLjM5OTlIMi4yNDE1NkMxLjg4ODEgMTAuMzk5OSAxLjYwMTU2IDEwLjY4NjQgMS42MDE1NiAxMS4wMzk5VjEzLjc1OTlDMS42MDE1NiAxNC4xMTM0IDEuODg4MSAxNC4zOTk5IDIuMjQxNTYgMTQuMzk5OUg0Ljk2MTU2QzUuMzE1MDIgMTQuMzk5OSA1LjYwMTU2IDE0LjExMzQgNS42MDE1NiAxMy43NTk5VjExLjAzOTlDNS42MDE1NiAxMC42ODY0IDUuMzE1MDIgMTAuMzk5OSA0Ljk2MTU2IDEwLjM5OTlaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik0xMy43NTg0IDEuNjAwMUgxMS4wMzg0QzEwLjY4NSAxLjYwMDEgMTAuMzk4NCAxLjg4NjY0IDEwLjM5ODQgMi4yNDAxVjQuOTYwMUMxMC4zOTg0IDUuMzEzNTYgMTAuNjg1IDUuNjAwMSAxMS4wMzg0IDUuNjAwMUgxMy43NTg0QzE0LjExMTkgNS42MDAxIDE0LjM5ODQgNS4zMTM1NiAxNC4zOTg0IDQuOTYwMVYyLjI0MDFDMTQuMzk4NCAxLjg4NjY0IDE0LjExMTkgMS42MDAxIDEzLjc1ODQgMS42MDAxWiIgZmlsbD0iI2ZmZiIvPgo8cGF0aCBkPSJNNCAxMkwxMiA0TDQgMTJaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00IDEyTDEyIDQiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgo8L3N2Zz4K&logoColor=ffffff)](https://zread.ai/Excurs1ons/PrismaEngine)
[![Mintlify](https://img.shields.io/badge/Mintlify-Docs-262626?style=flat-square&logo=googlegemini&logoColor=fff)](https://mintlify.wiki/Excurs1ons/PrismaEngine)

Prisma Engine is a cross-platform 3D game engine built with modern C++20, focusing on high-performance rendering and modern graphics architectures.

[中文文档](./docs/README_zh.md) | [English](./README.md)

> **Current Status**: CoreCLR C# scripting functional. SoA entity pool with 1M virtual capacity. Android Vulkan runtime production-ready.
> **Last Updated**: 2026-05-11

## Architecture Highlights

### Driver-Device Pattern

```
┌────────────────────────────────────────────────────────────┐
│                    Application Layer                       │
│              (Template2D / Game / Editor)                 │
└─────────────────────────────┬──────────────────────────────┘
                              │
┌─────────────────────────────▼──────────────────────────────┐
│                    Device Layer                            │
│         AudioDevice  │  InputDevice  │  RenderSystem       │
└─────────────────────────────┬──────────────────────────────┘
                              │
┌─────────────────────────────▼──────────────────────────────┐
│                    Driver Interface                         │
│        IAudioDriver  │  IInputDriver  │  IRenderDevice     │
└─────────────────────────────┬──────────────────────────────┘
                              │
┌─────────────────────────────▼──────────────────────────────┐
│                 Platform Implementation                     │
│  Windows: XAudio2/RawInput/DX12                           │
│  Android: AAudio/GameActivity/Vulkan                       │
│  Cross:  SDL3 (audio/input/window)                         │
└────────────────────────────────────────────────────────────┘
```

### SoA Entity Pool (Structure of Arrays)

```
┌─────────────────────────────────────────────────────────────┐
│              Virtual Memory Block (1M entities max)         │
├─────────────────┬─────────────────┬─────────────────────────┤
│  Transform SoA  │  Transform SoA  │      Render SoA         │
│     (Buffer A)  │     (Buffer B)  │                        │
│  ─────────────  │  ─────────────  │  ─────────────────────│
│  posX, posY     │  posX, posY     │  active, generation   │
│  rotation       │  rotation       │  colorRGBA            │
│  scaleX, scaleY │  scaleX, scaleY  │  sizeW, sizeH         │
└─────────────────┴─────────────────┴─────────────────────────┘
         ↑ Double-buffer read             ↑ Single read
           (ping-pong swap)
```

- **VirtualAlloc MEM_RESERVE**: Pre-reserve 1M entity virtual address space
- **Double-buffered Transform**: Ping-pong SoA for lock-free read during writes
- **Fixed pointer stability**: C# never gets dangling pointers

### CoreCLR C# Scripting

```
C++ Engine                        CoreCLR Runtime
├── CoreCLRHost                   └── hostfxr (self-contained)
│   └── hostfxr_init              └── PrismaEngine.Core.dll
├── ScriptEngine                  C# GameScripts.dll
│   └── PrismaAPI (18 fns)        ├── Node (entity = position)
├── Engine::Run()                 ├── Script (base class)
│   └── Bootstrap → OnFrame       ├── Input/Time/Math
└── Template2DApp                 └── SceneInit, Behaviors
```

- **Unity-style C#**: `Node` is entity, C++ provides backend
- **No GameObject/Transform split**: `Node.Position/Rotation/Scale` directly
- **Self-contained publish**: No system .NET dependency

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
| SoA Entity Pool | ✅ 100% | Virtual memory 1M capacity, double-buffered |
| CoreCLR Scripting | ✅ 90% | C# Node/Script system, self-contained publish |
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

**Overall: ~80%**

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
- [CoreCLR Scripting Summary](docs/plans/2026-05-10-coreclr-scripting-summary.md) - C# scripting implementation
- [RenderGraph Plan](docs/RenderGraph_Migration_Plan.md) - Future rendering roadmap

## Core Features

- **Modern C++20**: Utilizing concepts, coroutines, and designated initializers.
- **Smart Dependency Management**: No manual library installation required; CMake handles everything.
- **Unified Rendering API**: Write once, run on DX12 or Vulkan.
- **Android Deep Optimization**: Zero-latency input via GameActivity and high-performance Vulkan rendering path.
- **CoreCLR Scripting**: Full C# scripting with SoA entity pool and self-contained deployment.
- **MCP Integration**: Full Model Context Protocol support for AI Agent control (31 tools).

## License

MIT License - see [LICENSE](LICENSE) for details.