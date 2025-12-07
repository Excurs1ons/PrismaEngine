# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Prisma Engine](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml)


Prisma Engine is a personal learning project through which the author is learning the basics of game engine development. As a beginner's work, the project inevitably has shortcomings and errors, but every step is part of the learning process. The engine attempts to implement some basic features and supports Windows and Android platforms.

English | [简体中文](docs/README_zh.md)

## 🎯 Project Goals

### 📚 Learning Objectives
- Learn the basics of game engine development through practice
- Understand fundamental concepts of graphics rendering, resource management, and system architecture
- Explore the basic workflow of cross-platform development

### 🔧 Attempted Features
- **Cross-Platform Support**: Attempting to support Windows and Android platforms
- **Rendering Backend**: Trying to implement DirectX 12 and Vulkan rendering support
- **Input System**: Basic input handling functionality
- **Audio System**: Simple audio playback functionality
- **Resource Management**: Basic resource loading and management mechanisms

### ⚠️ Notes
- This is a learning project, and code quality and architecture design may not be perfect
- Feature implementations may contain bugs and performance issues
- Suggestions and guidance are welcome to help the author improve and learn

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

*PrismaEngine - A beginner's game engine learning project*