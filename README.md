# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Prisma Engine](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml)


Prisma Engine is a modern cross-platform game engine designed for lightweight/high-performance game development. The engine features a modular architecture, supports Windows and Android platforms, with plans to extend to more platforms.

English | [简体中文](docs/README_zh.md)

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
- **IDE**: Visual Studio 2022 or newer, CLion, or any CMake-compatible IDE
- **SDK**: Windows 10 SDK (10.0.22621.0+)
- **Build System**: CMake 3.24+
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
├── src/              # Source code
│   ├── core/         # Core engine module
│   ├── editor/       # Game editor
│   ├── game/         # Game framework
│   └── runtime/      # Game runtime
├── projects/         # Platform-specific project files
│   ├── android/      # Android projects
│   └── windows/      # Windows projects
├── docs/             # Documentation resources
├── tools/            # Development tools
├── vcpkg/            # Package manager
├── CMakeLists.txt    # Main CMake configuration
└── CMakePresets.json # CMake presets
```

## 📚 Documentation Resources

- [📖 Engine Architecture](docs/RenderingSystem.md) - Detailed rendering system explanation
- [🗺️ Development Roadmap](docs/Roadmap.md) - Project development plan
- [💾 Asset Serialization](docs/AssetSerialization.md) - Resource management mechanism
- [📝 Development Notes](docs/MEMO.md) - Technical implementation details

## 📄 License

This project is licensed under the [MIT License](LICENSE) - see the license file for details.

## 📞 Contact

- **Project Homepage**: [GitHub Repository](https://github.com/Excurs1ons/PrismaEngine)
- **Issue Reporting**: [Issues](https://github.com/Excurs1ons/PrismaEngine/issues)

---

*PrismaEngine - High-performance engine for modern game development*