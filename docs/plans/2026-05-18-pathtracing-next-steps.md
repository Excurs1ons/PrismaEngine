# 路径追踪下一步计划

## 现状

PathTracing3D Cornell Box 路径追踪已在 SSBO 架构上跑通：
- 8 个场景对象（6 平面 + 1 球体 + 1 盒子）
- 500 帧累积，160x120 分辨率
- 在 llvmpipe 软件渲染器上 ~350ms/帧

## 路线图

### Step 1: NEE（Next Event Estimation）

**目标**: 将采样效率提高 10-100x

当前 random walk 每次弹跳随机散射，只有 ~0.1% 的概率打到小面积光源。NEE 每次弹跳直接朝光源发 shadow ray。

改动范围：
- `pathtrace.comp` 的 `tracePath()` 中，每次弹跳后朝光源采样
- 需要知道光源位置、法线、面积（SSBO 中已有）
- Shadow ray 检测遮挡

```
当前: radiance = throughput * color / pi   // 仅当随机打到光源
NEE:  radiance = throughput * color / pi * (直接光贡献)  // 每次弹跳都有
```

### Step 2: Firefly Clamping

**目标**: 消除单像素异常亮值

在 tone mapping 前 clamp 颜色值：
```glsl
color = min(color, vec3(100.0));
```

或在累积时 clamp：
```glsl
color = clamp(color, 0.0, 20.0);
```

### Step 3: Windows 硬件验证

**目标**: 在真实 GPU 上验证性能

```bash
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
bin\PathTracing3D.exe --headless --frames 200 --width 1920 --height 1080 --output pt.png
```

预期：160x120 200 帧从 70s（llvmpipe）→ <0.5s（RTX）

### Step 4: 降噪器

**目标**: 从 1-4 spp 实时重建干净图像

方案选型：
- **SVGF**（Spatiotemporal Variance-Guided Filtering）— 标准选择，质量好性能适中
- **A-SVGF**（Adaptive SVGF）— 对动态场景更好
- **BMFR**（Bilateral Moments Filtering）— 质量最高的实时降噪

### Step 5: ReSTIR / 重要性重采样

**目标**: 1 spp 等效 1000 spp

- 时空域复用邻居样本
- 需要 motion vector 通道
- 复杂但效果最好的采样优化

### Step 6: 更多几何体 & 材质

- 三角形网格 + BVH
- PBR 材质（金属/粗糙度/法线贴图）
- 次表面散射

## 优先级

```
高: NEE + Clamping （1-2 天）
中: Windows 验证  （半天）
低: 降噪器         （1 周）
远: ReSTIR         （2 周+）
```
