# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Excurs1ons/PrismaEngine)
[![zread](https://img.shields.io/badge/Ask_Zread-_.svg?style=flat&color=00b0aa&labelColor=000000&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuOTYxNTYgMS42MDAxSDIuMjQxNTZDMS44ODgxIDEuNjAwMSAxLjYwMTU2IDEuODg2NjQgMS42MDE1NiAyLjI0MDFWNC45NjAxQzEuNjAxNTYgNS4zMTM1NiAxLjg4ODEgNS42MDAxIDIuMjQxNTYgNS42MDAxSDQuOTYxNTZDNS4zMTUwMiA1LjYwMDEgNS42MDE1NiA1LjMxMzU2IDUuNjAxNTYgNC45NjAxVjIuMjQwMUM1LjYwMTU2IDEuODg2NjQgNS4zMTUwMiAxLjYwMDEgNC45NjE1NiAxLjYwMDFaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00Ljk2MTU2IDEwLjM5OTlIMi4yNDE1NkMxLjg4ODEgMTAuMzk5OSAxLjYwMTU2IDEwLjY4NjQgMS42MDE1NiAxMS4wMzk5VjEzLjc1OTlDMS42MDE1NiAxNC4xMTM0IDEuODg4MSAxNC4zOTk5IDIuMjQxNTYgMTQuMzk5OUg0Ljk2MTU2QzUuMzE1MDIgMTQuMzk5OSA1LjYwMTU2IDE0LjExMzQgNS42MDE1NiAxMy43NTk5VjExLjAzOTlDNS42MDE1NiAxMC42ODY0IDUuMzE1MDIgMTAuMzk5OSA0Ljk2MTU2IDEwLjM5OTlaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik0xMy43NTg0IDEuNjAwMUgxMS4wMzg0QzEwLjY4NSAxLjYwMDEgMTAuMzk4NCAxLjg4NjY0IDEwLjM5ODQgMi4yNDAxVjQuOTYwMUMxMC4zOTg0IDUuMzEzNTYgMTAuNjg1IDUuNjAwMSAxMS4wMzg0IDUuNjAwMUgxMy43NTg0QzE0LjExMTkgNS42MDAxIDE0LjM5ODQgNS4zMTM1NiAxNC4zOTg0IDQuOTYwMVYyLjI0MDFDMTQuMzk4NCAxLjg4NjY0IDE0LjExMTkgMS42MDAxIDEzLjc1ODQgMS42MDAxWiIgZmlsbD0iI2ZmZiIvPgo8cGF0aCBkPSJNNCAxMkwxMiA0TDQgMTJaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00IDEyTDEyIDQiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgo8L3N2Zz4K&logoColor=ffffff)](https://zread.ai/Excurs1ons/PrismaEngine)

[![Vulkan Backend](https://img.shields.io/badge/Vulkan%20Backend-Implemented-success.svg)](docs/VulkanIntegration.md)
[![RenderGraph](https://img.shields.io/badge/RenderGraph-In%20Progress-orange.svg)](docs/RenderGraph_Migration_Plan.md)

Prisma Engine is a cross-platform 3D game engine built with modern C++20, focusing on learning advanced graphics programming techniques and modern rendering architectures.

English | [简体中文](docs/README_zh.md)

> **Current Status**: Android Vulkan runtime implemented, Windows DirectX 12 backend in development.

## Current Progress

| Module | Status | Description |
|--------|--------|-------------|
| ECS Component System | ✅ 75% | Entity Component System |
| DirectX 12 Backend | ✅ 65% | Primary Windows rendering backend |
| Vulkan Backend | ✅ 85% | Cross-platform (Windows/Linux/Android) |
| Platform Layer | ✅ 95% | Windows/Linux/Android abstraction |
| Logger System | ✅ 95% | Cross-platform logging |
| Audio System | ✅ 40% | XAudio2/SDL3 backends |
| Resource Management | ✅ 60% | AssetManager implementation |
| Android Runtime | ✅ 85% | Full Vulkan support with game-activity |
| Physics System | ❌ 5% | Planned |
| Editor Tools | ⏳ 10% | ImGui basic integration |

**Overall: ~50-55%**

## Quick Start

### Windows

```bash
# Clone repository
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# Initialize vcpkg
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install

# Build
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
```

### Linux

```bash
# Clone repository
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# Initialize vcpkg
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install

# Build
cmake --preset linux-x64-debug
cmake --build --preset linux-x64-debug
```

### Android

```bash
# Using Android Studio
cd projects/android/PrismaAndroid
# Open project in Android Studio
# Click Run or ./gradlew assembleDebug
```

See: [Android Integration](docs/VulkanIntegration.md)

## Documentation

### Index
- [Documentation Index](docs/Index.md) - Complete documentation navigation

### Core Docs
- [Directory Structure](docs/DirectoryStructure.md) - Project organization
- [Resource Management](docs/ResourceManager.md) - Asset system
- [Rendering System](docs/RenderingSystem.md) - Rendering architecture
- [Asset Serialization](docs/AssetSerialization.md) - Serialization format

### Architecture & Design
- [RenderGraph Migration Plan](docs/RenderGraph_Migration_Plan.md)
- [Rendering Architecture Redesign](docs/rendering-architecture-redesign.md)
- [Rendering Architecture Comparison](docs/rendering-architecture-comparison.md)

### Platform Integration
- [Android Integration](docs/VulkanIntegration.md) - ✅ Implemented
- [Google Swappy Integration](docs/SwappyIntegration.md)
- [Audio System](docs/AudioSystem.md)
- [HAP Video System](docs/HAPVideoSystem.md)

### Others
- [Coding Style Guide](docs/CodingStyle.md)
- [Development Notes](docs/MEMO.md)
- [Development Roadmap](docs/Roadmap.md)
- [Requirements](docs/Requirements.md)

## Project Structure

```
PrismaEngine/
├── src/                       # Source code
│   ├── engine/                # Core engine
│   │   ├── audio/            # Audio system
│   │   ├── core/             # ECS & Asset
│   │   ├── graphic/          # Rendering
│   │   │   ├── adapters/     # DX12, Vulkan
│   │   │   ├── pipelines/    # Forward, Deferred
│   │   │   └── interfaces/   # Rendering interfaces
│   │   ├── input/           # Input system
│   │   ├── math/            # Math library
│   │   ├── platform/        # Platform abstraction
│   │   ├── resource/        # Resource management
│   │   └── scripting/       # Scripting
│   ├── editor/              # Editor application
│   ├── game/                # Game framework
│   └── runtime/             # Runtime environments
│       ├── windows/         # Windows runtime
│       ├── linux/           # Linux runtime
│       └── android/         # Android runtime (Vulkan)
│
├── resources/               # Engine resources
│   ├── common/              # Shared resources
│   │   ├── shaders/
│   │   │   ├── hlsl/        # DirectX 12 shaders
│   │   │   └── glsl/        # Vulkan/OpenGL shaders
│   │   ├── textures/
│   │   └── fonts/
│   └── runtime/             # Platform-specific
│       ├── windows/
│       ├── linux/
│       └── android/
│
├── projects/                # Platform projects
│   └── android/             # Android Studio project
│
├── cmake/                   # CMake modules
├── docs/                    # Documentation
├── assets/                  # Example assets
└── vcpkg.json               # Dependencies
```

See: [Full Directory Structure](docs/DirectoryStructure.md)

## Features

### Cross-Platform Support
- ✅ Windows (DirectX 12, Vulkan)
- ✅ Linux (Vulkan)
- ✅ Android (Vulkan)

### Rendering Backends
- **DirectX 12**: Primary Windows backend
- **Vulkan**: Cross-platform support
  - Windows
  - Linux
  - Android (complete implementation, ~1456 lines)

### Core Systems
- **ECS (Entity Component System)**: Component-based architecture
- **Asset Management**: Unified resource management
- **Audio System**: XAudio2/SDL3
- **Platform Abstraction**: Unified platform interface

## Namespace

All engine code uses the `PrismaEngine` namespace:

```cpp
namespace PrismaEngine {
    namespace Graphic {
        // Rendering code
    }
    namespace Audio {
        // Audio code
    }
}
```

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- [DirectX 12](https://github.com/microsoft/DirectX-Graphics-Samples) - Graphics samples
- [Vulkan](https://github.com/KhronosGroup/Vulkan-Guide) - Vulkan guide
- [SDL3](https://github.com/libsdl-org/SDL) - Platform abstraction
- [Dear ImGui](https://github.com/ocornut/imgui) - UI framework
- [GLM](https://github.com/g-truc/glm) - Mathematics library
