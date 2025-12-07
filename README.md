# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Status](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml)
[![Code Quality](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/code_quality.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/code_quality.yml)

Prisma Engine is a modern cross-platform game engine designed for lightweight/high-performance game development. The engine features a modular architecture, supports Windows and Android platforms, with plans to extend to more platforms.

English | [简体中文](Docs/README_zh.md)

## 🚀 Core Features

### 🎯 Cross-Platform Support
- **Windows**: Native DirectX 12 support for optimal performance
- **Android**: Complete mobile platform support based on Vulkan graphics API
- **Future Plans**: Linux, macOS and other platform support

### 🎮 Pluggable Backend System
- **Render Backend**: Support for DirectX 12, Vulkan, SDL3 with runtime switching
- **Input Backend**: Support for Win32, SDL3, DirectInput input systems
- **Audio Backend**: Support for XAudio2, SDL3 audio rendering engines
- **Unified Interface**: Abstract backend interface design for easy extension

### 🎮 Rendering System
- **Multi-Backend Rendering**: DirectX 12, Vulkan, SDL3 with runtime switching
- **Modern Graphics Features**: Multi-threaded rendering, Bindless textures, Instancing
- **Advanced Features**: Async compute, Hardware ray tracing, Tile-based rendering
- **Real-time Rendering**: Dynamic lighting, shadows, and post-processing effects

### 🔧 Engine Architecture
- **Component System**: Based on ECS (Entity-Component-System) architecture
- **Modular Design**: Pluggable subsystems for easy extension and maintenance
- **Resource Management**: Smart resource loading, caching and lifecycle management
- **Audio System**: Multi-backend audio rendering with WAV format support

## 📋 System Requirements

### Development Environment
- **IDE**: Visual Studio 2022 or newer
- **SDK**: Windows 10 SDK (10.0.22621.0+)
- **Package Manager**: vcpkg
- **Mobile Development**: Android NDK, Android SDK

### Runtime Environment
- **Windows**: Windows 10+, DirectX 12 compatible graphics card
- **Android**: Android 5.0+, Vulkan compatible device
- **Graphics API**: DirectX 12 or Vulkan 1.1+

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

Or use our setup script:

```bash
./setup_project.bat
```

### 2. Setup Development Environment
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# Install dependencies
vcpkg install
```

### 3. Build Project
1. Open `PrismaEngine.sln` in Visual Studio
2. Select target platform and configuration (Debug/Release)
3. Build solution (Ctrl+Shift+B)
4. Run sample projects

### 4. Run Samples
- **Editor**: Game editor application
- **Runtime**: Game runtime environment
- **EngineTest**: Engine functionality tests

## 📁 Project Structure

```
PrismaEngine/
├── Engine/           # Core engine module
│   ├── include/     # Public header files
│   ├── src/         # Implementation files
│   └── Engine.vcxitems  # Project configuration
├── Editor/          # Game editor
├── Runtime/         # Game runtime
├── EngineTest/      # Engine tests
├── EditorTest/      # Editor tests
├── RuntimeTest/     # Runtime tests
├── Docs/           # Documentation resources
└── Tools/          # Development tools
```

## 🔬 Core Modules

### Backend System
- **RenderBackend**: Abstract render backend interface, supports DirectX 12, Vulkan, SDL3
- **InputBackend**: Abstract input backend interface, supports Win32, SDL3, DirectInput
- **AudioBackend**: Abstract audio backend interface, supports XAudio2, SDL3
- **Backend Management**: Runtime backend switching and hot-plug support

### Rendering System
- **Renderer**: Unified render interface with multi-backend support
- **RenderSystem**: Render pipeline management with multi-threaded rendering
- **MeshRenderer**: Mesh rendering component with instancing support
- **Camera2D**: 2D camera system with viewport management
- **Shader**: Shader management system with HLSL/GLSL support

### Scene Management
- **Scene**: Scene management with entity hierarchy support
- **GameObject**: Game object base class with component container
- **Transform**: Transform component with 2D/3D transformation support
- **Component**: Component base class system with serialization support

### Input System
- **KeyCode**: Keyboard input mapping with cross-platform keycode unification
- **InputManager**: Input event handling with multi-backend support
- **Cross-platform Input**: Unified keyboard, mouse, touch input handling

## 📚 Documentation Resources

- [📖 Engine Architecture](Docs/RenderingSystem.md) - Detailed rendering system explanation
- [🗺️ Development Roadmap](Docs/Roadmap.md) - Project development plan
- [💾 Asset Serialization](Docs/AssetSerialization.md) - Resource management mechanism
- [📝 Development Notes](Docs/MEMO.md) - Technical implementation details

## 📄 License

This project is licensed under the [MIT License](LICENSE) - see the license file for details.

## 📞 Contact

- **Project Homepage**: [GitHub Repository](https://github.com/Excurs1ons/PrismaEngine)
- **Issue Reporting**: [Issues](https://github.com/Excurs1ons/PrismaEngine/issues)

---

*PrismaEngine - High-performance engine for modern game development*