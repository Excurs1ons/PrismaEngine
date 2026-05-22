# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI Windows](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml)
[![CI Linux](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml)
[![CI Android](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml)

Prisma Engine is a cross-platform 3D game engine built with modern C++20, focusing on high-performance rendering and modern graphics architectures.

[中文文档](./docs/README_zh.md) | [English](./README.md)

> **Current Status**: CoreCLR C# scripting functional. SoA entity pool with 1M virtual capacity. Android Vulkan runtime production-ready.
> **Last Updated**: 2026-05-11

## Architecture Highlights

### Driver-Device Pattern

```mermaid
flowchart TD
    subgraph App[Application Layer]
        A1[Template2D]
        A2[Game]
        A3[Editor]
    end
    
    subgraph Dev[Device Layer]
        D1[AudioDevice]
        D2[InputDevice]
        D3[RenderSystem]
    end
    
    subgraph Drv[Driver Interface]
        I1[IAudioDriver]
        I2[IInputDriver]
        I3[IRenderDevice]
    end
    
    subgraph Plat[Platform Implementation]
        P1[Windows: XAudio2/RawInput/DX12]
        P2[Android: AAudio/GameActivity/Vulkan]
        P3[Cross: SDL3 audio/input/window]
    end
    
    App --> Dev
    Dev --> Drv
    Drv --> Plat
```

### SoA Entity Pool (Structure of Arrays)

```mermaid
flowchart LR
    subgraph VM["Virtual Memory Block (1M entities max)"]
        TA["Transform SoA<br/>Buffer A<br/>posX, posY<br/>rotation<br/>scaleX, scaleY"]
        TB["Transform SoA<br/>Buffer B<br/>posX, posY<br/>rotation<br/>scaleX, scaleY"]
        TR["Render SoA<br/>active, generation<br/>colorRGBA<br/>sizeW, sizeH"]
    end
    TA <-->|"ping-pong swap"| TB
```

### CoreCLR C# Scripting

```mermaid
flowchart LR
    subgraph Cpp["C++ Engine"]
        H[CoreCLRHost]
        E[ScriptEngine]
        A[PrismaAPI<br/>63 fns + SRP]
        R[Engine::Run]
    end
    
    subgraph DotNet["CoreCLR Runtime"]
        HR[hostfxr<br/>self-contained]
        Core[PrismaEngine.Core.dll]
        Game[GameScripts.dll]
    end
    
    subgraph CS["C# Game"]
        N[Node<br/>entity=position]
        S[Script<br/>OnCreate/OnUpdate]
        W[World<br/>SceneInit]
    end
    
    H --> HR
    E --> A
    R --> H
    HR --> Core
    Core --> Game
    Game --> N
    Game --> S
    Game --> W
```

## CI/CD Status

| Platform | Status | Trigger |
|----------|--------|---------|
| **Windows** | [![CI Windows](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml) | Push / PR |
| **Linux** | [![CI Linux](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml) | Push / PR |
| **Android** | [![CI Android](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml) | Push / PR |

## Current Progress

| Module | Status | Description |
|--------|--------|-------------|
| SoA Entity Pool | ✅ 100% | Virtual memory 1M capacity, double-buffered |
| CoreCLR Scripting | ✅ 95% | C# Node/Script + SRP (CommandBuffer, RendererFeature, 计算管线) |
| Rendering Architecture | ✅ 90% | Core Pass + Feature system + Compute Pipeline RHI |
| Resource Management | ✅ 95% | Handle<T> system + resource pools |
| Vulkan Backend | ✅ 90% | Robust cross-platform Vulkan implementation |
| DirectX 12 Backend | ⏳ 70% | Primary Windows rendering backend |
| Platform Layer | ✅ 95% | Unified Windows/Linux/Android abstraction |
| Logger System | ✅ 100% | Thread-safe cross-platform logging |
| Audio System | ✅ 50% | XAudio2/SDL3 backends with 3D spatial support |
| Shaders | ✅ 50% | PBR lighting shaders (lit/unlit) |
| Android Runtime | ✅ 90% | Optimized Vulkan runtime (integrated GameActivity) |
| Editor Tools | ⏳ 15% | ImGui-based inspector |
| MCP Protocol | ✅ 85% | 17 tools, 7 categories, dual transport, hash delta tracking |
| WebUI Editor | ✅ 80% | Browser-based editor with scene/game viewport, hierarchy, inspector |

**Overall: ~82%**

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

## Using the SDK (Without Full Source)

If you just want to try out a project (like PrismaCraft) or build your own game, you don't need the full engine source — download the **prebuilt SDK** from GitHub Releases:

### Option 1: Download Prebuilt SDK (Recommended)

```bash
# 1. Download from https://github.com/Excurs1ons/PrismaEngine/releases
#    Look for: PrismaEngine-SDK-<version>-<platform>.tar.gz

# 2. Extract
tar xzf PrismaEngine-SDK-0.1.0-linux.tar.gz

# 3. Build any sample project
cd PrismaEngine-SDK-0.1.0-linux/samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=../..
cmake --build build
./build/BasicTriangle
```

### Option 2: Use SDK from Local Repo

```bash
# SDK is already in the repo at sdk/
cd sdk/samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=$(pwd)/../..
cmake --build build
./build/BasicTriangle
```

The SDK contains:
- **190+ public C++ headers** — Full engine API (`#include <PrismaEngine/PrismaEngine.h>`)
- **CMake config** — `find_package(PrismaEngine)` ready
- **Sample projects** — BasicTriangle, BlockGame, PrismaCraftStarter
- **Precompiled libraries** — Engine binaries for your platform (in release downloads)

> **Note**: Prebuilt SDK is available on [GitHub Releases](https://github.com/Excurs1ons/PrismaEngine/releases). The `sdk/` directory in the repo contains headers and CMake configs but no prebuilt binaries — those are too large for the repository.

## Documentation

- [Documentation Index](docs/Index.md) - **Start here**
- [Architecture Overview](docs/README_zh.md) - Architecture and design
- [Vulkan Integration](docs/VulkanIntegration.md) - Detailed Android implementation
- [CoreCLR Scripting Summary](docs/plans/archived/2026-05-10-coreclr-scripting-summary.md) - C# scripting implementation
- [RenderGraph Plan](docs/RenderGraphMigrationPlan.md) - Future rendering roadmap

## Core Features

- **Modern C++20**: Utilizing concepts, coroutines, and designated initializers.
- **Smart Dependency Management**: No manual library installation required; CMake handles everything.
- **Unified Rendering API**: Write once, run on DX12 or Vulkan.
- **Android Deep Optimization**: Zero-latency input via GameActivity and high-performance Vulkan rendering path.
- **CoreCLR Scripting**: Full C# scripting with SoA entity pool and self-contained deployment.
- **MCP Integration**: Full Model Context Protocol support for AI Agent control (17 tools, hash delta tracking, double transport).
- **WebUI Editor**: Browser-based full editor with real-time scene viewport, hierarchy, inspector and console.

## License

MIT License - see [LICENSE](LICENSE) for details.
