# Prisma2D SRP 风格 2D 光照与后处理 — 差距分析

> 日期：2026-05-12
> 目标：评估 PrismaEngine Prisma2D 项目实现类 Unity SRP 2D Renderer 风格的 2D 光照与后处理所需的差距。

---

## 1. 当前架构总览

### 1.1 Prisma2D 项目结构

```
projects/Prisma2D/
├── CMakeLists.txt          # 共享库 (DLL) 构建，链接 Engine
├── assets/
│   ├── project.json        # 项目配置
│   └── scenes/2d_test.jsonc # 5 个静态精灵场景
├── src/
│   ├── main.cpp            # 独立可执行入口
│   ├── CreateApplication.cpp # DLL 插件入口
│   └── Prisma2DApp.cpp   # 应用实现 (渲染循环、OnRender、HUD)
└── scripts/
    └── GameScripts/        # C# 脚本项目 (CoreCLR, .NET 10)
        ├── SceneInit.cs    # 创建 20 个动态精灵
        ├── RotatingSprite.cs # 旋转动画
        └── CameraController.cs # 相机控制
```

- **双启动模式**：独立 exe 或 Launcher 加载的 DLL 插件
- **C++/C# 互操作**：CoreCLR 托管，PrismaAPI 函数指针桥接 + SoA 共享缓冲区

### 1.2 渲染管线架构

```
应用层
  └─ Renderer2D::DrawQuad() → 批处理顶点 → Renderer::Submit()
       └─ RenderSystem::EndFrame()
            └─ ForwardPipeline::Execute()
                 ├─ DepthPrePass (优先级50)
                 ├─ OpaquePass (优先级100)   ← Renderer2D 使用同一着色器
                 ├─ SkyboxPass (优先级200)
                 └─ TransparentPass (优先级300)
```

| 组件 | 状态 | 说明 |
|------|------|------|
| **Renderer2D** | ✅ 完整 | 静态批处理 Quad 渲染器（MAX_BATCH_QUADS=50000，3 帧槽轮换） |
| **ForwardPipeline** | ✅ 完整 | 4 个 Pass 顺序执行，默认管线 |
| **DeferredPipeline** | ⚠️ 存在但未启用 | Geometry→Lighting→Skybox→Transparent→Composition |
| **Light2D** | ❌ 空壳 | 仅有数据类（Type/Position/Color/Radius），Update() 为空 |
| **CompositionPass** | ❌ 空壳 | 定义了后处理枚举（Bloom/FXAA/SMAA/SSR/SSAO/DOF），但 Execute() 不实际绘制 |
| **RenderGraph** | ❌ 未激活 | 两个版本均定义了接口，Compile()/Execute() 未实现 |

### 1.3 C# 渲染 API 现状

**PrismaAPI 结构体（C++→C# 桥接）：**

```csharp
internal unsafe struct PrismaAPI
{
    // 日志
    delegate* unmanaged<byte*, byte*, void> Log;
    // 实体生命周期
    delegate* unmanaged<uint> CreateEntity;
    delegate* unmanaged<uint, void> DestroyEntity;
    // SoA 缓冲区访问
    delegate* unmanaged<TransformBufferSoA*> GetTransformBufferA;
    delegate* unmanaged<TransformBufferSoA*> GetTransformBufferB;
    delegate* unmanaged<RenderBufferSoA*> GetRenderBuffer;
    // 输入
    delegate* unmanaged<int, bool> IsKeyDown;
    delegate* unmanaged<float> GetMouseX, GetMouseY;
    // 时间
    delegate* unmanaged<float> GetDeltaTime;
    // 相机
    delegate* unmanaged<float, float, void> SetCameraPos;
    delegate* unmanaged<float*, float*, void> GetCameraPos;
    // 无！ 无 DrawQuad, 无纹理, 无材质, 无光照, 无后处理 API
}
```

**C# 能做什么：**
- 读写 `TransformBufferSoA`（双缓冲） — Position/Rotation/Scale
- 写 `RenderBufferSoA` — colorR/G/B/A, sizeW/sizeH
- 创建/销毁实体
- 输入查询

**C# 不能做什么：**
- ❌ 无 DrawQuad/DrawSprite/DrawMesh
- ❌ 无纹理加载/创建
- ❌ 无材质/着色器控制
- ❌ 无光源创建/配置
- ❌ 无后处理控制
- ❌ 无 Pipeline/Camera 控制

### 1.4 SoA 缓冲区关键问题

`RenderBufferSoA` 虽然暴露给 C# 写入颜色和尺寸数据，但 **C++ 渲染代码完全不读取它**。这是个死数据的单向通道：

```
C# 写入:  NativeAPI.RenderBuffer->ColorR[idx] = ...
         NativeAPI.RenderBuffer->SizeW[idx] = ...
         ↓
C++ 读取: 无！ Renderer2D 不迭代 RenderBufferSoA
         SpriteRenderer 使用自己的 m_color/m_size 字段
```

---

## 2. 与 Unity SRP 2D Renderer 的功能差距

### 2.1 2D 光照系统

| 功能 | Unity 2D Renderer | PrismaEngine | 差距 |
|------|-------------------|--------------|------|
| **Point Light 2D** | 核心光照类型，半径衰减 | ❌ 无 | 需完整实现 2D 光照着色器 + 光照纹理渲染 |
| **Freeform Light 2D** | 用户自定义多边形形状光源 | ❌ 无 | 需多边形光栅化 + 形状编辑器 |
| **Global Light 2D** | 全局光照，环境光统一照亮 | ❌ 无 | 简单，可作为第一步实现 |
| **Sprite Light 2D** | 用精灵形状作为光源 | ❌ 无 | 需精灵投影到光照纹理 |
| **法线贴图** | Light 2D 与法线贴图混合产生 3D 立体感 | ❌ 无 | 需法线贴图管线 + Sprite 法线支持 |
| **阴影** | 精灵投射/接收阴影 | ❌ 无 | 需 ShadowCaster2D + 阴影贴图渲染 |
| **阴影强度** | 阴影透明度控制 | ❌ 无 | 简单 |
| **光源排序** | 按 Light Order 控制覆盖 | ❌ 无 | 简单 |
| **混合模式** | Additive/Multiply/Subtractive 光源混合 | ❌ 无 | 需着色器支持 |
| **体积光照** | 光源体积效果 | ❌ 无 | 高级效果 |

### 2.2 后处理系统

| 功能 | Unity 2D Renderer | PrismaEngine | 差距 |
|------|-------------------|--------------|------|
| **Bloom** | 泛光效果 | ❌ 枚举定义，无实现 | 需完整 Bloom 管线（Downsample + Upsample + Blur） |
| **Color Grading** | 色彩分级 | ❌ 无 | 需 LUT 或参数化调色 |
| **Blur** | 模糊效果 | ❌ 无 | 需高斯模糊或其他模糊算法 |
| **Tonemapping** | 色调映射 | ⚠️ 枚举定义，无实现 | 需实现 ACES/Reinhard 等 |
| **Gamma Correction** | Gamma 校正 | ⚠️ 枚举定义，无实现 | 简单 |
| **FXAA/SMAA** | 抗锯齿 | ⚠️ 枚举定义，无实现 | 复杂，需完整的边缘检测 + 混合 |
| **SSAO/SSR/DOF** | 高级后处理 | ⚠️ 枚举定义，无实现 | AAA 级，近期可不考虑 |

### 2.3 管线架构

| 功能 | Unity 2D Renderer | PrismaEngine | 差距 |
|------|-------------------|--------------|------|
| **2D 专属 RenderPipeline** | Lightweight 2D SRP | ❌ 无 | ForwardPipeline 不感知 2D 光照 |
| **Renderer2D Pass 注册** | 2D Renderer 是可组合的 Pass | ❌ 无 | Light2D 不集成到任何 Pass 中 |
| **Camera Stacking** | 多相机分层渲染 | ❌ 无 | 单个 OrthographicCamera |
| **自定义 Pass 注入** | RendererFeatures | ❌ 无 | 无 IPass 注册机制用于外部扩展 |
| **RenderGraph** | SRP 核心调度 | ⚠️ 存在但未激活 | 两个 RenderGraph 版本均未启用 |

### 2.4 C# 渲染 API

| 功能 | Unity 2D Renderer | PrismaEngine | 差距 |
|------|-------------------|--------------|------|
| **光源创建** | `gameObject.AddComponent<Light2D>()` | ❌ 无 | C# 无法创建/配置光源 |
| **法线贴图** | Sprite.NormalMap | ❌ 无 | 无法线贴图概念 |
| **后处理控制** | Volume 组件 | ❌ 无 | C# 无法配置后处理 |
| **渲染回调** | OnRenderImage/OnBeforeRender | ❌ 无 | 无渲染管线事件 |
| **材质控制** | material.SetFloat/SetTexture | ❌ 无 | C# 无法访问材质 |

---

## 3. 缺失组件清单

### 3.1 关键缺失（Phase 1-2 必须实现）

| 组件 | 优先级 | 说明 |
|------|--------|------|
| **2D 光照管线 Pass** | 🔴 P0 | 将 Light2D 渲染到光照纹理（使用 additive blending） |
| **Light2D Point Shader** | 🔴 P0 | 基于距离衰减的 2D 点光照着色器 |
| **光照纹理合成** | 🔴 P0 | 光照纹理与场景颜色纹理在 TransparentPass 中合成 |
| **Light2D 数据完善** | 🔴 P0 | 添加光源强度、衰减曲线、混合模式等参数 |
| **ShadowCaster2D 组件** | 🟡 P1 | 精灵投射阴影的支持（阴影贴图） |
| **阴影渲染 Pass** | 🟡 P1 | 将阴影写入阴影贴图 |
| **法线贴图管线** | 🟡 P1 | 为 SpriteRenderer 添加法线贴图支持，光照中采样法线 |

### 3.2 中型缺失（Phase 3 实现）

| 组件 | 优先级 | 说明 |
|------|--------|------|
| **Bloom Pass/Shader** | 🟡 P1 | 完整的 Bloom 渲染通道（Downsample FrameBuffer x4 → Upsample + Blur → Composite） |
| **Color Grading 着色器** | 🟡 P1 | LUT-based 或参数化色彩调色 |
| **Blur Pass** | 🟡 P1 | 高斯模糊或其他模糊效果 |
| **CompositionPass 实际实现** | 🟡 P1 | 将枚举定义变成实际的后处理链 |
| **Tonemapping 着色器** | 🟢 P2 | ACES/Reinhard 等色调映射算法 |

### 3.3 架构缺失（Phase 4-5 实现）

| 组件 | 优先级 | 说明 |
|------|--------|------|
| **C# 光源 API** | 🟡 P1 | 添加 function pointer，允许 C# 创建/删除/配置光源 |
| **C# 后处理 API** | 🟢 P2 | 允许 C# 启用/禁用/配置后处理效果 |
| **2D RenderPipeline** | 🟢 P2 | 为 2D 光照创建专用管线（非 ForwardPipeline 子类） |
| **Camera Stacking** | 🟢 P2 | 多相机分层渲染 |
| **Renderer2D 集成 SoA** | 🟡 P1 | 让 Renderer2D 迭代 RenderBufferSoA 渲染 C# 实体的颜色和尺寸 |
| **自定义 Pass/Feature** | 🟢 P2 | 允许 C# 或 C++ 向管线注入自定义 Pass |
| **RenderGraph 激活** | 🟢 P3 | 启用 RenderGraph 编译和执行 |

---

## 4. 实现路线图

### Phase 1: 2D 光照核心（估计：2-3 周）

目标：实现基本的 2D 光照渲染，支持 Point Light 2D 和 Global Light 2D。

1. **Light2D 数据完善**
   - 添加 `intensity`、`falloffCurve`、`blendMode` 参数
   - 添加 `lightOrder`（控制覆盖层级）
   - 添加 `volumetricIntensity`（体积光准备）

2. **光照纹理框架**
   - 创建离屏光照渲染目标（与主渲染目标同尺寸）
   - 初始化时设置为黑色（默认无光照）

3. **Light2DPass**
   - 新的渲染 Pass（优先级 120，在 OpaquePass 后、TransparentPass 前）
   - 逐个光源渲染到光照纹理（additive blending）
   - 点光源：全屏 Quad，shader 计算每个像素到光源中心的距离

4. **PointLight2D 着色器**
   - GLSL 450，接收光源位置/颜色/强度/半径
   - 像素输出 = `color * (1 - dist/radius) * intensity`

5. **光照合成 Pass**
   - 在 TransparentPass 中，渲染时将光照纹理采样叠加到场景颜色上
   - 可选：支持 Multiply/Additive/Subtractive 混合模式

6. **C# 基础光照 API**
   - 添加 `CreateLight`/`DestroyLight`/`SetLightPosition` function pointer

### Phase 2: 阴影与法线贴图（估计：2-3 周）

目标：添加阴影投射/接收和法线贴图支持。

1. **ShadowCaster2D 组件**
   - 数据类：阴影投射形状（多边形轮廓）
   - SpriteRenderer 扩展：`CastShadows`、`ReceiveShadows` 标志

2. **阴影贴图渲染**
   - 创建阴影贴图纹理
   - 将投射阴影的精灵渲染到阴影贴图
   - 利用精灵的 alpha 通道裁剪

3. **阴影合成**
   - 在光照 Pass 中采样阴影贴图
   - 被阴影覆盖的像素不接收光照

4. **法线贴图管线**
   - SpriteRenderer 添加 `normalMap` 纹理字段
   - 法线贴图渲染到法线纹理（或作为 Sprite 属性传入光照 shader）
   - 光照着色器采样法线计算光照

5. **Freeform Light 2D**
   - 支持用户定义的多边形形状
   - CPU 端计算形状覆盖区域，仅渲染受影响像素

### Phase 3: 后处理系统（估计：2-3 周）

目标：让 CompositionPass 真正工作，实现关键后处理效果。

1. **后处理渲染框架**
   - 完整 CompositionPass 实现
   - 按启用的效果链式处理（ping-pong 纹理）
   - 效果启用/禁用/配置参数系统

2. **Bloom**
   - 提取亮部 → 4 次 Downsample → 4 次 Upsample + Blur → 叠加回原图
   - 参数：threshold、intensity、scatter、tint

3. **Color Grading**
   - LUT 生成（16x16x16 查色表）
   - 参数调色：色相/饱和度/亮度/对比度
   - 可选：温度/色调偏移

4. **Blur**
   - 高斯模糊（水平+垂直 Pass）
   - 参数：强度、迭代次数

5. **Tonemapping**
   - ACES（推荐默认）、Reinhard、Neutral 算法
   - 参数：exposure

6. **C# 后处理 API**
   - 添加 `SetPostProcessingEffect` function pointer
   - 添加 `GetPostProcessingStatus` / `SetPostProcessingParameter`

### Phase 4: C# 渲染 API 桥接（估计：1-2 周）

目标：让 C# 对渲染有更完整的控制力。

1. **C# 渲染函数指针**
   - `DrawQuad`（位置、大小、颜色、纹理）
   - `CreateTexture`（从文件或从 C# 像素数据）
   - `SetShaderGlobal`（设置全局 shader 参数）

2. **SoA 渲染集成**
   - 让 Renderer2D 在每帧迭代 RenderBufferSoA
   - 对每个 active 的实体，用其颜色和尺寸调用 DrawQuad
   - 这样 C# 写入的颜色/尺寸数据将自动渲染

3. **C# 光源对象模型**
   - `Light2D` C# 类封装 API
   - `Node.AddComponent<Light2D>()` 风格 API
   - 自动同步到 C++ Light2D 数据

4. **C# Volume 类比**
   - 后处理效果通过 C# 脚本控制
   - `PostProcessVolume` 类似 Unity Volume 组件

### Phase 5: 管线架构升级（估计：3-4 周）

目标：建立更灵活的渲染管线架构。

1. **2D RenderPipeline 专用管线**
   - 从 ForwardPipeline 独立出来
   - 内置 Pass 序列：OpaquePass → Light2DPass → ShadowPass → TransparentPass → CompositionPass
   - 可选：后处理开关

2. **Camera Stacking**
   - 支持 Base Camera + Overlay Camera
   - 多相机渲染到不同图层，最后合成
   - 可用于 UI/World 分离渲染

3. **自定义 Pass 注入**
   - 类似 `RendererFeatures` 的机制
   - C++ 端：`RenderPipeline::AddFeature(IRendererFeature*)`
   - C# 端：允许遍历 Pipeline 添加自定义 Pass

4. **RenderGraph 激活**
   - 将 RenderGraphCore 集成到主渲染流程
   - 自动资源管理和 Pass 排序
   - 可选 Pass 剔除（未使用的 Pass 跳过）

---

## 5. 总结与建议

### 当前状态评分

| 维度 | 分数 | 说明 |
|------|------|------|
| 批处理渲染 (Renderer2D) | ★★★★★ | 成熟，支持 50000 Quad 批处理 |
| 管线架构 | ★★★☆☆ | 基本架构存在，但无 2D 光照专用管线 |
| 2D 光照 | ★☆☆☆☆ | Light2D 类存在但未集成到渲染流程 |
| 后处理 | ★☆☆☆☆ | 枚举定义存在但无实际效果实现 |
| C# 渲染 API | ★★☆☆☆ | 仅数据通道，无渲染控制力 |
| RenderGraph | ★★☆☆☆ | 设计完备但未激活 |
| 阴影/法线贴图 | ★☆☆☆☆ | 完全缺失 |

### 立即行动建议

1. **最优先**：让 `Light2D` 渲染起来。实现 Point Light 2D 的光照纹理 Pass + 合成，这是 SRP 风格 2D 光照的基石。
2. **次优先**：修复 `RenderBufferSoA` 的死数据问题——让 Renderer2D 在每帧迭代并渲染 SoA 中的实体。
3. **第三优先**：实现 Bloom + Color Grading，让画面质量有质的提升。
4. **基础不做**：SSAO/SSR/DOF 在 2D 中不必要，优先跳过。
5. **不急于做**：RenderGraph 激活、Camera Stacking 等架构升级可等光照和后处理稳定后再做。

### 构建验证命令

```bash
# Prisma2D 调试构建
cmake --preset engine-windows-x64-debug
cmake --build --preset engine-windows-x64-debug

# C# 脚本构建
dotnet build projects/Prisma2D/scripts/GameScripts/GameScripts.csproj
```
