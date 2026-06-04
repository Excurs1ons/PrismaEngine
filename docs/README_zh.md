# Prisma Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20Android-lightgrey.svg)](https://github.com/Excurs1ons/PrismaEngine)
[![Build Status](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/Excurs1ons/PrismaEngine/actions)
[![Android APK](https://img.shields.io/badge/APK-下载-green.svg?logo=android)](https://github.com/Excurs1ons/PrismaEngine/releases/download/latest/PrismaAndroid.apk)

Prisma Engine 是一个使用现代 C++20 构建的跨平台 3D 游戏引擎，专注于高性能渲染和现代图形架构。

简体中文 | [English](../README.md)

> **当前状态**: Android Vulkan 运行时已达到生产级，Windows DirectX 12 后端处于深度开发阶段。
> **详细路线图**: [TODO.md](TODO.md) — 基于四维决策框架的优先级任务列表。
> **最后更新**: 2026-06-04

## CI/CD 状态

| 目标 | 平台 | 状态 | 触发方式 |
|------|------|------|----------|
| **CI** | Windows | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-windows.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-windows.yml) | 推送 / PR |
| **CI** | Android | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-android.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-android.yml) | 推送 / PR |
| **CI** | Linux | [![CI](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/ci-linux.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/ci-linux.yml) | 推送 / PR |
| **APK** | Android | [![Android Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-android-runtime.yml?branch=main&label=APK)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-android-runtime.yml) | 推送 / 手动 |
| **Engine** | Windows | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-engine.yml) | 推送 / 手动 |
| **Engine** | Android | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-android-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-android-engine.yml) | 推送 / 手动 |
| **Engine** | Linux | [![Engine](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-engine.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-engine.yml) | 推送 / 手动 |
| **Editor** | Windows | [![Editor](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-editor.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-editor.yml) | 推送 / 手动 |
| **Editor** | Linux | [![Editor](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-editor.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-editor.yml) | 推送 / 手动 |
| **Launcher** | Windows | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-windows-runtime.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-windows-runtime.yml) | 推送 / 手动 |
| **Launcher** | Android | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-android-runtime.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-android-runtime.yml) | 推送 / 手动 |
| **Launcher** | Linux | [![Runtime](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/build-linux-runtime.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/build-linux-runtime.yml) | 推送 / 手动 |
| **Release** | 全部 | [![Release](https://img.shields.io/github/actions/workflow/status/Excurs1ons/PrismaEngine/release.yml?branch=main&label=)](https://github.com/Excurs1ons/PrismaEngine/actions/workflows/release.yml) | 标签 (`v*.*.*`) |

> 模块进度详情见 [MODULE_PROGRESS.md](MODULE_PROGRESS.md)，开发优先级见 [TODO.md](TODO.md)。

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
- [TODO / 路线图](TODO.md) - 基于四维决策框架的优先级任务列表
- [架构优化说明](ArchitectureOptimization.md) - 最新的设计改进
- [Vulkan 集成详情](VulkanIntegration.md) - 详细的 Android 实现
- [RenderGraph 计划](RenderGraph_Migration_Plan.md) - 未来渲染架构路线图

## 核心特性

- **现代 C++20**: 利用 Concepts、Coroutines 和 Designated Initializers。
- **智能依赖管理**: 彻底告别手动库安装，一切交给 CMake。
- **统一渲染 API**: 一次编写，在 DX12 或 Vulkan 上同步运行。
- **Android 深度优化**: 通过 GameActivity 实现零延迟输入，以及高性能 Vulkan 渲染路径。

## 许可证

MIT 许可证 - 详见 [LICENSE](../LICENSE)。
