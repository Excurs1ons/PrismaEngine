# 路径追踪系统

## 当前实现 (Template3D)

Template3D 包含一个纯 compute shader 实现的路径追踪器，位于 `projects/Template3D/`。

### 架构

```
   BeginFrame (默认 RenderPass 开始)
        │
        ▼
   RenderPathTracing()
        ├── EndSwapChainRenderPass()      ← 结束交换链 RP（compute 不能在 RP 内）
        ├── PipelineBarrier (Undefined → UAV)
        ├── 填充 Camera UBO (位置/方向/fov/帧计数)
        ├── SetComputePipeline / BindDescriptorSet
        ├── Dispatch (8×8 线程组)
        ├── PipelineBarrier (UAV → ShaderRead)
        ├── 收敛检测 (frameCount ≥ maxSamples)
        └── 提交 gizmo stats 到队列
        │
        ▼
   EndFrame → 重新开始 RenderPass
        ├── OnPresentOverlay (全屏四边形采样 storageTexture)
        │   └── ProcessGizmoOverlay (stats 文字)
        └── 结束 RenderPass
```

### 着色器

`assets/shaders/pathtrace.comp` — GLSL 4.60 compute shader:

- **光线求交**: 纯数学计算（无 RT Core 加速）
  - `intersectPlane()` — 点到平面 + bounds 检查
  - `intersectSphere()` — 二次方程求根
  - `intersectBox()` — AABB slab 算法（带 Y 轴旋转）
  - `traceScene()` — for 循环遍历 SSBO 中所有物体
- **路径追踪**: `tracePath()` — 半球采样 + 俄式轮盘赌，最多 10 次反弹
- **累积**: 两纹理（outputImage + accumImage），逐帧混入新采样

### 收敛检测

- `m_ptMaxSamples`：默认 4096 帧（可在 `Template3DApp.h` 修改，0 = 无限制）
- `m_ptConverged`：达到上限后冻结 frameCount，跳过所有 GPU dispatch
- 显示：橙色 `PathTrace: N/4096` → 绿色 `Converged: N/4096`
- 重置：R 键（重置累积）、P 键（切换模式）、窗口 resize

### 场景数据

场景从 JSON 文件加载（`assets/scenes/pt_scene.json`），通过 SSBO 传入 compute shader。支持三种图元类型：
- `type=0`: 平面（point + normal + UV bounds）
- `type=1`: 球体（center + radius）
- `type=2`: 盒子（center + half-size + Y 轴旋转）

### Cornell Box (Forward3D 模式)

- 5 墙面 + 1 光源 + 2 内部方块，每面独立顶点颜色
- 使用 gizmo PSO（`Renderer2D.vert` + `UnlitVertex.frag`）+ 透视投影
- 与路径追踪共用同一个 overlay 回调绘制 stats

---

## 已知限制

| 限制 | 说明 |
|------|------|
| 无 RT Core | 所有光线求交由 compute shader 软件实现，不用 RTX 硬件 |
| 收敛慢 | 需 4096 帧（~70 秒 @ 60fps）才能达到无噪声画面 |
| 相机移动重置 | 任何相机移动都从头累积 |
| 与管线分离 | 位于 Template3DApp，不通过 IPipeline 接口管理 |
| 无 Denoise | 1 spp 不降噪直接显示，需要大量帧数积累 |

---

## 规划方向

### 1. PathTracingPipeline (近期)

将路径追踪重构为 `IPipeline` 实现，与 `ForwardPipeline` / `Pipeline2D` 平级：

```
class PathTracingPipeline : public IPipeline {
    // 接管 compute dispatch + overlay quad
    // 由 RenderSystem::SetMainPipeline 管理
    // InitPathTracingResources → Initialize()
    // RenderPathTracing → Execute()
    // OnPresentOverlay → 内置到 Pipeline 内部
};
```

好处：
- 统一 `BeginFrame/EndFrame/Present` 流程
- Pipeline 显式控制 RP 生命周期，不再依赖 `BeginFrame` 的默认行为
- 与 Forward3D 共用同一套 overlay 机制
- app 代码只需要 `SetMainPipeline<PathTracingPipeline>()`

### 2. Hybrid 渲染 (中期)

光栅化直接光照 + 路径追踪全局光照：

```
光栅化 → G-buffer (位置/法线/颜色/材质)
    ↓
路径追踪 GI 计算 → 间接光照贴图（低分辨率 + 时域累积）
    ├── 只追踪间接光（更少噪声、更快收敛）
    ├── 相机不动时渐近收敛
    └── 相机移动时用上一帧 GI（reprojection）
    ↓
最终合成：直接光照（光栅） + 间接光照（路径追踪）
```

方案对比：

| 方案 | 直接光照 | 间接光照 | 实时响应 | 画质 |
|------|----------|----------|----------|------|
| 当前纯 PT | 路径追踪 | 路径追踪 | 慢（累积帧） | 离线级 |
| 混合 | 光栅(PBR) | 路径追踪 GI | 实时 | 高 |
| RTXGI | 光栅(PBR) | 探针插值 | 实时 | 中等 |

### 3. NRD 降噪 (近期-中期)

接入 [NVIDIA Real-Time Denoiser](https://github.com/NVIDIA-RTX/NRD)：

- **原理**：输入 1-4 spp 的不完美 PT 结果 + 法线/深度/运动矢量 → 一帧去噪
- **效果**：从需要 4096 帧收敛 → 1-4 帧即得干净画面
- **管线位置**：

```
Compute Dispatch (1-4 spp)
    ↓
G-buffer (法线/深度)
    ↓
NRD Denoise Pass (ReLAX 或 ReBLUR)
    │
    ▼ 帧缓冲 ↓
合成 → Present
```

- **价值**：相机移动时不再需要从头累积，实时交互成为可能
- **许可证**：MIT 开源
- **工作项**：
  - 将 NRD 加入 `cmake/FetchThirdPartyDeps.cmake`
  - 创建 `DenoisePass`（输入：PT output + G-buffer → 输出：去噪结果）
  - 管理 Motion Vector 和时间累积状态

### 4. RTX 硬件加速 (长期)

完整 Vulkan Ray Tracing pipeline：

```
VK_KHR_acceleration_structure
VK_KHR_ray_tracing_pipeline
    ├── rgen (ray generation) — 发射主光线
    ├── rchit (closest hit) — 材质/纹理
    └── rmiss (miss) — 环境光

BLAS/TLAS 构建
Shader Binding Table (SBT)
```

- 需要引擎级别的 Ray Tracing 支持
- 与 RTXGI/RTXDI 等 SDK 联动
- 替代当前手写求交，利用 RT Core 硬件加速

### 5. 与 RTXGI 的关系

参考 `docs/RTXGI_Integration.md` — 已有的 RTXGI 设计文档：

| | RTXGI (DDGI) | 我们的纯 PT | NRD + 混合 PT |
|---|---|---|---|
| 光线来源 | 场景探针 | 每像素随机采样 | 每像素 + 时域 |
| 收敛 | 探针更新 + 插值 | Monte Carlo 累积 | NRD 时域降噪 |
| RT Core | 必须 | 不用 | 可选 |
| 适用场景 | 实时游戏 | 离线渲染 | 实时 GI |

两者是互补而非替代关系。NRD + 混合 PT 覆盖纯 PT 和 RTXGI 之间的"中等收敛时间 + 实时交互"区间。

---

## 参考链接

- [NRD GitHub](https://github.com/NVIDIA-RTX/NRD) — MIT 开源降噪器
- [RTXGI GitHub](https://github.com/NVIDIA-RTX/RTXGI) — 探针式 GI SDK
- [RTXDI GitHub](https://github.com/NVIDIA-RTX/RTXDI) — 直接光照采样
- [Vulkan Ray Tracing](https://www.khronos.org/vulkan-ray-tracing/) — 官方规范
- [RenderingSystem.md](RenderingSystem.md) — 渲染系统架构
- [RTXGI_Integration.md](RTXGI_Integration.md) — RTXGI 集成设计
- [Roadmap.md](Roadmap.md) — 功能路线图
