# Prisma Engine


[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Excurs1ons/PrismaEngine)

[![Vulkan Backend](https://img.shields.io/badge/Vulkan%20Backend-Implemented-success.svg)](VulkanIntegration.md)
[![RenderGraph](https://img.shields.io/badge/RenderGraph-In%20Progress-orange.svg)](RenderGraph_Migration_Plan.md)

Prisma Engine 是一个使用现代 C++20 构建的跨平台 3D 游戏引擎，专注于学习高级图形编程技术和现代渲染架构。

简体中文 | [English](../README.md)

> **当前状态**: Android Vulkan 运行时已实现，Windows DirectX 12 后端开发中。

## 当前进度 / Current Progress

| 模块 | 状态 | 说明 |
|------|--------|------|
| ECS 组件系统 | ✅ 70% | Entity Component System |
| DirectX 12 后端 | ✅ 65% | Windows 主要渲染后端 |
| Vulkan 后端 | ✅ 80% | 跨平台支持（Windows/Linux/Android）|
| Platform 层 | ✅ 90% | Windows/Linux/Android 抽象层 |
| Logger 系统 | ✅ 85% | 跨平台日志系统 |
| 音频系统 | ✅ 40% | XAudio2/SDL3 后端 |
| 资源管理 | ✅ 60% | AssetManager 实现 |
| Android 运行时 | ✅ 80% | 完整 Vulkan 支持 |
| 物理系统 | ❌ 5% | 规划中 |
| 编辑器工具 | ⏳ 10% | ImGui 基础集成 |

**总体: ~45-50%**

## 快速开始 / Quick Start

### Windows / Windows

```bash
# 克隆仓库
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# 初始化 vcpkg
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install

# 构建项目
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
```

### Linux / Linux

```bash
# 克隆仓库
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# 初始化 vcpkg
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install

# 构建项目
cmake --preset linux-x64-debug
cmake --build --preset linux-x64-debug
```

### Android / Android

```bash
# 使用 Android Studio
cd projects/android/PrismaAndroid
# 在 Android Studio 中打开项目
# 点击 Run 或 ./gradlew assembleDebug
```

详见：[Android 集成文档](VulkanIntegration.md)

## 文档 / Documentation

### 索引 / Index
- [文档索引](Index.md) - Complete documentation navigation

### 核心文档 / Core Docs
- [目录结构](DirectoryStructure.md) - Project organization / 项目组织
- [资源管理](ResourceManager.md) - Asset system / 资产系统
- [渲染系统](RenderingSystem.md) - Rendering architecture / 渲染架构
- [资产序列化](AssetSerialization.md) - Serialization format / 序列化格式

### 架构与设计 / Architecture & Design
- [RenderGraph 迁移计划](RenderGraph_Migration_Plan.md)
- [渲染架构重设计](rendering-architecture-redesign.md)
- [渲染架构对比](rendering-architecture-comparison.md)

### 平台集成 / Platform Integration
- [Android 集成](VulkanIntegration.md) - ✅ Implemented / 已实现
- [Google Swappy 集成](SwappyIntegration.md)
- [音频系统](AudioSystem.md)
- [HAP 视频系统](HAPVideoSystem.md)

### 其他 / Others
- [代码风格指南](CodingStyle.md)
- [开发笔记](MEMO.md)
- [开发路线图](Roadmap.md)
- [引擎需求](Requirements.md)

## 项目结构 / Project Structure

```
PrismaEngine/
├── src/                       # Source code / 源代码
│   ├── engine/                # Core engine / 核心引擎
│   │   ├── audio/            # Audio system / 音频系统
│   │   ├── core/             # ECS & Asset / ECS 和资产
│   │   ├── graphic/          # Rendering / 渲染
│   │   │   ├── adapters/     # DX12, Vulkan
│   │   │   ├── pipelines/    # Forward, Deferred
│   │   │   └── interfaces/   # Rendering interfaces
│   │   ├── input/           # Input system / 输入系统
│   │   ├── math/            # Math library / 数学库
│   │   ├── platform/        # Platform abstraction / 平台抽象
│   │   ├── resource/        # Resource management / 资源管理
│   │   └── scripting/       # Scripting / 脚本
│   ├── editor/              # Editor application / 编辑器应用
│   ├── game/                # Game framework / 游戏框架
│   └── runtime/             # Runtime environments / 运行时环境
│       ├── windows/         # Windows runtime
│       ├── linux/           # Linux runtime
│       └── android/         # Android runtime (Vulkan)
│
├── resources/               # Engine resources / 引擎资源
│   ├── common/              # Shared resources / 共享资源
│   │   ├── shaders/
│   │   │   ├── hlsl/        # DirectX 12 shaders
│   │   │   └── glsl/        # Vulkan/OpenGL shaders
│   │   ├── textures/
│   │   └── fonts/
│   └── runtime/             # Platform-specific / 平台特定
│       ├── windows/
│       ├── linux/
│       └── android/
│
├── projects/                # Platform projects / 平台项目
│   └── android/             # Android Studio project
│
├── cmake/                   # CMake modules / CMake 模块
├── docs/                    # Documentation / 文档
├── assets/                  # Example assets / 示例资产
└── vcpkg.json               # Dependencies / 依赖配置
```

详见：[完整目录结构](DirectoryStructure.md)

## 特性 / Features

### 跨平台支持 / Cross-Platform
- ✅ Windows (DirectX 12, Vulkan)
- ✅ Linux (Vulkan)
- ✅ Android (Vulkan)

### 渲染后端 / Rendering Backends
- **DirectX 12**: Windows 主要后端
- **Vulkan**: 跨平台支持
  - Windows
  - Linux
  - Android (完整实现，~1456 行代码)

### 核心系统 / Core Systems
- **ECS (Entity Component System)**: 组件化架构
- **Asset Management**: 统一资源管理
- **Audio System**: XAudio2/SDL3
- **Platform Abstraction**: 统一平台接口

## 命名空间 / Namespace

所有引擎代码使用 `PrismaEngine` 命名空间：

```cpp
namespace PrismaEngine {
    namespace Graphic {
        // Rendering code / 渲染代码
    }
    namespace Audio {
        // Audio code / 音频代码
    }
}
```

## 许可证 / License

MIT License - 详见 [LICENSE](../LICENSE)

## 致谢 / Acknowledgments

- [DirectX 12](https://github.com/microsoft/DirectX-Graphics-Samples) - Graphics samples
- [Vulkan](https://github.com/KhronosGroup/Vulkan-Guide) - Vulkan guide
- [SDL3](https://github.com/libsdl-org/SDL) - Platform abstraction
- [Dear ImGui](https://github.com/ocornut/imgui) - UI framework
- [GLM](https://github.com/g-truc/glm) - Mathematics library
