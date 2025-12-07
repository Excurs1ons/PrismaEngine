# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Prisma Engine](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build.yml)

Prisma Engine 是一个现代化的跨平台游戏引擎，专为轻量化/高性能游戏开发而设计。引擎采用模块化架构，支持Windows和Android平台，并计划扩展至更多平台。

简体中文 | [English](../README.md)

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

*PrismaEngine - 为现代游戏开发而生的高性能引擎*