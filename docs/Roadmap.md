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
| **HAP 视频播放** | 低 | Snappy | [HAPVideoSystem.md](plans/HAPVideoSystem.md) |

### 相关库链接
- [SDL3](https://github.com/libsdl-org/SDL) - 跨平台多媒体层
- [Google Snappy](https://github.com/google/snappy) - 快速压缩库
- [Google Swappy](https://developer.android.com/games/sdk/frame-pacing) - Android 帧率控制
- [HAP Codec](https://github.com/Vidvox/hap) - GPU 加速视频编解码

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
- ✅ Linux/macOS ([SDL3](https://github.com/libsdl-org/SDL)/Vulkan) - PlatformSDL.cpp
- ✅ 日志系统统一接口
- ✅ 条件编译保护
- 🔄 输入系统完善中

### 其他系统
- 🔄 音频系统 (架构已定义)
- ❌ 物理系统 ([JoltPhysics](https://github.com/jrouwe/JoltPhysics))
- ❌ 动画系统 (角色/骨骼动画)
- ❌ UI 补间动画 ([Tweeny](https://github.com/mobius3/tweeny))
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
1. 高级渲染特性 ([RTXGI](https://github.com/NVIDIA-RTX/RTXGI))
2. 动画系统 (角色/骨骼)
3. 脚本系统

### UI 系统
- ❌ UI 框架 ([Dear ImGui](https://github.com/ocornut/imgui) 编辑器)
- ❌ 补间动画 ([Tweeny](https://github.com/mobius3/tweeny))

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

---
*最后更新: 2026-01-02*

---

## 📊 模块完成进度追踪

> 以下内容合并自 `ModuleProgress.md` (2026-05-22)

# PrismaEngine 模块完成进度

本文档跟踪 PrismaEngine 各模块的开发状态。

**最后更新**: 2026-05-18 (Template3D + SSBO 路径追踪)

---

## 最近活动 (24小时)

| 时间 | 模块 | 变更 |
|------|------|------|
| 09:22 | 路径追踪 | ✅ Template3D Cornell Box 路径追踪成功渲染（SSBO 架构） |
| 09:15 | 路径追踪 | ✅ UBO → SSBO 迁移（场景对象放入 SSBO，突破 UBO 65536 限制） |
| 09:10 | 路径追踪 | ✅ JSON 场景文件系统（Glaze 解析，支持 plane/sphere/box） |
| 09:00 | 路径追踪 | ✅ SPIR-V 重新编译，pathtrace.comp 重写为 SSBO 遍历 |
| 22:00 | MCP 协议 | ✅ 17 个工具，7 类覆盖，19 项测试全部通过 |
| 21:30 | WebUI 编辑器 | ✅ 浏览器访问编辑，视图 + 层级 + 检查器 + 控制台 |
| 21:17 | 脚本系统 | ✅ PrismaEngine.Core 迁移至引擎 (src/engine/scripting/CSharp) |
| 12:45 | 2D 渲染 | ✅ 极致 2D 合批系统 (DC 缩减 >99%, 支持 5万+ Quads) |
| 12:30 | 性能分析 | ✅ 实时性能 HUD + 瓶颈自动分析 (CPU/GPU/VSync/FPS Limit) |
| 12:15 | 输入系统 | ✅ 鼠标事件合并 (Coalescing) + 手柄指针缓存 (解决 3000FPS 掉帧) |
| 11:30 | 项目重构 | ✅ 2DTest 更名为 Template2D，UI 布局深度优化 |
| 11:00 | 渲染架构 | ✅ 三倍环形缓冲 (Ring Buffer) 实现，解决多帧资源竞争 |
| 21:19 | Android UI | ✅ Prisma Logo 矢量图标 (prisma_1.xml) |
| 20:44 | Android UI | ✅ 启动屏幕背景和动画配置 |
| 19:25 | 着色器 | ✅ PBR 光照着色器 (lit.vert/frag) |
| 12:28 | 着色器 | ✅ 无光照着色器 (unlit.vert/frag) |
| 12:28 | 构建系统 | ✅ 启用着色器编译到 CMake |
| 10:58 | Android 平台 | ✅ Android 输入后端实现 |
| 09:10 | 资源管理 | ✅ 资源池重构为类型安全模板 |
| 09:02 | 资源管理 | ✅ Handle<T> 替代 void* 句柄系统 |
| 08:53 | 渲染架构 | ✅ 重构为 核心 Pass + Feature 架构 |
| 02:57 | 渲染架构 | ✅ 5 个核心 Pass 定义 |
| 02:51 | 渲染架构 | ✅ BasicRenderer + IRenderFeature 基础设施 |

---

## 一、渲染系统 (Rendering)

### 1.1 渲染管线架构

| 模块 | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **BasicRenderer** | ✅ 已实现 | 85% | 主渲染器，集成 Feature 系统，Pass 定义完整 |
| **IRenderFeature** | ✅ 已实现 | 100% | Feature 接口 + RenderPassEvent 插入点 |
| **RenderFeatureManager** | ✅ 已实现 | 100% | Feature 生命周期管理 |
| **IRenderContext** | ✅ 已实现 | 100% | Feature 执行上下文接口 |
| **Architecture.md** | ✅ 已完成 | 100% | 架构设计文档 |

### 1.2 核心 Pass (5个)

| Pass | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **OpaquePass** | ✅ 头文件完成 | 60% | PBR 不透明物体渲染，定义在 BasicRenderer.h |
| **TransparentPass** | ✅ 头文件完成 | 60% | 透明物体渲染 |
| **SkyboxPass** | ✅ 头文件完成 | 60% | 天空盒渲染 |
| **ShadowPass** | ✅ 头文件完成 | 60% | 阴影贴图渲染 |
| **FinalBlitPass** | ✅ 头文件完成 | 60% | 最终输出 Blit |

### 1.3 渲染 Feature (9个)

| Feature | 状态 | 完成度 | 说明 |
|---------|------|--------|------|
| **BloomFeature** | 📋 已规划 | 0% | Bloom 发光效果 |
| **PostProcessFeature** | 📋 已规划 | 0% | 后处理容器（色调映射） |
| **AntiAliasingFeature** | 📋 已规划 | 0% | 抗锯齿 (FXAA/TAA) |
| **ScreenSpaceFeature** | 📋 已规划 | 0% | 屏幕空间效果 (SSAO/SSR) |
| **DebugFeature** | 📋 已规划 | 0% | 调试可视化 |
| **DepthPrepassFeature** | 📋 已规划 | 0% | 深度预渲染 |
| **UIFeature** | 📋 已规划 | 0% | UI 渲染 |
| **ReflectionFeature** | 📋 已规划 | 0% | 反射效果 |
| **VolumetricFeature** | 📋 已规划 | 0% | 体积光/雾 |

### 1.4 Feature 预设配置

| 配置 | 状态 | 说明 |
|------|------|------|
| **CreateDefaultFeatures** | ✅ 已定义 | 深度预通过 + SSAO + 后处理 + 抗锯齿 + UI |
| **CreateHighQualityFeatures** | ✅ 已定义 | 全效果（含 SSR、Bloom、体积光） |
| **CreateMobileFeatures** | ✅ 已定义 | 移动端精简版（仅后处理 + UI） |

---

## 二、资源管理 (Resources)

### 2.1 句柄系统

| 组件 | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **Handle<T>** | ✅ 已实现 | 100% | 类型安全句柄模板（索引+世代） |
| **RenderHandle.h** | ✅ 已实现 | 100% | 720行完整句柄系统 |
| **RenderHandle.cpp** | ✅ 已实现 | 100% | 334行完整实现 |

### 2.2 资源池

| 组件 | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **ResourcePool<T>** | ✅ 已实现 | 100% | 模板化资源池（FreeList + 世代计数） |
| **TexturePool** | ✅ 已实现 | 100% | 纹理池 |
| **BufferPool** | ✅ 已实现 | 100% | 缓冲池（含 Map/Unmap） |
| **TempTexturePool** | ✅ 已实现 | 100% | 帧临时纹理池 |

### 2.3 句柄类型

| 类型 | 状态 | 说明 |
|------|------|------|
| **TextureHandle** | ✅ 已定义 | 纹理句柄 |
| **BufferHandle** | ✅ 已定义 | 缓冲句柄 |
| **PipelineHandle** | ✅ 已定义 | 管线句柄 |
| **RenderPassHandle** | ✅ 已定义 | 渲染通道句柄 |
| **FramebufferHandle** | ✅ 已定义 | 帧缓冲句柄 |
| **ShaderHandle** | ✅ 已定义 | 着色器句柄 |
| **SamplerHandle** | ✅ 已定义 | 采样器句柄 |

### 2.4 渲染数据

| 组件 | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **RenderingData** | ✅ 已实现 | 80% | 渲染数据结构 |
| **LightingData** | ✅ 已实现 | 80% | 光照数据 |
| **ShadowSettings** | ✅ 已实现 | 80% | 阴影设置 |
| **RenderQueue** | ✅ 已实现 | 80% | 渲染队列管理 |
| **TextureDesc** | ✅ 已实现 | 100% | 纹理描述 |
| **BufferDesc** | ✅ 已实现 | 100% | 缓冲描述 |
| **SamplerDesc** | ✅ 已实现 | 100% | 采样器描述 |

---

## 三、着色器 (Shaders)

### 3.1 GLSL 着色器 (Vulkan #version 450)

| 着色器 | 状态 | 行数 | 说明 |
|--------|------|------|------|
| **lit.vert** | ✅ 已实现 | 36 | PBR 光照顶点着色器 |
| **lit.frag** | ✅ 已实现 | 36 | PBR 光照片段着色器 (Ambient+Diffuse+Specular) |
| **unlit.vert** | ✅ 已实现 | 23 | 无光照顶点着色器 |
| **unlit.frag** | ✅ 已实现 | ~10 | 无光照片段着色器 |
| **skybox.vert** | ✅ 已实现 | 30 | 天空盒顶点着色器 |
| **skybox.frag** | ✅ 已实现 | ~15 | 天空盒片段着色器 |
| **clearcolor.vert** | ✅ 已实现 | 15 | 纯色背景顶点 |
| **clearcolor.frag** | ✅ 已实现 | 10 | 纯色背景片段 |
| **shader.vert** | ✅ 已实现 | 20 | 通用顶点着色器 |

### 3.2 HLSL 着色器 (参考)

| 着色器 | 状态 | 说明 |
|--------|------|------|
| **PBRCommon.hlsl** | ✅ 已添加 | PBR 通用定义 |
| **Debug.hlsl** | ✅ 已添加 | 调试着色器 |

---

## 四、平台层 (Platform)

### 4.1 Android 平台

| 组件 | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **输入后端** | ✅ 已实现 | 80% | Android 输入系统封装 |
| **MainActivity** | ✅ 已添加 | 60% | Java 主活动 |
| **CMakeLists** | ✅ 已配置 | 100% | 着色器编译已启用 |

### 4.2 Android UI 资源

| 资源 | 状态 | 说明 |
|------|------|------|
| **prisma_1.xml** | ✅ 已添加 | Prisma Logo 矢量图标 (63行) |
| **splash_background.xml** | ✅ 已添加 | 启动屏幕背景 (日间/夜间) |
| **themes.xml** | ✅ 已更新 | 主题配置 |

---

## 五、工具类 (Utilities)

| 组件 | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **Camera** | ✅ 已实现 | 80% | 相机控制 |
| **Frustum** | ✅ 已实现 | 80% | 视锥体裁剪 |
| **DepthState** | ✅ 已实现 | 100% | 深度状态配置 |
| **StencilState** | ✅ 已实现 | 100% | 模板状态配置 |
| **Viewport** | ✅ 已实现 | 100% | 视口结构 |
| **Rect** | ✅ 已实现 | 100% | 裁剪矩形 |
| **ClearValue** | ✅ 已实现 | 100% | 清除值 |

---

## 六、整体完成度

### 按子系统

| 子系统 | 完成度 | 变化 | 说明 |
|--------|--------|------|------|
| **渲染架构** | 85% | ↑15% | 核心 Pass + Feature 架构完成 |
| **资源管理** | 95% | ↑5% | 句柄和池系统完整实现 |
| **着色器** | 50% | ↑10% | 光照着色器添加 |
| **平台层** | 70% | ↑10% | Android UI 资源完善 |
| **工具类** | 80% | - | 相机、裁剪等基础工具完成 |
| **MCP 协议** | 85% | 🆕 | 17 工具、7 分类、双传输、增量哈希 |
| **WebUI 编辑器** | 80% | 🆕 | 浏览器编辑器，Scene/Game 视口 |
| **路径追踪 (Template3D)** | 30% | 🆕 | Cornell Box SSBO 路径追踪，待 NEE + 降噪 |

### 按功能

| 功能 | 完成度 | 说明 |
|------|--------|------|
| **前向渲染** | 85% | Pass 架构和 Feature 系统完成 |
| **延迟渲染** | 0% | 已在配置中预留 |
| **PBR 光照** | 70% | 着色器完成，集成中 |
| **阴影** | 60% | ShadowPass 架构完成 |
| **后处理** | 0% | Feature 已规划，待实现 |
| **HDR/Bloom** | 0% | 详见 HDR_ROADMAP.md |

---

## 七、代码统计

| 类别 | 数量 | 总行数 |
|------|------|--------|
| **头文件** | 55 | ~8,400 |
| **源文件** | 29 | ~7,400 |
| **着色器 (GLSL)** | 10 | ~525 |
| **着色器 (HLSL)** | 2 | ~600 |
| **着色器 (Compute)** | 1 | ~289 |
| **UI 资源** | 4 | ~100 |
| **MCP 测试** | 3 | ~600 |
| **技能文件** | 1 | ~160 |
| **场景配置** | 1 | ~73 |
| **总计** | 106 | ~18,147 |

---

## 八、MCP 协议 (Model Context Protocol)

### 8.1 架构

| 组件 | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **MCPSubSystem** | ✅ 已实现 | 100% | ISubSystem 注册，引擎生命周期管理 |
| **MCPServer** | ✅ 已实现 | 100% | JSON-RPC 路由，方法派发 |
| **TransportStdio** | ✅ 已实现 | 100% | 子进程 stdio 模式 |
| **TransportTCP** | ✅ 已实现 | 90% | TCP socket 模式 (单客户端) |
| **MCPSession** | ✅ 已实现 | 90% | 会话管理 + 哈希增量追踪 |
| **DeltaTracker** | ✅ 已实现 | 100% | xxh3_64 哈希树，变化比阈值检测 |
| **TokenBudget** | ✅ 已实现 | 100% | Agent token 预算管理 |
| **ComponentSerializer** | ✅ 已实现 | 60% | 默认值省略框架，待具体组件补齐 |
| **ToolRegistry** | ✅ 已实现 | 100% | 分层注册，分类过滤，排序输出 |
| **MCPJson** | ✅ 已实现 | 100% | JSON-RPC 2.0 请求/响应序列化 |

### 8.2 工具清单 (共 17 个)

#### scene — 场景操作 (4 工具)

| 工具 | 方法 | 参数 | 状态 |
|------|------|------|------|
| 层级 | `scene_get_hierarchy` | `fields` | ✅ 已实现 |
| 详情 | `scene_get_entity` | `entity_id`, `fields` | ✅ 已实现 |
| 创建 | `scene_create_entity` | `name`, `parent_id` | ✅ 已实现 |
| 删除 | `scene_delete_entity` | `entity_id` | ✅ 已实现 |

#### ecs — 组件操作 (3 工具)

| 工具 | 方法 | 参数 | 状态 |
|------|------|------|------|
| 列表 | `ecs_component_list` | `entity_id` | ✅ 已实现 |
| 获取 | `ecs_component_get` | `entity_id`, `component_type` | ✅ 已实现 |
| 设置 | `ecs_component_set` | `entity_id`, `component_type`, `data` | ✅ 已实现 |

#### engine — 引擎 (3 工具)

| 工具 | 方法 | 描述 | 状态 |
|------|------|------|------|
| 状态 | `engine_get_status` | 引擎运行状态、FPS、场景信息 | ✅ 已实现 |
| 哈希 | `mcp/get_state_hash` | 根状态哈希（增量入口） | ✅ 已实现 |
| 构建 | `engine_get_build_info` | 编译器、平台、构建类型 | ✅ 已实现 |

#### debug — 调试 (2 工具)

| 工具 | 方法 | 描述 | 状态 |
|------|------|------|------|
| 帧数据 | `debug_frame_stats` | FPS、Draw Calls、三角面数 | ✅ 已实现 |
| 日志 | `debug_log_get` | 引擎日志（级别过滤 + 分页） | ✅ 已实现 |

#### asset — 资源 (2 工具)

| 工具 | 方法 | 描述 | 状态 |
|------|------|------|------|
| 列表 | `asset_list` | 按类型过滤、分页 | ✅ 已实现 |
| 详情 | `asset_get_info` | 资源元数据 | ✅ 已实现 |

#### editor — 编辑器 (2 工具)

| 工具 | 方法 | 描述 | 状态 |
|------|------|------|------|
| 选区 | `editor_get_selection` | 当前选中实体 | ✅ 已实现 |
| 控制台 | `editor_console_get` | 编辑器控制台输出 | ✅ 已实现 |

#### game — 运行时 (2 工具)

| 工具 | 方法 | 描述 | 状态 |
|------|------|------|------|
| 状态 | `game_get_state` | 游戏运行状态 | ✅ 已实现 |
| 推进 | `game_simulate` | 推进 N 帧 | ✅ 已实现 |

### 8.3 传输模式

| 模式 | CLI 参数 | 状态 | 说明 |
|------|----------|------|------|
| Stdio | `--mcp --mcp-transport=stdio` | ✅ 就绪 | 开发模式，Agent 管理子进程 |
| TCP | `--mcp --mcp-transport=tcp --mcp-port=3100` | ✅ 就绪 | 独立进程/远程调试 |

### 8.4 编译控制

```cmake
option(PRISMA_ENABLE_MCP "Enable MCP server for AI agent support" ON)
```

通过 CMake 选项 `PRISMA_ENABLE_MCP=OFF` 可完全禁用。

### 8.5 测试

| 测试模块 | 文件 | 测试数 | 状态 |
|----------|------|--------|------|
| JSON-RPC 序列化 | `tests/mcp/test_mcp_core.cpp` | 4 | ✅ 通过 |
| 工具注册 | `tests/mcp/test_mcp_core.cpp` | 3 | ✅ 通过 |
| 哈希增量 | `tests/mcp/test_mcp_core.cpp` | 5 | ✅ 通过 |
| 服务器路由 | `tests/mcp/test_mcp_core.cpp` | 5 | ✅ 通过 |
| Token 预算 | `tests/mcp/test_mcp_core.cpp` | 1 | ✅ 通过 |
| 会话管理 | `tests/mcp/test_mcp_core.cpp` | 1 | ✅ 通过 |
| Stdio 传输 | `tests/mcp/test_mcp_stdio.cpp` | — | ✅ 通过 |
| Python 集成 | `tests/mcp/test_mcp_client.py` | — | ✅ 通过 |

---

## 九、WebUI 编辑器

### 9.1 架构

| 组件 | 状态 | 完成度 | 说明 |
|------|------|--------|------|
| **WebUIEditor** | ✅ 已实现 | 90% | HTTP 服务器 + 前端页面宿主 |
| **httplib** | ✅ 已集成 | 100% | 内嵌 C++ HTTP 服务器 (header-only) |
| **EditorService** | ✅ 已实现 | 80% | 传输无关的编辑器服务接口 |
| **EditorSharedMemory** | ✅ 已实现 | 100% | 视口帧缓冲共享内存 |

### 9.2 功能

| 功能 | 状态 | 说明 |
|------|------|------|
| **场景视口 (Scene Viewport)** | ✅ 已实现 | 1280×720 RGBA 实时帧缓冲流，60fps 刷新 |
| **游戏视口 (Game Viewport)** | ✅ 已实现 | 独立游戏帧缓冲通道 |
| **层级面板 (Hierarchy)** | ✅ 已实现 | 实体列表，支持选中高亮 |
| **检查器 (Inspector)** | ✅ 已实现 | Transform 编辑 (X/Y/Z) |
| **控制台 (Console)** | ✅ 已实现 | 引擎日志实时输出 |
| **资源浏览器 (Asset Browser)** | ✅ 已实现 | 按路径浏览资产 |
| **状态栏 (Status Bar)** | ✅ 已实现 | FPS、场景名、物体数 |
| **视口交互** | ✅ 已实现 | 鼠标拖拽平移、滚轮缩放 |
| **响应式布局** | ✅ 已实现 | 移动端单栏/桌面端三栏自适应 |
| **实体创建** | ✅ 已实现 | New Entity 按钮 |
| **CORS 支持** | ✅ 已实现 | `Access-Control-Allow-Origin: *` |

### 9.3 API 端点

| 端点 | 方法 | 描述 |
|------|------|------|
| `/` | GET | 主页面 (内嵌完整 HTML5 编辑器) |
| `/api/v1/viewport/scene` | GET | 场景视口帧缓冲 (RGBA raw) |
| `/api/v1/hierarchy/get` | POST | 获取层级 |
| `/api/v1/entity/get` | POST | 获取实体详情 |
| `/api/v1/entity/update` | POST | 更新实体属性 |
| `/api/v1/entity/create` | POST | 创建实体 |
| `/api/v1/engine/status` | POST | 引擎运行状态 |
| `/api/v1/console/get` | POST | 控制台日志 |
| `/api/v1/assets/list` | POST | 资源列表 |
| `/api/v1/viewport/input` | POST | 视口输入事件 (鼠标/滚轮) |
| `/(.*)` | POST | 通用动作分发 (通配路由) |

### 9.4 启动方式

```bash
./bin/PrismaEditor --webui --webui-port=8080
```

然后浏览器访问 `http://localhost:8080`。

### 9.5 技术栈

- **WebUIEditor**: C++ httplib (header-only HTTP server)
- **前端**: 原生 HTML5 + CSS3 Grid + Canvas 2D + 原生 JavaScript (无框架依赖)
- **序列化**: glaze (glz::json_t)
- **帧缓冲**: EditorSharedMemory (共享内存)
- **通信**: RESTful JSON API + Canvas RGBA 流

### 9.6 已知限制

- 视口渲染目前为软件模拟 (Unity 蓝色背景 + 白色方块)，待接入真实 GPU 帧缓冲
- 单个客户端连接
- Inspector 仅支持 Transform 编辑

---

## 十、Template3D / 路径追踪

### 10.1 Template3D 项目

| 组件 | 状态 | 说明 |
|------|------|------|
| **Template3D** | ✅ 已完成 | 3D 路径追踪模板项目 |
| **CMake 构建** | ✅ 已完成 | 独立 CMakeLists，post-build 复制资产 |
| **SPIR-V 嵌入** | ✅ 已完成 | pathtrace.comp → PathtraceCompSPIRV.h |
| **全屏呈现管线** | ✅ 已完成 | fullscreen.vert + present.frag 呈现 |

### 10.2 SSBO 架构

| 组件 | 状态 | 说明 |
|------|------|------|
| **SceneDataSSBO** | ✅ 已完成 | std430 SSBO，存储 <32 个场景对象 |
| **CameraUBO** | ✅ 已完成 | std140 UBO，仅含相机 + 帧数据（96 bytes） |
| **对象类型** | ✅ 已完成 | Plane (0)、Sphere (1)、Box (2) |
| **JSON 场景** | ✅ 已完成 | Glaze 解析 pt_scene.json |
| **SSBO 回读验证** | ✅ 已完成 | GPU/CPU 数据一致性验证 |

### 10.3 路径追踪着色器

| 组件 | 状态 | 说明 |
|------|------|------|
| **intersectPlane** | ✅ 已完成 | AABB 限定的平面求交 |
| **intersectSphere** | ✅ 已完成 | 球体求交 |
| **intersectBox** | ✅ 已完成 | Y 轴旋转 AABB 求交 |
| **cosineSampleHemisphere** | ✅ 已完成 | Lambertian 漫反射采样 |
| **traceScene** | ✅ 已完成 | SSBO 对象遍历 + <= 优先级 |
| **gamma/tonemap** | ✅ 已完成 | Reinhard + 2.2 gamma |

### 10.4 已知问题

| 问题 | 级别 | 说明 |
|------|------|------|
| **窗口缩放不更新** | 中 | 路径追踪收敛后缩放窗口，渲染内容不跟随缩放（已修复，见 PathTracing.md Bug 6） |
| **Firefly 噪点** | 中等 | Monte Carlo 方差，需 firefly clamping |
| **低采样效率** | 中等 | Random walk 找不到光源 |
| **软件渲染** | 环境 | proot 下仅 llvmpipe，需真机硬件 |
| **退出时 crash** | 低 | "terminate called without an active exception" |

---

## 十一、近期计划

1. **Template3D NEE** - 实现 Next Event Estimation，提高采样效率 10-100x
2. **Firefly Clamping** - 颜色累积前 clamp 异常值
3. **Windows 硬件验证** - 在 Windows GPU 上测试实时性能
4. **更多几何体** - 支持三角形网格、变换矩阵
5. **2D 渲染增强** - 实施 `docs/plans/2026-05-16-advanced-2d-rendering-enhancements.md` 中的方案
6. **Pass 实现** - 完成 OpaquePass/TransparentPass 等 .cpp 实现
7. **Feature 实现** - 选择 1-2 个 Feature 完整实现
8. **PBR 集成** - 将 lit.frag/vert 集成到 OpaquePass

---

## 十一、图例

- ✅ **已完成** - 头文件和实现文件都完成
- 📋 **已规划** - 头文件存在，待实现
- 🚧 **进行中** - 正在开发
- ⏳ **计划中** - 已规划但未开始

---

*Last Updated: 2026-05-22 (Merged from ModuleProgress.md)*
