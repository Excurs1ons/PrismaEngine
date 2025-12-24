# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![PrismaAndroid](https://img.shields.io/badge/PrismaAndroid-Vulkan%20Runtime-success.svg)](https://github.com/Excurs1ons/PrismaAndroid)
[![Vulkan Migration](https://img.shields.io/badge/Vulkan%20Backend-In%20Progress-blue.svg)](docs/Roadmap.md)
[![RenderGraph](https://img.shields.io/badge/RenderGraph-Planning-orange.svg)](docs/RenderGraph_Migration_Plan.md)


Prisma Engine is a modern 3D game engine built from scratch, focusing on learning advanced graphics programming techniques and modern rendering architectures. The project implements cutting-edge rendering systems including ScriptableRenderPipeline and is currently migrating to a RenderGraph-based architecture for optimal performance and flexibility.

English | [简体中文](docs/README_zh.md)

> **Note**: [PrismaAndroid](https://github.com/Excurs1ons/PrismaAndroid) contains a fully functional Vulkan runtime (~1300 lines) that is being progressively migrated into the engine's rendering abstraction layer.

## 🎯 Project Goals

## ✨ Features

### 🎮 Core Systems
- **Modern Rendering Architecture**: ScriptableRenderPipeline with flexible Pass system
- **Multi-Backend Support**: DirectX 12 (Windows) and Vulkan (Cross-platform)
- **Cross-Platform**: Windows and Android with shared codebase
- **Input Management**: Unified input system with SDL3 backend
- **Audio System**: XAudio2 (Windows) and SDL3 Audio (Cross-platform)
- **Resource Management**: JSON-based asset serialization system

### 🚀 Advanced Rendering
- **Forward Rendering Pipeline**: Physically-based rendering with PBR materials
- **Skybox Rendering**: Cubemap-based environment rendering
- **GUI System**: Dear ImGui integration for editor UI
- **Scriptable Passes**: Flexible render pass architecture for custom effects

### 🔄 Upcoming (RenderGraph Migration)
- **RenderGraph System**: Modern DAG-based rendering architecture
  - Automatic resource dependency management
  - Optimized memory allocation with resource aliasing
  - Parallel pass execution support
  - Cross-API abstraction layer
- **GPU-Driven Rendering**: Indirect drawing and GPU culling
- **Advanced Post-Processing**: Tone mapping, FXAA, and custom effects

### ⚠️ Development Status
- This is a learning project focused on modern graphics programming
- **Vulkan Backend Migration**: PrismaAndroid's production-ready Vulkan renderer is being integrated into the engine
- Actively migrating to RenderGraph architecture (see [migration plan](docs/RenderGraph_Migration_Plan.md))
- Performance optimizations and bug fixes ongoing
- Community contributions and feedback are welcome!

### 📊 Current Progress (30-35%)
- ✅ ECS Component System, Scene Management, Camera2D/3D
- ✅ DirectX 12 Backend (65% complete)
- ✅ **Vulkan Backend (PrismaAndroid) - 80% complete**
- 🔄 ScriptableRenderPipeline framework
- ⏳ RenderGraph architecture (planning phase)
- ❌ Physics, Animation, Editor UI (not started)

## 📋 System Requirements

### Development Environment
- **IDE**: Visual Studio 2022 (Windows), CLion (Cross-platform), or any CMake-compatible IDE
- **SDK**: Windows 10 SDK (10.0.26100.0+)
- **Build System**: CMake 3.31+
- **Package Manager**: vcpkg
- **Mobile Development**: Android NDK r25+, Android SDK API 30+

### Runtime Environment
- **Windows**: Windows 10 (1903) or later, DirectX 12 compatible graphics card
- **Android**: Android 7.0+ (API Level 24), Vulkan 1.1+ compatible device
- **Graphics API**: DirectX 12 (Windows), Vulkan 1.1+ (Cross-platform)
- **Memory**: 4GB RAM minimum, 8GB recommended

## 🛠️ Quick Start

### 1. Get Source Code

To clone the repository along with its submodules, use the `--recursive` flag:

```bash
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine
```

If you have already cloned the repository without the `--recursive` flag, you can initialize and update the submodules separately:

```bash
git submodule init
git submodule update
```

### 2. Setup Development Environment
```bash
# Bootstrap vcpkg
./vcpkg/bootstrap-vcpkg.bat

# Install dependencies
./vcpkg/vcpkg install
```

### 3. Build Project
Using CMake with presets:
```bash
# Configure the project
cmake --preset=windows-x64-debug

# Build the project
cmake --build --preset=windows-x64-debug
```

Alternative Visual Studio approach:
1. Open folder in Visual Studio (File -> Open -> Folder)
2. Select the PrismaEngine root folder
3. Visual Studio will automatically detect CMake configuration
4. Build solution (Ctrl+Shift+B)

### 4. Run Samples
- **Editor**: Game editor application
- **Runtime**: Game runtime environment

## 📁 Project Structure

```
PrismaEngine/
├── src/                          # Source code
│   ├── engine/                    # Core engine modules
│   │   ├── audio/                 # Audio system
│   │   ├── graphic/               # Rendering system
│   │   │   ├── pipelines/         # Render pipelines
│   │   │   │   └── forward/       # Forward rendering
│   │   │   ├── RenderGraphCore.h  # RenderGraph architecture
│   │   │   ├── RenderBackend.h    # Rendering backend interface
│   │   │   └── ...                # Other rendering components
│   │   ├── platform/              # Platform abstraction
│   │   ├── resource/              # Resource management
│   │   └── ...                    # Other core systems
│   ├── editor/                    # Game editor
│   ├── game/                      # Game framework
│   └── runtime/                   # Game runtime
├── projects/                     # Platform-specific projects
│   ├── android/                   # Android projects
│   │   └── Game/                  # Android game app
│   └── windows/                   # Windows projects
│       └── Game/                  # Windows game app
├── docs/                         # Documentation
│   ├── RenderGraph_Migration_Plan.md  # RenderGraph migration guide
│   ├── RenderingSystem.md             # Rendering system docs
│   ├── Roadmap.md                     # Project roadmap
│   └── ...                            # Other documentation
├── assets/                        # Game assets
│   └── shaders/                    # HLSL shader files
├── tools/                         # Development tools
│   └── Scripts/                    # Build and utility scripts
├── vcpkg/                         # Package manager submodule
├── CMakeLists.txt                 # Main CMake configuration
├── CMakePresets.json              # CMake presets
└── .gitmodules                    # Git submodule configuration

Related Repositories:
├── PrismaAndroid/                 # Vulkan Android Runtime (being migrated)
│   ├── app/src/main/cpp/
│   │   ├── VulkanContext.{h,cpp}  # Vulkan context management
│   │   ├── RendererVulkan.{h,cpp} # Full Vulkan renderer (~1300 lines)
│   │   ├── ShaderVulkan.{h,cpp}   # SPIR-V shader loading
│   │   └── TextureAsset.{h,cpp}   # Texture asset management
│   └── ...
```

## 📚 Documentation

### 📖 Architecture & Design
- [🔄 RenderGraph Migration Plan](docs/RenderGraph_Migration_Plan.md) - Detailed migration strategy
- [🎮 Rendering System](docs/RenderingSystem.md) - Current rendering architecture
- [💾 Asset Serialization](docs/AssetSerialization.md) - Resource management system
- [📝 Development Notes](docs/MEMO.md) - Technical implementation details

### 🗺️ Project Planning
- [📍 Development Roadmap](docs/Roadmap.md) - Project development timeline
- [📋 Requirements](docs/Requirements.md) - Engine requirements and specifications

## 📄 License

This project is licensed under the [MIT License](LICENSE) - see the license file for details.

## 🙏 Acknowledgments

- [DirectX 12](https://github.com/microsoft/DirectX-Graphics-Samples) - Inspiration for rendering backend
- [Vulkan](https://github.com/KhronosGroup/Vulkan-Guide) - Cross-platform graphics API
- [SDL3](https://github.com/libsdl-org/SDL) - Cross-platform platform layer
- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI library

## 📞 Contact & Support

- **Project Homepage**: [GitHub Repository](https://github.com/Excurs1ons/PrismaEngine)
- **Issue Reporting**: [GitHub Issues](https://github.com/Excurs1ons/PrismaEngine/issues)

---

*PrismaEngine - A personal learning project for exploring modern graphics programming*