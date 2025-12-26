# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Excurs1ons/PrismaEngine)
[![PrismaAndroid](https://img.shields.io/badge/PrismaAndroid-Vulkan%20Runtime-success.svg)](https://github.com/Excurs1ons/PrismaAndroid)

[![Vulkan Migration](https://img.shields.io/badge/Vulkan%20Backend-In%20Progress-blue.svg)](docs/Roadmap.md)
[![RenderGraph](https://img.shields.io/badge/RenderGraph-Planning-orange.svg)](docs/RenderGraph_Migration_Plan.md)

Prisma Engine is a modern 3D game engine built from scratch, focusing on learning advanced graphics programming techniques and modern rendering architectures.

English | [简体中文](docs/README_zh.md)

> **Note**: [PrismaAndroid](https://github.com/Excurs1ons/PrismaAndroid) contains a fully functional Vulkan runtime (~1300 lines) that is being progressively migrated into the engine's rendering abstraction layer.

## Current Progress

| Module | Status |
|--------|--------|
| ECS Component System | ✅ 70% |
| DirectX 12 Backend | ✅ 65% |
| **Vulkan Backend (PrismaAndroid)** | ✅ **80%** |
| ScriptableRenderPipeline | 🔄 40% |
| Audio System | ⏳ 15% |
| Physics System | ❌ 5% |
| Editor Tools | ⏳ 10% |

**Overall: ~30-35%**

## Quick Start

```bash
# Clone with submodules
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# Bootstrap vcpkg
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install

# Build
cmake --preset=windows-x64-debug
cmake --build --preset=windows-x64-debug
```

## Documentation

### Index
- [Documentation Index](docs/Index.md) - Complete documentation navigation

### Architecture & Design
- [RenderGraph Migration Plan](docs/RenderGraph_Migration_Plan.md)
- [Rendering System](docs/RenderingSystem.md)
- [Asset Serialization](docs/AssetSerialization.md)
- [Embedded Resources](docs/EmbeddedResources.md)

### Planning
- [Development Roadmap](docs/Roadmap.md)
- [Requirements](docs/Requirements.md)

### Platform Integration
- [Vulkan Backend Integration](docs/VulkanIntegration.md)
- [Google Swappy Integration](docs/SwappyIntegration.md)
- [Audio System Design](docs/AudioSystem.md)
- [HAP Video System](docs/HAPVideoSystem.md)

### Development
- [Coding Style Guide](docs/CodingStyle.md)
- [Development Notes](docs/MEMO.md)

## Project Structure

```
PrismaEngine/
├── src/                   # Source code
│   ├── engine/           # Core engine modules
│   ├── editor/           # Game editor
│   ├── game/             # Game framework
│   └── runtime/          # Game runtime
├── projects/             # Platform-specific projects
├── docs/                 # Documentation
├── assets/               # Game assets
└── tools/                # Development tools
```

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- [DirectX 12](https://github.com/microsoft/DirectX-Graphics-Samples)
- [Vulkan](https://github.com/KhronosGroup/Vulkan-Guide)
- [SDL3](https://github.com/libsdl-org/SDL)
- [Dear ImGui](https://github.com/ocornut/imgui)
