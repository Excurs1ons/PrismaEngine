# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![CI Windows](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml)
[![CI Linux](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml)
[![CI Android](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml)
[![Android APK](https://img.shields.io/badge/APK-下载-green.svg?logo=android)](https://github.com/Excurs1ons/PrismaEngine/releases/download/latest/PrismaAndroid.apk)

Prisma Engine 是一个使用现代 C++23 构建的跨平台 3D 游戏引擎，专注于高性能渲染和现代图形架构。

简体中文 | [English](../README.md)

> **当前状态**: Android Vulkan 运行时已达到生产级，CoreCLR C# 脚本系统已集成，SoA Entity Pool 支持百万级虚拟容量。
> **最后更新**: 2026-05-25

## CI/CD 状态

| 平台 | 状态 | 触发方式 |
|------|------|----------|
| **Windows** | [![CI Windows](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml) | Push / PR |
| **Linux** | [![CI Linux](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml) | Push / PR |
| **Android** | [![CI Android](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml) | Push / PR |

## 当前进度

| 模块 | 状态 | 说明 |
|------|--------|------|
| SoA Entity Pool | ✅ 100% | 虚拟内存 1M 容量，双缓冲 |
| CoreCLR Scripting | ✅ 95% | C# Node/Script + SRP (CommandBuffer, RendererFeature, Compute Pipeline) |
| 渲染架构 | ✅ 90% | 核心 Pass + Feature 系统 + Compute Pipeline RHI |
| 资源管理 | ✅ 95% | Handle<T> 句柄系统 + 资源池 |
| Vulkan 后端 | ✅ 90% | 稳健的跨平台 Vulkan 实现 |
| DirectX 12 后端 | ⏳ 70% | Windows 主要渲染后端 |
| Platform 层 | ✅ 95% | 统一的 Windows/Linux/Android 抽象层 |
| Logger 系统 | ✅ 100% | 线程安全的跨平台日志 |
| 音频系统 | ✅ 50% | XAudio2/SDL3 后端，支持 3D 空间音频 |
| 着色器 | ✅ 50% | PBR 光照着色器 (lit/unlit) |
| Android 运行时 | ✅ 90% | 优化的 Vulkan 运行时（集成 GameActivity） |
| 编辑器工具 | ⏳ 15% | ImGui 基础检查器 |
| MCP 协议 | ✅ 85% | 17 工具、7 类别、双传输、Hash Delta 追踪 |
| WebUI Editor | ✅ 80% | 浏览器编辑器，包含场景/游戏视图、层级、检查器 |

**总体进度: ~82%**

## 快速开始

Prisma Engine 默认使用 **CMake FetchContent** 进行依赖管理，无需手动安装第三方库。

### Windows

```bash
# 克隆仓库
git clone --recursive https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# 使用 CMake Presets 构建
cmake --preset windows-x64-debug
cmake --build build/windows-x64-debug --parallel
```

### Linux

```bash
# 安装必要依赖 (Ubuntu/Debian)
sudo apt-get install libx11-dev libxrandr-dev libvulkan-dev

# 构建
cmake --preset linux-x64-debug
cmake --build build/linux-x64-debug --parallel
```

### Android

```bash
# 使用 Android Studio 打开
# 路径: projects/android/PrismaAndroid
# 所有依赖将通过 CMake FetchContent 自动下载
```

## 文档导航

- [文档索引](Index.md) - **从这里开始**
- [架构优化说明](ArchitectureOptimization.md) - 最新的设计改进
- [Vulkan 集成详情](VulkanIntegration.md) - 详细的 Android 实现
- [RenderGraph 计划](RenderGraphMigrationPlan.md) - 未来渲染架构路线图

## 核心特性

- **现代 C++23**: 利用 Concepts、Coroutines 和 Designated Initializers。
- **智能依赖管理**: 彻底告别手动库安装，一切交给 CMake。
- **统一渲染 API**: 一次编写，在 DX12 或 Vulkan 上同步运行。
- **Android 深度优化**: 通过 GameActivity 实现零延迟输入，以及高性能 Vulkan 渲染路径。

## 许可证

MIT 许可证 - 详见 [LICENSE](../LICENSE)。
