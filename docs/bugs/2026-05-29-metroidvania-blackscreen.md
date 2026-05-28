# MetroidvaniaDemo 黑屏 Bug 调查报告

**日期**: 2026-05-29  
**分支**: dev  
**关联提交**: 68be15a7 (及之前多轮修复)  
**状态**: 部分修复，待完成 Dynamic Rendering 迁移

---

## 现象
MetroidvaniaDemo 启动后窗口正常创建，日志无报错，但画面纯黑无任何渲染内容。

## 根因（5 个 bug 叠加）

经过 8 并发 agent 代码调查 + Vulkan Validation Layer 诊断，确认以下 5 个 bug 叠加导致黑屏：

### Bug 1 (P0): OnRender() 被 BeginGizmo/EndGizmo 包裹
**文件**: `src/engine/app/Engine.cpp:552-554`

```cpp
// Bug: 所有 DrawQuad 调用被路由到 Gizmo 队列
Graphic::Renderer2D::BeginGizmo();
m_CurrentApp->OnRender();   // → DrawQuad → GizmoFrames → GizmoCommands
Graphic::Renderer2D::EndGizmo();
```
- `Pipeline2D::Execute()` 只消费 `Renderer::GetCommandQueue()`，不消费 `GetGizmoQueue()`
- 对比 `ForwardPipeline` 在 `ForwardPipeline.cpp:189-218` 正确消费了 Gizmo 队列
- **修复**: 去掉 BeginGizmo/EndGizmo 包裹

### Bug 2 (P0): OpaquePass 覆盖 PixelPerfect 视口
**文件**: `src/engine/graphic/pipelines/forward/OpaquePass.cpp:79-87`

```cpp
// Bug: 无条件将 viewport 设为 swapchain 尺寸 1024×896
width = swapChain->GetWidth();   // 1024
height = swapChain->GetHeight(); // 896
cmd->SetViewport(0, 0, width, height);  // 覆盖 Pipeline2D 设置的 256×224
```
- Pipeline2D 离屏 RT 为 256×224，OpaquePass 覆盖后内容被裁剪到左下角 1/4
- **修复**: 删除 OpaquePass 中的 viewport/scissor 硬编码，信任调用方

### Bug 3 (P0): LitSprite.frag Descriptor Layout 与 Material 不匹配
**文件**: `assets/shaders/LitSprite.frag` ↔ `src/engine/graphic/Material.cpp:325-395`

| 绑定 | LitSprite.frag (旧) | Material PBR Layout | 兼容? |
|------|-------------------|-------------------|-------|
| Set 0, Binding 0 | sampler2D AlbedoMap | UBO MaterialData | ❌ 类型不匹配 |
| Set 0, Binding 1 | sampler2D LightMap | sampler2D AlbedoMap | ⚠️ 类型对，命名错 |
| Binding 2-5 | (无) | sampler2D × 4 | ❌ 缺失 → Layout 不等价 |

- `VkDescriptorSetLayout` 必须 "identically defined" 才能被 `vkCmdBindDescriptorSets` 接受
- Material 的 6-binding PBR Layout ≠ LitSprite SPIR-V 反射的 2-binding Layout → **Vulkan 静默拒绝绑定**（validation OFF）
- **修复**: LitSprite.frag 添加 MaterialData UBO(Binding 0) + 补齐 bindings 2-5（即使未使用），SPIR-V 重新编译

### Bug 4 (P0): 配置文件搜索路径缺失 → renderMode 错误
**文件**: `src/engine/app/Engine.cpp:164-171`

```cpp
// Bug: 搜索路径不含 projects/<name>/assets/
projPaths = {
    projName + ".jsonc",         // CWD/
    projName + ".json",
    "assets/" + projName + ".json",  // CWD/assets/
    // ... 没有 "projects/<name>/assets/<name>.json" 路径
};
```
- `MetroidvaniaDemo.json` 实际位于 `projects/MetroidvaniaDemo/assets/`
- 文件未找到 → `renderMode` 保持默认 `Mode3D_Forward` → 创建 `ForwardPipeline` 而非 `Pipeline2D`
- **修复**: 搜索路径添加 `"projects/" + projName + "/assets/" + projName + ".json"`

### Bug 5 (P1, 待修复): PixelPerfect 离屏 RP 与 PSO 的 Swapchain RP 不兼容
**文件**: `src/engine/graphic/2d/Pipeline2D.cpp` ↔ `OpaquePass.cpp`

```
OpaquePass PSO 创建时绑定: Swapchain RP (VK_FORMAT_B8G8R8A8_UNORM, 1 subpass, 1 dep)
运行时激活:                Offscreen RP  (VK_FORMAT_R8G8B8A8_UNORM, 1 subpass, 2 dep)
→ VUID-vkCmdDrawIndexed-renderPass-02684
→ VUID-vkCmdDrawIndexed-None-08600 (descriptor set never bound)
```
- Vulkan 传统 RP 模式下，Pipeline 与 RenderPass 严格耦合
- 即使 patch BGRA8 格式也无法完全兼容（dependencyCount 不同）
- **临时绕过**: Phase 2 仅在 PixelPerfect 模式执行 swapchain RP，非 PP 模式跳过清屏
- **永久修复**: 启用 `VK_KHR_dynamic_rendering`（Vulkan 1.3 核心特性），Pipeline 创建时不绑 RP

---

## Vulkan Validation Layer 输出摘要

```
VUID-vkCmdDrawIndexed-renderPass-02684: pAttachments[0].format
  (VK_FORMAT_R8G8B8A8_UNORM) != (VK_FORMAT_B8G8R8A8_UNORM)

VUID-vkCmdDrawIndexed-None-08600: descriptor was never bound → 
  pipeline layouts not compatible

VUID-vkCmdDraw-None-04007: no vertex buffer bound for binding 0

VUID-vkCmdDraw-None-08114: descriptor [Set 0, Binding 1] never updated

VUID-VkImageViewCreateInfo-usage-08931: BGRA8 color attachment not supported on this GPU
```

---

## 修复文件清单

| 文件 | 修改 | 关联 Bug |
|------|------|---------|
| `src/engine/app/Engine.cpp` | 去掉 BeginGizmo/EndGizmo；添加 projects/ 搜索路径 | #1, #4 |
| `src/engine/graphic/pipelines/forward/OpaquePass.cpp` | 删除 viewport/scissor 硬编码 | #2 |
| `assets/shaders/LitSprite.frag` | 添加 UBO Binding 0 + bindings 2-5 补齐 PBR layout | #3 |
| `src/engine/graphic/2d/Pipeline2D.cpp` | Phase 2 仅在 PixelPerfect 模式执行 swapchain RP | #5 (绕过) |

---

## 待完成

1. **启用 VK_KHR_dynamic_rendering** — 彻底消除 Pipeline-RenderPass 耦合
   - 修改 `VulkanPipelineState::Create()`: 设置 `VkPipelineRenderingCreateInfo` in pNext chain
   - 修改 `VulkanCommandBuffer`: `vkCmdBeginRenderPass` → `vkCmdBeginRendering`
   - 影响范围：全部 Vulkan PSO 创建 + 所有 RenderPass 管理代码

2. **Renderer2D LightMap 绑定修复** — `Material::UpdateDescriptorSetPBR()` 中 Binding 2 查 "NormalMap" 而非 "LightMap"
   - 影响：2D 光照系统完全失效（fallback 为白色纹理）

3. **相机初始位置修复** — CameraMinY=112 使玩家 (Y=400) 永远不可见
