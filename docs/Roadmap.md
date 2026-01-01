# PrismaEngine 功能路线图

## 项目概述

PrismaEngine（原YAGE）是一个为现代游戏开发设计的跨平台游戏引擎，支持Windows和Android平台，使用DirectX 12和Vulkan作为渲染后端。

**相关项目**: [PrismaAndroid](https://github.com/Excurs1ons/PrismaAndroid) - Vulkan运行时实现 (~1300行)

## 功能完成度

| 模块 | 完成度 | 状态 |
|------|--------|------|
| 基础架构 (ECS/Scene/Resource) | 75% | 🟡 进行中 |
| DirectX 12 后端 | 65% | 🟡 进行中 |
| Vulkan 后端 (PrismaAndroid) | 85% | 🟢 已完成 |
| Platform 层 | 95% | 🟢 已完成 |
| Logger 系统 | 95% | 🟢 已完成 |
| 音频系统 | 40% | 🟡 进行中 |
| 跨平台支持 | 80% | 🟢 已完成 |
| 编辑器工具 | 10% | 🔴 未开始 |
| 物理系统 | 5% | 🔴 未开始 |

## Vulkan 后端迁移计划

[PrismaAndroid](https://github.com/Excurs1ons/PrismaAndroid) 包含功能完整的 Vulkan 运行时实现，正在逐步迁移到 PrismaEngine。

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | 渲染抽象层设计 | 🔄 进行中 |
| Phase 2 | VulkanContext 迁移 | ⏳ 计划中 |
| Phase 3 | RendererVulkan 迁移 | ⏳ 计划中 |
| Phase 4 | Shader/Texture 迁移 | ⏳ 计划中 |
| Phase 5 | 集成测试与优化 | ⏳ 计划中 |

详细文档: [VulkanIntegration.md](VulkanIntegration.md)

## 高级渲染功能规划

| 功能 | 优先级 | 依赖 | 文档 |
|------|--------|------|------|
| **HLSL → SPIR-V** | 高 | Vulkan迁移 | [Roadmap详情](#-高级渲染功能规划) |
| **Google Snappy** | 高 | - | [EmbeddedResources.md](EmbeddedResources.md) |
| **Google Swappy** | 中 | Vulkan迁移 | [SwappyIntegration.md](SwappyIntegration.md) |
| **HAP 视频播放** | 低 | Snappy | [HAPVideoSystem.md](HAPVideoSystem.md) |

## 各系统实现状态

### 2D/3D 渲染系统
- ✅ ECS组件系统
- ✅ 相机系统 (Camera2D/3D)
- ✅ 变换系统 (Transform/Transform2D)
- ✅ DirectX 12 渲染器
- 🔄 前向渲染管线
- ❌ 阴影渲染
- ❌ 后处理效果

### 跨平台支持
- ✅ Windows平台 (DirectX 12) - PlatformWindows.cpp
- ✅ Android平台 (Vulkan) - PlatformAndroid.cpp
- ✅ Linux/macOS (SDL3/Vulkan) - PlatformSDL.cpp
- ✅ 日志系统统一接口
- ✅ 条件编译保护
- 🔄 输入系统完善中

### 其他系统
- 🔄 音频系统 (架构已定义)
- ❌ 物理系统 (JoltPhysics集成)
- ❌ 动画系统
- 🔄 编辑器框架

## 开发优先级

### 高优先级
1. 完善 Vulkan 迁移
2. 完善输入系统
3. 完善音频系统

### 中优先级
1. 跨平台支持完善
2. 基础编辑器功能
3. 物理引擎集成

### 低优先级
1. 高级渲染特性 (RTXGI)
2. 动画系统
3. 脚本系统

## 文档索引

完整文档导航见: [Index.md](Index.md)

## 最近更新

### 2026-01-02
- ✅ 重构组件系统和游戏对象管理
  - 优化 ECS 架构，组件生命周期管理更清晰
  - 游戏对象与组件的关联关系改进
- ✅ 添加 Android 平台日志系统实现
  - 集成 game-activity 库支持
  - 完善 Android 构建配置
  - 调整 CMake 配置以支持 Android 平台

### 2025-12-28
- ✅ 重构 Platform 为静态函数接口
- ✅ 合并 PlatformWindows/PlatformSDL/PlatformAndroid 到单一类
- ✅ 新增 IPlatformLogger 接口，打破循环依赖
- ✅ Android 支持 logcat 日志输出
- ✅ 添加条件编译保护跨平台头文件依赖

---

*最后更新: 2026-01-02*
