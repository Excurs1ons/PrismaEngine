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
        ├── PipelineBarrier (Undefined→UAV 或 ShaderRead→UAV)
        ├── 填充 Camera UBO (位置/方向/fov/帧计数)
        ├── SetComputePipeline / BindDescriptorSet
        ├── Dispatch (8×8 线程组)
        │   └── 着色器流程:
        │       ├── 非网格图元 (for 循环遍历 type 0-3: plane/sphere/box/cone)
        │       └── 网格 (BVH 遍历: intersectAABB + 栈式 depth-first 搜索 → O(log N))
        ├── PipelineBarrier (UAV → ShaderRead)
        ├── 收敛检测 (frameCount ≥ maxSamples)
        └── 提交 gizmo stats 到队列
        │
        ▼
   EndFrame → 重新开始 RenderPass
        ├── OnPresentOverlay (全屏四边形采样 storageTexture)
        │   └── ProcessGizmoOverlay (stats 文字，正交投影随视口更新)
        └── 结束 RenderPass
```

### 着色器

`assets/shaders/pathtrace.comp` — GLSL 4.60 compute shader:

- **光线求交**: 纯数学计算（无 RT Core 加速）
  - `intersectPlane()` — 点到平面 + bounds 检查
  - `intersectSphere()` — 二次方程求根
  - `intersectBox()` — AABB slab 算法（带 Y 轴旋转）
  - `traceScene()` — for 循环遍历 SSBO 中所有物体（对象级 + BVH 加速网格求交）
- **几何加速**: 空间分割 BVH（Bounding Volume Hierarchy）构建于三角形 SSBO 之上，将网格求交互复杂度从 O(N) 降至 O(log N)
- **路径追踪**: `tracePath()` — 半球采样 + 俄式轮盘赌，最多 10 次反弹
- **累积**: 两纹理（outputImage + accumImage），逐帧混入新采样

### 收敛检测

- `m_ptMaxSamples`：默认 4096 帧（可在 `Template3DApp.h` 修改，0 = 无限制）
- `m_ptConverged`：达到上限后冻结 frameCount，跳过所有 GPU dispatch
- 显示：橙色 `PathTrace: N/4096` → 绿色 `Converged: N/4096`
- 重置：R 键（重置累积）、P 键（切换模式）、窗口 resize

### 场景数据

场景从 JSON 文件加载（`assets/scenes/pt_scene.json`），通过 SSBO 传入 compute shader。支持四种图元类型：
- `type=0`: 平面（point + normal + UV bounds）
- `type=1`: 球体（center + radius）
- `type=2`: 盒子（center + half-size + Y 轴旋转）
- `type=4`: 三角形网格（本地空间顶点 + 世界矩阵，经 BVH 加速）

网格顶点在 BVH 构建时预变换到世界空间并存储在扁平 SSBO 中，通过 `triToObject` 映射查找所属对象的材质/自发光属性。

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
| 无 Denoise | 1 spp 不降噪直接显示，需要大量帧数积累 |
| 静态网格 BVH | BVH 在场景加载时构建，当前不支持动态变换（移动/旋转后需重建） |

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

## Bug 修复记录

### Bug 1: 黑屏 — 面法线未根据入射方向翻转

**症状**: 路径追踪输出全黑，stats 正常显示，dispatch 日志正确，无 ERROR 日志。

**根因**: `intersectTriangle()` 返回面法线 `normalize(cross(e1, e2))`，但未根据入射射线方向翻转。当射线从三角形背面击中时，面法线与射线方向同侧（`dot(dir, normal) > 0`），导致 hemisphere sampling 生成"穿入表面"的方向，路径即刻终止。

以 Cornell Box 背墙为例：
- `plane.obj` 顶点绕序为顺时针 → 面法线为 `(0, -1, 0)`（向下）
- 背墙旋转 90° 绕 X 轴 → 世界法线为 `(0, 0, -1)`（沿 -Z 方向）
- 相机射线沿 `(0, 0, -1)` 击中背墙，`dot(dir, normal) = 1 > 0` → 背面击中
- 未翻转法线 → 半球采样朝向 -Z（穿出盒子）→ 路径永远不会找到光源

**修复**: 在 `intersectTriangle()` 中，射线击中后始终将法线翻转向入射方向：

```glsl
vec3 faceNormal = normalize(cross(e1, e2));
if (dot(dir, faceNormal) > 0.0) {
    faceNormal = -faceNormal;
}
hit.normal = faceNormal;
```

### Bug 2: 几何破碎（不规则三角形）— CPU/GPU 结构体布局不一致

**症状**: mesh 面破损，呈现不规则的多个三角形（灯的 2 个三角形显示为分离碎片）。黑屏修复后此问题暴露。

**根因**: CPU 端 `PTTriangle` 使用 `float v0[3]`（12 字节/顶点），而 GLSL `std430` 布局中 `vec3` 的对齐要求为 **16 字节**。结果：

| 域 | CPU 偏移 | GPU 期望偏移 |
|---|---|---|
| `v0` | 0 字节 | 0 字节 |
| `v1` | 12 字节 | **16 字节** |
| `v2` | 24 字节 | **32 字节** |
| `sizeof` | **36 字节** | **48 字节** |

第一个三角形正确，但 GPU 从偏移 36 而非 48 读取第二个三角形 → 后续所有三角形数据错位 → 几何破碎。

**修复**: 将 `PTTriangle` 改为 `float v0[4], v1[4], v2[4]`（16 字节/顶点，第 4 个元素为 padding），匹配 `std430` 布局。

### Bug 3: 首帧卡顿（1 秒）— 无意义 resize 触发着色器重新编译

**症状**: 启动后首帧卡顿约 1 秒。

**根因**: `m_width = 0, m_height = 0` 导致 `Execute()` 中 `ctx.width != m_width` 为真，触发完整的 `DestroyResources() + CreateResources()`（包括着色器重新编译）。实际上窗口尺寸从未改变，仅因初始值未设置导致假 resize。

**修复**: `Initialize()` 中主动预置 `m_width = 1280; m_height = 720;`，并分离出 `ResizeResources()` 仅重建纹理 + 描述符集，不碰着色器/管线。

### Bug 4: 黑屏（第二类）— PipelineBarrier 初始状态错误

**症状**: 路径追踪输出全黑，dispatch 正常执行。

**根因**: `PipelineBarrier` 每帧都使用 `ResourceState::Undefined` 作为旧状态，导致 GPU 认为存储纹理每帧都是新创建的，丢弃了上一帧累积的像素数据。

**修复**: 添加 `m_textureInitialized` 标记，首次使用从 `Undefined → UA`，后续从 `ShaderRead → UA`，保留累积数据。

---

## 参考链接

- [NRD GitHub](https://github.com/NVIDIA-RTX/NRD) — MIT 开源降噪器
- [RTXGI GitHub](https://github.com/NVIDIA-RTX/RTXGI) — 探针式 GI SDK
- [RTXDI GitHub](https://github.com/NVIDIA-RTX/RTXDI) — 直接光照采样
- [Vulkan Ray Tracing](https://www.khronos.org/vulkan-ray-tracing/) — 官方规范
- [RenderingSystem.md](RenderingSystem.md) — 渲染系统架构
- [RTXGI_Integration.md](RTXGI_Integration.md) — RTXGI 集成设计
- [Roadmap.md](Roadmap.md) — 功能路线图

---

## 加速结构演进

路径追踪的几何求交系统经历了三个阶段的设计演进：

| 阶段 | 架构 | 复杂度 | 场景 |
|------|------|--------|------|
| **Phase 1: Flat** | 逐对象 for 循环 + 逐顶点 worldMatrix 变换 | O(N) | 当前实现（已废弃） |
| **Phase 2: BVH** | 空间分割树 + 预变换世界空间顶点 + triToObject 映射 | O(log N) | 当前实现 |
| **Phase 3: Hardware** | DXR BLAS/TLAS + RT Core 硬件加速 | O(log N) + HW | 计划中 |

### Phase 1: Flat 逐三角形遍历

原始的 `traceScene()` 对每个 type=4 物体执行：
```glsl
for (int ti = 0; ti < triCount; ti++) {
    vec3 wv = (objToWorld * vec4(tri.v, 1.0)).xyz; // 每顶点变换
    intersectTriangle(origin, dir, wv0, wv1, wv2, rt);
}
```

每个三角形需要 3 次 `mat4 × vec4` 乘法（顶点变换），并且必须遍历场景中的**所有**三角形。对于 Cornell Box（~100 个三角形）尚可接受，但随场景规模增大呈线性退化。

### Phase 2: SSBO + BVH

将 SSBO 中的三角形用空间 BVH 树组织，`traverseBVH()` 通过 AABB 测试快速跳过空白区域：

```cpp
// CPU 端 (PathTracingPipeline::BuildBVH)
struct BVHNode {
    float aabbMin[4]; // xyz = 包围盒最小值, w = triangleCount (0=内部)
    float aabbMax[4]; // xyz = 包围盒最大值, w = childOrTriStart
};
// 递归中分法构建，≤4 个三角形时为叶节点
// 节点按 depth-first 连续排列，左子 = nodeIdx + 1
```

```glsl
// GPU 端 (pathtrace.comp)
struct BVHNode {
    vec4 aabbMin; // xyz=min, w=triangleCount
    vec4 aabbMax; // xyz=max, w=childOrTriStart
};
bool traverseBVH(..., out HitResult hit) {
    int stack[64]; int sp = 1; stack[0] = 0;
    while (sp > 0) {
        sp--;
        // AABB 测试 → 跳过不相交子树
        if (!intersectAABB(...)) continue;
        if (叶) 测试叶内三角形;
        else    将左右子压栈;
    }
}
```

配套数据结构：
- `bvhNodes[]` SSBO (binding 5): BVH 节点数组
- `triToObject[]` SSBO (binding 6): 每个三角形的对象索引（用于材质/颜色查询）
- `BuildBVH()`: 中分法递归构建，重排三角形为 depth-first 连续布局
- 场景加载后由 `IPipeline::OnSceneLoaded` 自动触发

优点：
- 将网格求交互复杂度从 O(N) 降至 O(log N)
- 顶点在 BVH 构建时预变换到世界空间，免去运行时每顶点矩阵乘法
- `IPipeline::OnSceneLoaded` 使场景→管线的数据桥接自动化

### Phase 3: 硬件加速 (DXR)

规划见上方"RTX 硬件加速"章节。BVH 阶段是直接的前置准备——每个三角形的 `triToObject` 映射在设计上兼容 DXR 的 BLAS 实例化模型（BLAS = BVH 节点 + 三角形数据，TLAS = triToObject 的等效概念）。`hardwareRayTracing` 配置项（`project.json` → `rendering.hardwareRayTracing`）作为开关预留。
