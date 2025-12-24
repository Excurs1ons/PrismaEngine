# Vulkan 后端集成

> **状态**: 🔲 规划中
> **优先级**: 高
> **依赖**: [PrismaAndroid](https://github.com/Excurs1ons/PrismaAndroid) Vulkan 运行时

## 概述

PrismaEngine 的 Vulkan 后端将从 PrismaAndroid 项目迁移，提供功能完整的跨平台渲染能力。

## 现状

PrismaAndroid 项目已包含约 1300 行功能完整的 Vulkan 实现：

- ✅ VulkanContext - Instance/Device/SwapChain 管理
- ✅ RendererVulkan - 完整渲染循环
- ✅ ShaderVulkan - SPIR-V 着色器加载
- ✅ TextureAsset - 纹理资源管理
- ✅ 屏幕旋转支持 (SwapChain 重建)

## 迁移计划

详见 [Roadmap.md - Vulkan 后端迁移计划](Roadmap.md#-vulkan-后端迁移计划)

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | 渲染抽象层设计 | 🔄 进行中 |
| Phase 2 | VulkanContext 迁移 | ⏳ 计划中 |
| Phase 3 | RendererVulkan 迁移 | ⏳ 计划中 |
| Phase 4 | Shader/Texture 迁移 | ⏳ 计划中 |
| Phase 5 | 集成测试与优化 | ⏳ 计划中 |

## 架构对齐

| PrismaAndroid | PrismaEngine | 迁移策略 |
|---------------|--------------|----------|
| `VulkanContext` | `RenderBackend` + `VulkanDevice` | 抽取接口，保留实现 |
| `RendererVulkan` | `VulkanRenderer` | 重构为适配器模式 |
| `ShaderVulkan` | `Shader` + `VulkanShader` | 统一着色器接口 |
| `TextureAsset` | `Texture` + `VulkanTexture` | 统一资源接口 |
| `Scene/GameObject` | 已有 ECS | 保持引擎架构，参考 Android 实现 |

## 迁移后的目录结构

```
src/engine/graphic/
├── RenderBackend.h           # 渲染后端抽象接口
├── VulkanBackend.h/cpp       # Vulkan 后端实现（迁移自 PrismaAndroid）
├── VulkanContext.h/cpp       # Vulkan 上下文（迁移自 PrismaAndroid）
├── VulkanCommandList.h/cpp   # Vulkan 命令列表封装
├── VulkanShader.h/cpp        # Vulkan 着色器（迁移）
├── VulkanTexture.h/cpp       # Vulkan 纹理（迁移）
└── ...
```

## 相关链接

- [PrismaAndroid Repository](https://github.com/Excurs1ons/PrismaAndroid)
- [Vulkan Guide](https://vulkan-guide.com/)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)

---

*文档创建时间: 2025-12-25*
