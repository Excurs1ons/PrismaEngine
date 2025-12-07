# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Status](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml)
[![Code Quality](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/code_quality.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/code_quality.yml)

Prisma Engine 是一个现代化的跨平台游戏引擎，专为轻量化/高性能游戏开发而设计。引擎采用模块化架构，支持Windows和Android平台，并计划扩展至更多平台。

简体中文 | [English](README.md)

## 🚀 核心特性

### 🎯 跨平台支持
- **Windows**: 原生DirectX 12支持，提供最佳性能
- **Android**: 完整的移动平台支持，基于Vulkan图形API
- **未来规划**: Linux、macOS等平台支持

### 🎮 可插拔后端系统
- **渲染后端**: 支持DirectX 12、Vulkan、SDL3等多种渲染API，运行时切换
- **输入后端**: 支持Win32、SDL3、DirectInput等多种输入系统
- **音频后端**: 支持XAudio2、SDL3等音频渲染引擎
- **统一接口**: 抽象的后端接口设计，便于扩展新后端

### 🎮 渲染系统
- **多后端渲染**: DirectX 12、Vulkan、SDL3可切换
- **现代图形特性**: 支持多线程渲染、Bindless纹理、实例化渲染
- **高级功能**: 异步计算、硬件光线追踪、瓦片渲染
- **实时渲染**: 动态光照、阴影和后期处理效果

### 🔧 引擎架构
- **组件系统**: 基于ECS（Entity-Component-System）架构
- **模块化设计**: 可插拔的子系统，便于扩展和维护
- **资源管理**: 智能资源加载、缓存和生命周期管理
- **音频系统**: 多后端音频渲染，支持WAV格式播放

## 📋 系统要求

### 开发环境
- **IDE**: Visual Studio 2022 或更新版本
- **SDK**: Windows 10 SDK (10.0.22621.0+) 
- **包管理**: vcpkg
- **移动开发**: Android NDK, Android SDK

### 运行时环境
- **Windows**: Windows 10+，DirectX 12兼容显卡
- **Android**: Android 5.0+，Vulkan兼容设备
- **图形API**: DirectX 12 或 Vulkan 1.1+

## 🛠️ 快速开始

### 1. 获取源代码
```bash
git clone https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine
```

### 2. 设置开发环境
```bash
# 安装vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# 安装依赖库
vcpkg install
```

### 3. 构建项目
1. 使用Visual Studio打开 `PrismaEngine.sln`
2. 选择目标平台和配置（Debug/Release）
3. 构建解决方案 (Ctrl+Shift+B)
4. 运行示例项目

### 4. 运行示例
- **Editor**: 游戏编辑器应用
- **Runtime**: 游戏运行时环境
- **EngineTest**: 引擎功能测试

## 📁 项目结构

```
PrismaEngine/
├── Engine/           # 核心引擎模块
│   ├── include/     # 公共头文件
│   ├── src/         # 实现文件
│   └── Engine.vcxitems  # 项目配置
├── Editor/          # 游戏编辑器
├── Runtime/         # 游戏运行时
├── EngineTest/      # 引擎测试
├── EditorTest/      # 编辑器测试
├── RuntimeTest/     # 运行时测试
├── Docs/           # 文档资源
└── Tools/          # 开发工具
```

## 🔬 核心模块

### 后端系统
- **RenderBackend**: 抽象渲染后端接口，支持DirectX 12、Vulkan、SDL3
- **InputBackend**: 抽象输入后端接口，支持Win32、SDL3、DirectInput
- **AudioBackend**: 抽象音频后端接口，支持XAudio2、SDL3
- **后端管理**: 运行时后端切换和热插拔支持

### 渲染系统
- **Renderer**: 统一渲染接口，支持多后端
- **RenderSystem**: 渲染管线管理，支持多线程渲染
- **MeshRenderer**: 网格渲染组件，支持实例化
- **Camera2D**: 2D相机系统，支持视口管理
- **Shader**: 着色器管理系统，支持HLSL/GLSL

### 场景管理
- **Scene**: 场景管理，支持实体层次结构
- **GameObject**: 游戏对象基类，组件容器
- **Transform**: 变换组件，支持2D/3D变换
- **Component**: 组件基类系统，支持序列化

### 输入系统
- **KeyCode**: 键盘输入映射，跨平台键码统一
- **InputManager**: 输入事件处理，多后端支持
- **跨平台输入**: 统一键盘、鼠标、触摸输入处理

## 📚 文档资源

- [📖 引擎架构](RenderingSystem.md) - 渲染系统详细说明
- [🗺️ 开发路线图](Roadmap.md) - 项目发展规划
- [💾 资源序列化](AssetSerialization.md) - 资源管理机制
- [📝 开发备忘录](MEMO.md) - 技术实现细节

## 📄 许可证

本项目采用 [MIT 许可证](LICENSE) - 详情请参阅许可证文件。

## 📞 联系方式

- **项目主页**: [GitHub Repository](https://github.com/Excurs1ons/PrismaEngine)
- **问题反馈**: [Issues](https://github.com/Excurs1ons/PrismaEngine/issues)

---

*PrismaEngine - 为现代游戏开发而生的高性能引擎*