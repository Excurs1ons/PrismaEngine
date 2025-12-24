# PrismaEngine 文档索引

## 📁 目录结构

```
docs/
├── Index.md                           # 本文件 - 文档导航索引
│
├── 📘 项目概述
│   ├── README.md                       # 项目总览（从仓库根链接）
│   ├── Roadmap.md                      # 开发路线图与功能完成度
│   └── Requirements.md                 # 引擎需求规格说明
│
├── 📗 核心系统设计
│   ├── RenderingSystem.md             # 渲染系统架构设计
│   ├── AssetSerialization.md           # 资源序列化系统
│   ├── EmbeddedResources.md           # 内嵌资源压缩系统（声明式）
│   ├── ScriptingSystem.md              # 脚本系统设计 [占位]
│   └── AudioSystem.md                  # 音频系统设计 [占位]
│
├── 📙 渲染架构
│   ├── RenderGraph_Migration_Plan.md  # RenderGraph 迁移计划
│   ├── rendering-architecture-comparison.md   # 渲染架构对比
│   ├── rendering-architecture-redesign.md    # 渲染架构重设计
│   └── rendering-refactoring-plan.md          # 渲染重构计划
│
├── 📕 高级功能集成
│   ├── RTXGI_Integration.md            # RTXGI 全局光照集成
│   ├── VulkanIntegration.md            # Vulkan 后端集成 [占位]
│   ├── SwappyIntegration.md            # Google Swappy 帧率管理 [占位]
│   └── HAPVideoSystem.md               # HAP 视频播放系统 [占位]
│
├── 📓 开发指南
│   ├── MEMO.md                         # 开发笔记
│   ├── LifecycleManagement.md          # 生命周期管理
│   ├── LifecycleManagement_Implementation.md  # 生命周期实现
│   ├── DeviceConfiguration.md          # 设备配置
│   ├── SCRIPTING_GUIDE.md              # 脚本编写指南
│   └── CodingStyle.md                  # 代码风格指南 [占位]
│
└── 📔 国际化
    ├── README_zh.md                    # 中文文档
    └── README_zh/                      # 中文翻译目录
```

## 📖 按主题浏览

### 渲染相关
- [RenderingSystem.md](RenderingSystem.md) - 渲染系统架构
- [RenderGraph_Migration_Plan.md](RenderGraph_Migration_Plan.md) - RenderGraph 迁移
- [rendering-architecture-comparison.md](rendering-architecture-comparison.md) - 架构对比
- [rendering-architecture-redesign.md](rendering-architecture-redesign.md) - 架构重设计
- [rendering-refactoring-plan.md](rendering-refactoring-plan.md) - 重构计划

### 资源管理
- [AssetSerialization.md](AssetSerialization.md) - 资源序列化
- [EmbeddedResources.md](EmbeddedResources.md) - 内嵌资源系统

### 高级功能
- [RTXGI_Integration.md](RTXGI_Integration.md) - RTXGI 集成

### 开发相关
- [MEMO.md](MEMO.md) - 开发笔记
- [LifecycleManagement.md](LifecycleManagement.md) - 生命周期管理
- [SCRIPTING_GUIDE.md](SCRIPTING_GUIDE.md) - 脚本指南

## 🏷️ 文档状态

| 文档 | 状态 | 说明 |
|------|------|------|
| RenderingSystem.md | ✅ 完整 | 渲染系统架构设计 |
| AssetSerialization.md | ✅ 完整 | 资源序列化系统 |
| EmbeddedResources.md | ✅ 完整 | 内嵌资源压缩系统 |
| RenderGraph_Migration_Plan.md | ✅ 完整 | RenderGraph 迁移计划 |
| RTXGI_Integration.md | ✅ 完整 | RTXGI 集成方案 |
| ScriptingSystem.md | 🔲 占位 | 脚本系统设计 |
| AudioSystem.md | 🔲 占位 | 音频系统设计 |
| VulkanIntegration.md | 🔲 占位 | Vulkan 后端集成 |
| SwappyIntegration.md | 🔲 占位 | Google Swappy 集成 |
| HAPVideoSystem.md | 🔲 占位 | HAP 视频播放系统 |
| CodingStyle.md | 🔲 占位 | 代码风格指南 |

## 🔗 快速链接

- [项目 README](../README.md)
- [开发路线图](Roadmap.md)
- [需求规格](Requirements.md)
- [开发笔记](MEMO.md)
