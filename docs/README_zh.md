# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Prisma Engine](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml)

Prisma Engine 是一个个人学习项目，作者通过它来学习游戏引擎开发的基础知识。作为初学者的作品，项目中难免存在不足和错误，但每一步都是学习的过程。引擎尝试实现一些基本功能，支持Windows和Android平台。

简体中文 | [English](../README.md)

## 🎯 项目目标

### 📚 学习目的
- 通过实践学习游戏引擎开发的基础知识
- 理解图形渲染、资源管理和系统架构的基本概念
- 探索跨平台开发的基本流程

### 🔧 尝试实现的功能
- **跨平台支持**: 尝试支持Windows和Android平台
- **渲染后端**: 尝试实现DirectX 12和Vulkan渲染支持
- **输入系统**: 基本的输入处理功能
- **音频系统**: 简单的音频播放功能
- **资源管理**: 基本的资源加载和管理机制

### ⚠️ 注意事项
- 这是一个学习项目，代码质量和架构设计可能不够完善
- 功能实现可能存在bug和性能问题
- 欢迎提出建议和指导，帮助作者改进和学习

## 📋 系统要求

### 开发环境
- **IDE**: Visual Studio 2022 或更新版本、CLion 或任何支持CMake的IDE
- **SDK**: Windows 10 SDK (10.0.22621.0+) 
- **构建系统**: CMake 3.24+
- **包管理**: vcpkg
- **移动开发**: Android NDK, Android SDK

### 运行时环境
- **Windows**: Windows 10+，DirectX 12兼容显卡
- **Android**: Android 5.0+，Vulkan兼容设备
- **图形API**: DirectX 12 或 Vulkan 1.1+

## 🛠️ 快速开始

### 1. 获取源代码

要克隆仓库及其子模块，请使用 `--recursive` 标志：

```bash
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine
```

如果您已经克隆了仓库但没有使用 `--recursive` 标志，可以单独初始化和更新子模块：

```bash
git submodule init
git submodule update
```

### 2. 设置开发环境
```bash
# 初始化vcpkg
./vcpkg/bootstrap-vcpkg.bat

# 安装依赖库
./vcpkg/vcpkg install
```

### 3. 构建项目
使用CMake预设：
```bash
# 配置项目
cmake --preset=windows-x64-debug

# 构建项目
cmake --build --preset=windows-x64-debug
```

或者使用Visual Studio方法：
1. 在Visual Studio中打开文件夹（文件 -> 打开 -> 文件夹）
2. 选择PrismaEngine根文件夹
3. Visual Studio会自动检测CMake配置
4. 构建解决方案 (Ctrl+Shift+B)

### 4. 运行示例
- **Editor**: 游戏编辑器应用
- **Runtime**: 游戏运行时环境

## 📁 项目结构

```
PrismaEngine/
├── src/              # 源代码
│   ├── core/         # 核心引擎模块
│   ├── editor/       # 游戏编辑器
│   ├── game/         # 游戏框架
│   └── runtime/      # 游戏运行时
├── projects/         # 平台相关项目文件
│   ├── android/      # Android项目
│   └── windows/      # Windows项目
├── docs/             # 文档资源
├── tools/            # 开发工具
├── vcpkg/            # 包管理器
├── CMakeLists.txt    # 主CMake配置
└── CMakePresets.json # CMake预设
```

## 📚 文档资源

- [📖 引擎架构](RenderingSystem.md) - 渲染系统详细说明
- [🗺️ 开发路线图](Roadmap.md) - 项目发展规划
- [💾 资源序列化](AssetSerialization.md) - 资源管理机制
- [📝 开发备忘录](MEMO.md) - 技术实现细节

## 📄 许可证

本项目采用 [MIT 许可证](../LICENSE) - 详情请参阅许可证文件。

## 📞 联系方式

- **项目主页**: [GitHub Repository](https://github.com/Excurs1ons/PrismaEngine)
- **问题反馈**: [Issues](https://github.com/Excurs1ons/PrismaEngine/issues)

---

*PrismaEngine - 一个初学者的游戏引擎学习项目*