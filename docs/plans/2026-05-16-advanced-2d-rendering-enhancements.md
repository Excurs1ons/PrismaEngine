# 高级 2D 渲染增强设计方案 (2026-05-16)

## 概述

本方案旨在进一步完善 PrismaEngine 的 2D 渲染器（Renderer2D），解决目前 `ReflectionPass2D` 的性能瓶颈，并引入现代 2D 游戏所需的高级视觉特性。

## 1. Vulkan 加速场景捕获 (优化 ReflectionPass2D)

### 现状问题
当前 `ReflectionPass2D::CaptureScene` 使用 `ITexture::CopyFrom`，在 Vulkan 后端会触发 CPU 回读（Download -> CPU Copy -> Upload），这在 1080p 分辨率下会导致严重的掉帧（每帧延迟约 20-50ms）。

### 改进方案：GPU-side vkCmdCopyImage
利用 Vulkan 的原生命令在 GPU 内部完成从交换链图像到反射源纹理的拷贝，无需 CPU 介入。

**核心步骤：**
1. **图像布局转换 (Layout Transition)**：
   - 将 Swapchain Image 从 `COLOR_ATTACHMENT_OPTIMAL` 转换为 `TRANSFER_SRC_OPTIMAL`。
   - 将 Reflection Texture 从 `SHADER_READ_ONLY_OPTIMAL` 转换为 `TRANSFER_DST_OPTIMAL`。
2. **执行拷贝 (Copy Image)**：
   - 调用 `vkCmdCopyImage` 进行位块传输（Bit-for-bit copy）。
3. **恢复布局**：
   - 将图像转换回其原始或下一次使用的布局。

**预期效果：** 拷贝延迟从 ~30ms 降低至 <0.1ms。

## 2. 2D 法线贴图与材质属性

### 方案描述
扩展 `Renderer2D` 的合批系统，支持精灵同时携带颜色贴图（Albedo）和法线贴图（Normal Map）。

**实现细节：**
- **Shader 扩展**：更新 `Renderer2D` 专用的着色器，在计算 2D 光照时采样法线贴图，实现具有深度感的动态光照。
- **合批系统升级**：通过多纹理绑定（Descriptor Indexing / Bindless）在一次 Draw Call 中同时处理大量带有不同法线贴图的精灵。
- **材质数据**：支持设置精灵的粗糙度（Roughness）和反射率（Specular），使光照交互更真实。

## 3. 基于 SDF 的高性能 2D 阴影

### 方案描述
目前的 2D 光照缺乏阴影遮挡。引入基于有向距离场（Signed Distance Field, SDF）的阴影方案。

**技术路线：**
1. **遮挡物生成**：将场景中的静态遮挡物（如墙壁）渲染到一张低分辨率的遮挡纹理中。
2. **JFA 变换**：通过 Jump Flood Algorithm (JFA) 在 GPU 上快速生成 SDF 纹理。
3. **光照采样**：在 `Light2DPass` 中，光源根据 SDF 纹理进行步进，实现完美的软硬阴影效果。

## 4. 2D 专用后处理管线

### 方案描述
建立一套专门针对 2D 艺术风格的后处理效果堆栈。

**包含效果：**
- **CRT 模拟器**：扫描线、色差（Chromatic Aberration）、屏幕弯曲。
- **2D Bloom**：提取精灵的高亮区域进行模糊合成，营造发光感。
- **热浪/水波纹 (Distortion)**：利用位移贴图实现背景扭曲。
- **全屏故障效果 (Glitch)**：支持位移偏移和颜色分离。

## 5. 多层级瓦片地图 (Tilemap) 渲染器

### 方案描述
针对大规模 Tilemap 进行专项优化，不再将每个 Tile 视为独立节点。

**核心设计：**
- **Chunks 系统**：将 Tilemap 分为 16x16 或 32x32 的 Chunk，每个 Chunk 构建一次静态 VBO。
- **自动裁剪**：仅提交当前相机视野内的 Chunk。
- **层级合并**：将多个背景层在渲染前合并到离屏缓存中，大幅减少 Draw Call。

---

## 路线图 (Roadmap)

| 阶段 | 功能 | 优先级 | 状态 |
|------|------|--------|------|
| **阶段 1** | Vulkan 硬件加速 CaptureScene | 🔴 极高 | 待实现 |
| **阶段 2** | 2D 法线贴图支持 (Renderer2D) | 🟡 中 | 待实现 |
| **阶段 3** | SDF 2D 阴影系统 | 🟡 中 | 待实现 |
| **阶段 4** | 2D 后处理 Feature 集合 | 🟢 低 | 待实现 |

---

*注：本方案当前仅作为设计文档，不直接接入 `ForwardPipeline` 运行循环，待架构验证完成后逐步实施。*
