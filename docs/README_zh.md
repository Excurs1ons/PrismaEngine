# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![PrismaAndroid](https://img.shields.io/badge/PrismaAndroid-Vulkan%20Runtime-success.svg)](https://github.com/Excurs1ons/PrismaAndroid)
[![Vulkan Migration](https://img.shields.io/badge/Vulkan%20Backend-In%20Progress-blue.svg)](Roadmap.md)
[![RenderGraph](https://img.shields.io/badge/RenderGraph-Planning-orange.svg)](RenderGraph_Migration_Plan.md)

Prisma Engine 是一个从零开始构建的现代 3D 游戏引擎，专注于学习高级图形编程技术和现代渲染架构。

简体中文 | [English](../README.md)

> **注意**: [PrismaAndroid](https://github.com/Excurs1ons/PrismaAndroid) 包含功能完整的 Vulkan 运行时实现（~1300 行），正在逐步迁移到引擎的渲染抽象层。

## 当前进度

| 模块 | 状态 |
|------|--------|
| ECS 组件系统 | ✅ 70% |
| DirectX 12 后端 | ✅ 65% |
| **Vulkan 后端 (PrismaAndroid)** | ✅ **80%** |
| ScriptableRenderPipeline | 🔄 40% |
| 音频系统 | ⏳ 15% |
| 物理系统 | ❌ 5% |
| 编辑器工具 | ⏳ 10% |

**总体: ~30-35%**

## 快速开始

```bash
# 克隆仓库及子模块
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# 初始化 vcpkg
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install

# 构建项目
cmake --preset=windows-x64-debug
cmake --build --preset=windows-x64-debug
```

## 文档

### 索引
- [文档索引](Index.md) - 完整文档导航

### 架构与设计
- [RenderGraph 迁移计划](RenderGraph_Migration_Plan.md)
- [渲染系统](RenderingSystem.md)
- [资源序列化](AssetSerialization.md)
- [内嵌资源系统](EmbeddedResources.md)

### 规划
- [开发路线图](Roadmap.md)
- [引擎需求](Requirements.md)

### 平台集成
- [Android 平台集成计划](VulkanIntegration.md)
- [Google Swappy 集成](SwappyIntegration.md)
- [音频系统设计](AudioSystem.md)
- [HAP 视频系统](HAPVideoSystem.md)

### 开发
- [代码风格指南](CodingStyle.md)
- [开发笔记](MEMO.md)

## 项目结构

```
PrismaEngine/
├── src/                   # 源代码
│   ├── engine/           # 核心引擎模块
│   ├── editor/           # 游戏编辑器
│   ├── game/             # 游戏框架
│   └── runtime/          # 游戏运行时
├── projects/             # 平台特定项目
├── docs/                 # 文档
├── assets/               # 游戏资源
└── tools/                # 开发工具
```

## 许可证

MIT License - 详见 [LICENSE](../LICENSE)

## 致谢

- [DirectX 12](https://github.com/microsoft/DirectX-Graphics-Samples)
- [Vulkan](https://github.com/KhronosGroup/Vulkan-Guide)
- [SDL3](https://github.com/libsdl-org/SDL)
- [Dear ImGui](https://github.com/ocornut/imgui)
