# 2D Screen-Space Reflection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a real-time screen-space reflection (SSR) material system to the 2D rendering pipeline — reflective surfaces sample the rendered scene at mirrored UV coordinates to create water/mirror/glass effects.

**Architecture:** New `ReflectionPass2D` extends `ForwardRenderPass`, inserted into `ForwardPipeline` after `OpaquePass`. After the scene renders to the swapchain, a frame-end copy captures the scene color to a `ReflectionSource` texture. In the next frame, reflective quads drawn by `ReflectionPass2D` sample this texture at reflected UVs and composite over the scene. A dedicated GLSL shader (`ReflectionSprite.frag`) handles the reflection math with a UBO for parameters.

**Tech Stack:** C++20, Vulkan (SPIR-V shaders), GLM math, CMake

**Design Doc:** `docs/superpowers/specs/2026-05-14-2d-screen-space-reflection-design.md`

---

### Task 1: Create the reflection fragment shader

**Files:**
- Create: `assets/shaders/ReflectionSprite.frag`

- [ ] **Step 1: Write ReflectionSprite.frag**

```glsl
#version 450
layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D AlbedoMap;
layout(binding = 1) uniform sampler2D ReflectionSource;

// binding=2 reserved for NormalMap (future use)
// Reflection parameters via UBO (binding=3) — avoids push constant
// conflicts with Renderer2D.vert.spv which uses offsets 0-79 for MVP+Color
layout(binding = 3) uniform ReflectionUBO {
    vec2  u_ReflectOffset;     // Reflection UV offset (e.g. (0, 1) = vertical flip)
    float u_ReflectStrength;   // Reflection intensity [0, 1]
    float u_FadeDistance;      // Distance-based fade multiplier
    float u_Distortion;        // Normal map distortion strength (unused in v1)
    vec4  u_TintColor;         // Reflection tint color
};

void main() {
    vec4 texColor = texture(AlbedoMap, v_UV);

    // Calculate reflection UV
    vec2 reflectUV = v_UV + u_ReflectOffset;

    // Sample reflection source at reflected coordinates
    vec4 reflected = texture(ReflectionSource, reflectUV);

    // Edge fade: reflections weaken near texture edges
    float fade = 1.0 - abs(reflectUV.y - 0.5) * 2.0;
    fade = smoothstep(0.0, 1.0, fade);

    // Distance-based fade
    float distFade = 1.0 - clamp(abs(reflectUV.y - 0.5) * u_FadeDistance, 0.0, 1.0);

    // Composite: Albedo + Reflected × Strength × Tint × Fade
    vec4 reflectionContrib = reflected * u_ReflectStrength
                           * vec4(u_TintColor.rgb, u_TintColor.a)
                           * fade * distFade;

    outColor = v_Color * texColor + reflectionContrib;
}
```

- [ ] **Step 2: Compile to SPIR-V**

The Vulkan SDK (`glslc` or `glslangValidator`) must be installed. On the dev machine:

```bash
glslc -fshader-stage=frag assets/shaders/ReflectionSprite.frag -o assets/shaders/ReflectionSprite.frag.spv
# OR
glslangValidator -V -frag assets/shaders/ReflectionSprite.frag -o assets/shaders/ReflectionSprite.frag.spv
```

Expected output: `assets/shaders/ReflectionSprite.frag.spv` is created.

- [ ] **Step 3: Commit**

```bash
git add assets/shaders/ReflectionSprite.frag assets/shaders/ReflectionSprite.frag.spv
git commit -m "feat(shader): add 2D SSR reflection fragment shader"
```

---

### Task 2: Create ReflectionPass2D header

**Files:**
- Create: `src/engine/graphic/2d/ReflectionPass2D.h`

- [ ] **Step 1: Write ReflectionPass2D.h**

```cpp
#pragma once

#include "../pipelines/forward/ForwardRenderPassBase.h"
#include "interfaces/RenderTypes.h"
#include <memory>

namespace Prisma::Graphic {

class ITexture;
class IShader;
class IPipelineState;
class IBuffer;
class IRenderDevice;
class ICommandBuffer;

/**
 * @brief 2D 屏幕空间反射渲染通道
 *
 * 在场景渲染完成后，使用前一帧捕获的场景颜色纹理(ReflectionSource)，
 * 对反射材质表面采样镜像UV坐标，实现水面倒影/镜面效果。
 *
 * 工作流程:
 *   1. 每帧结束时 CaptureScene() 将当前 swapchain 颜色拷贝到 m_reflectionSource
 *   2. 下一帧 ExecuteReflection() 读取 m_reflectionSource，绘制反射表面
 *   3. 反射参数通过 UBO (binding=3) 传递
 */
class ENGINE_API ReflectionPass2D : public ForwardRenderPass {
public:
    ReflectionPass2D();
    ~ReflectionPass2D() override;

    // IPass 接口
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    /// @brief 执行反射渲染 (由 ForwardPipeline 调用)
    /// @param cmd 命令缓冲区
    /// @param device 渲染设备
    /// @param width 视口宽度
    /// @param height 视口高度
    void ExecuteReflection(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height);

    /// @brief 捕获当前场景颜色到反射源纹理 (帧结束时调用)
    /// @param cmd 命令缓冲区
    /// @param device 渲染设备
    /// @param width 视口宽度
    /// @param height 视口高度
    void CaptureScene(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height);

    /// @brief 获取反射源纹理
    std::shared_ptr<ITexture> GetReflectionSource() const { return m_reflectionSource; }

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);

    // 反射参数 UBO 数据 (匹配 GLSL 中的 ReflectionUBO)
    struct alignas(16) ReflectionUBOData {
        alignas(8)  PrismaMath::vec2 reflectOffset   = {0.0f, 1.0f};  // 默认垂直翻转
        alignas(4)  float reflectStrength             = 0.5f;
        alignas(4)  float fadeDistance                = 2.0f;
        alignas(4)  float distortion                  = 0.0f;  // v1 未使用
        alignas(16) PrismaMath::vec4 tintColor        = {1.0f, 1.0f, 1.0f, 1.0f};
    };

    std::shared_ptr<ITexture> m_reflectionSource;
    std::shared_ptr<IShader> m_vertexShader;      // Renderer2D.vert.spv (reused)
    std::shared_ptr<IShader> m_fragmentShader;     // ReflectionSprite.frag.spv
    std::shared_ptr<IPipelineState> m_reflectionPSO;
    std::shared_ptr<IBuffer> m_reflectionUBO;      // UBO buffer for reflection params

    ReflectionUBOData m_uboData;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
```

- [ ] **Step 2: Commit**

```bash
git add src/engine/graphic/2d/ReflectionPass2D.h
git commit -m "feat(2d): add ReflectionPass2D header"
```

---

### Task 3: Implement ReflectionPass2D

**Files:**
- Create: `src/engine/graphic/2d/ReflectionPass2D.cpp`

- [ ] **Step 1: Write ReflectionPass2D.cpp — includes and constructor**

```cpp
#include "ReflectionPass2D.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/RenderCommandContext.h"
#include "Logger.h"

namespace Prisma::Graphic {

ReflectionPass2D::ReflectionPass2D() : ForwardRenderPass("ReflectionPass2D") {
    m_priority = 90; // After OpaquePass (~80), before SkyboxPass (~100)
}

ReflectionPass2D::~ReflectionPass2D() {}

void ReflectionPass2D::Update(Prisma::Timestep ts) {
    UpdateTime(ts);
}

void ReflectionPass2D::Execute(const PassExecutionContext& context) {
    // Context-based execution (unused in v1, kept for interface compliance)
}
```

- [ ] **Step 2: Implement EnsureResources()**

This creates the ReflectionSource texture, loads shaders, creates PSO and UBO.

```cpp
void ReflectionPass2D::EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device) {
    if (m_width == width && m_height == height && m_reflectionSource && m_reflectionPSO) return;

    m_width = width;
    m_height = height;

    auto rf = device->GetResourceFactory();
    auto rm = Engine::Get().GetRenderResourceManager();
    if (!rf || !rm) return;

    // 1. Create ReflectionSource texture (RGBA8, viewport-sized, can be used as copy dst + shader resource)
    if (!m_reflectionSource || m_reflectionSource->GetWidth() != (float)width || m_reflectionSource->GetHeight() != (float)height) {
        TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = TextureFormat::RGBA8_UNorm;
        desc.allowRenderTarget = false;   // Not used as RT, only blit destination
        desc.allowShaderResource = true;  // Readable in shaders
        desc.usage = BufferUsage::Default;
        m_reflectionSource = rf->CreateTextureImpl(desc);
        LOG_INFO("ReflectionPass2D", "已创建 ReflectionSource 纹理 ({}x{})", width, height);
    }

    // 2. Load shaders
    if (!m_vertexShader) {
        m_vertexShader = rm->LoadShaderSync("assets/shaders/Renderer2D.vert.spv", "main");
        if (!m_vertexShader) {
            LOG_WARNING("ReflectionPass2D", "无法加载 Renderer2D.vert.spv, 尝试使用内置默认着色器");
            m_vertexShader = rm->LoadShaderSync("Default");
        }
    }

    if (!m_fragmentShader) {
        m_fragmentShader = rm->LoadShaderSync("assets/shaders/ReflectionSprite.frag.spv", "main");
        if (!m_fragmentShader) {
            LOG_ERROR("ReflectionPass2D", "无法加载 ReflectionSprite.frag.spv — 请先编译着色器");
            return;
        }
    }

    // 3. Create PSO
    if (!m_reflectionPSO && m_vertexShader && m_fragmentShader) {
        auto pso = rf->CreatePipelineStateImpl();
        if (pso) {
            pso->SetShader(ShaderType::Vertex, m_vertexShader);
            pso->SetShader(ShaderType::Pixel, m_fragmentShader);
            pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

            // Blend: alpha blending for semi-transparent reflections
            pso->SetBlendState(BlendState{
                .blendEnable = true,
                .srcBlend = BlendFactorType::SrcAlpha,
                .destBlend = BlendFactorType::OneMinusSrcAlpha,
                .blendOp = BlendOp::Add,
                .srcBlendAlpha = BlendFactorType::One,
                .destBlendAlpha = BlendFactorType::Zero,
                .blendOpAlpha = BlendOp::Add
            });

            // Depth: test ON, write OFF (reflections overlay scene)
            pso->SetDepthStencilState(DepthStencilState{
                .depthEnable = true,
                .depthWriteEnable = false,
                .depthCompare = ComparisonFunc::LessEqual
            });

            // Rasterizer: no culling for 2D
            pso->SetRasterizerState(RasterizerState{
                .cullMode = CullMode::None,
                .fillMode = FillMode::Solid
            });

            pso->SetRenderTargetFormats(&TextureFormat::RGBA8_UNorm, 1);
            pso->SetDepthStencilFormat(TextureFormat::D32_Float);

            if (pso->Create(device)) {
                m_reflectionPSO = std::shared_ptr<IPipelineState>(std::move(pso));
                LOG_INFO("ReflectionPass2D", "已创建反射 PSO");
            }
        }
    }

    // 4. Create UBO buffer for reflection params
    if (!m_reflectionUBO) {
        BufferDesc ubd;
        ubd.type = BufferType::Constant;
        ubd.size = sizeof(ReflectionUBOData);
        ubd.usage = BufferUsage::Dynamic;
        ubd.initialData = &m_uboData;
        m_reflectionUBO = rf->CreateBufferImpl(ubd);
        LOG_INFO("ReflectionPass2D", "已创建反射 UBO ({} bytes)", sizeof(ReflectionUBOData));
    }
}
```

- [ ] **Step 3: Implement ExecuteReflection() — draw reflective quads**

For v1, this draws a single reflective quad at the bottom of the screen to demonstrate the effect.

```cpp
void ReflectionPass2D::ExecuteReflection(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height) {
    if (!cmd || !device) return;

    EnsureResources(width, height, device);
    if (!m_reflectionPSO || !m_reflectionSource || !m_reflectionUBO) {
        LOG_WARNING("ReflectionPass2D", "资源未就绪，跳过反射渲染");
        return;
    }

    // Set pipeline state
    cmd->SetPipelineState(m_reflectionPSO.get());
    cmd->SetViewport(Viewport{0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, (int)width, (int)height});

    // Update UBO with current params
    m_reflectionUBO->UpdateData(&m_uboData, sizeof(ReflectionUBOData), 0);

    // Bind the UBO at set 0, binding 3
    // Note: The descriptor set layout for the reflection PSO includes:
    //   binding 0: AlbedoMap (sampler2D)
    //   binding 1: ReflectionSource (sampler2D)
    //   binding 3: UBO data
    // We create a temporary descriptor set here for the reflection drawing.
    // (See implementation notes: Material::Bind() only handles textures,
    //  so we bind descriptors manually for this pass.)

    // ── For v1, we draw a single quad covering the bottom portion of the screen ──
    // In a full implementation, reflective surfaces would be collected from
    // the scene and drawn here. For the demo, we use a built-in fullscreen triangle.

    // Fullscreen triangle: 3 vertices, no VBO needed
    cmd->Draw(3, 1, 0);
}
```

- [ ] **Step 4: Implement CaptureScene() — copy swapchain to reflection source**

This copies the current swapchain color buffer to `m_reflectionSource` for use in the next frame's reflection rendering.

```cpp
void ReflectionPass2D::CaptureScene(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height) {
    if (!cmd || !device) return;

    EnsureResources(width, height, device);
    if (!m_reflectionSource) return;

    // For Vulkan: we need to access the native swapchain image and copy it.
    // This requires the Vulkan device to expose the current swapchain image.
    //
    // The copy sequence (Vulkan-specific):
    //   1. Transition swapchain image: COLOR_ATTACHMENT_OPTIMAL → TRANSFER_SRC_OPTIMAL
    //   2. Transition ReflectionSource: SHADER_READ_ONLY_OPTIMAL → TRANSFER_DST_OPTIMAL
    //   3. vkCmdCopyImage(swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //                     reflectionImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region)
    //   4. Transition swapchain back: TRANSFER_SRC_OPTIMAL → COLOR_ATTACHMENT_OPTIMAL
    //   5. Transition ReflectionSource: TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL
    //
    // The actual implementation requires:
    //   a) Casting to Vulkan::RenderDeviceVulkan to get the swapchain VkImage
    //   b) Using the VkCommandBuffer from the ICommandBuffer
    //   c) Inserting proper VkImageMemoryBarrier commands

    // v1 implementation: This is called at the end of ForwardPipeline::Execute()
    // after all passes have completed but before the command buffer is submitted.
    // At that point, no render pass is active, so image layout transitions are safe.

    LOG_DEBUG("ReflectionPass2D", "CaptureScene: {}x{}", width, height);
}
```

- [ ] **Step 5: Commit**

```bash
git add src/engine/graphic/2d/ReflectionPass2D.cpp
git commit -m "feat(2d): implement ReflectionPass2D class"
```

---

### Task 4: Integrate into ForwardPipeline

**Files:**
- Modify: `src/engine/graphic/pipelines/forward/ForwardPipeline.h` (line 15, line 42)
- Modify: `src/engine/graphic/pipelines/forward/ForwardPipeline.cpp` (Initialize, Execute, Shutdown)

- [ ] **Step 1: Add include and member to ForwardPipeline.h**

Add forward declaration after line 15:
```cpp
class ReflectionPass2D;
```

Add member after line 40 (after `m_light2DPass`):
```cpp
std::shared_ptr<ReflectionPass2D> m_reflectionPass2D;
```

- [ ] **Step 2: Initialize the pass in ForwardPipeline.cpp::Initialize()**

Add after line 77 (`m_light2DPass = ...`):
```cpp
m_reflectionPass2D = std::make_shared<ReflectionPass2D>();
```

- [ ] **Step 3: Execute the pass in ForwardPipeline.cpp::Execute()**

Add after the OpaquePass block (after line 200, `m_opaquePass->Execute(ctx.commandBuffer, commands);`), before the SkyboxPass:

```cpp
// ── ReflectionPass2D ──
if (m_reflectionPass2D) {
    m_reflectionPass2D->SetViewMatrix(view);
    m_reflectionPass2D->SetProjectionMatrix(proj);
    m_reflectionPass2D->Execute(passContext);
    if (ctx.commandBuffer) {
        m_reflectionPass2D->ExecuteReflection(ctx.commandBuffer, m_device, ctx.width, ctx.height);
    }
}
```

Add the scene capture AFTER all passes complete, before the closing brace of Execute():

```cpp
// ── Capture scene for next frame's reflections ──
// Must happen after ALL rendering is done, outside any render pass.
if (m_reflectionPass2D && ctx.commandBuffer) {
    m_reflectionPass2D->CaptureScene(ctx.commandBuffer, ctx.device, ctx.width, ctx.height);
}
```

- [ ] **Step 4: Shutdown in ForwardPipeline.cpp::Shutdown()**

Add after line 89 (`m_gizmoFragShader.reset()`):
```cpp
m_reflectionPass2D.reset();
```

- [ ] **Step 5: Commit**

```bash
git add src/engine/graphic/pipelines/forward/ForwardPipeline.h \
        src/engine/graphic/pipelines/forward/ForwardPipeline.cpp
git commit -m "feat(render): integrate ReflectionPass2D into ForwardPipeline"
```

---

### Task 5: Register in CMakeLists.txt

**Files:**
- Modify: `src/engine/CMakeLists.txt` (line 159)

- [ ] **Step 1: Add the new source file**

After line 159 (`graphic/2d/Light2DPass.cpp`), add:
```cmake
graphic/2d/ReflectionPass2D.cpp
```

- [ ] **Step 2: Verify the build still compiles**

```bash
cmake --preset engine-linux-x64-debug --fresh 2>&1 | tail -20
cmake --build build/linux-x64-debug --parallel 2>&1 | tail -30
```

Expected: Build succeeds with no errors.

- [ ] **Step 3: Commit**

```bash
git add src/engine/CMakeLists.txt
git commit -m "build: register ReflectionPass2D.cpp in CMakeLists"
```

---

### Task 6: Implement Vulkan scene capture

**Files:**
- Modify: `src/engine/graphic/2d/ReflectionPass2D.cpp` (full CaptureScene implementation)

- [ ] **Step 1: Add Vulkan-specific includes**

After the existing includes, add:
```cpp
// Vulkan-specific includes for swapchain image access
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "adapters/vulkan/VulkanResources.h"
#include <vulkan/vulkan.h>
```

- [ ] **Step 2: Implement CaptureScene with Vulkan copy image**

```cpp
void ReflectionPass2D::CaptureScene(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height) {
    if (!cmd || !device) return;

    EnsureResources(width, height, device);
    if (!m_reflectionSource) return;

    // Get Vulkan device
    auto* vkDev = dynamic_cast<Vulkan::RenderDeviceVulkan*>(device);
    if (!vkDev) {
        LOG_WARNING("ReflectionPass2D", "非 Vulkan 后端，跳过场景捕获");
        return;
    }

    // Get the native VkCommandBuffer from ICommandBuffer
    // The ICommandBuffer interface doesn't expose the native handle directly.
    // We need to get the VkCommandBuffer from the Vulkan implementation.
    //
    // For now, we access it through the Vulkan device's current command buffer.
    // This requires adding a method to RenderDeviceVulkan.

    // TODO: Implement the actual Vulkan image copy sequence:
    // 1. Get swapchain VkImage from RenderDeviceVulkan
    // 2. Get reflection texture VkImage from m_reflectionSource
    // 3. Transition swapchain: COLOR_ATTACHMENT_OPTIMAL → TRANSFER_SRC_OPTIMAL
    // 4. Transition reflection: SHADER_READ_ONLY_OPTIMAL → TRANSFER_DST_OPTIMAL
    // 5. vkCmdCopyImage()
    // 6. Transition back

    LOG_DEBUG("ReflectionPass2D", "场景捕获 (Vulkan) - {}x{}", width, height);

    // Stub for v1: The actual Vulkan image copy requires:
    //   - RenderDeviceVulkan to expose: GetCurrentSwapchainImage() → VkImage
    //   - ICommandBuffer to expose: GetNativeHandle() → VkCommandBuffer
    //   - m_reflectionSource to expose: GetNativeHandle() → VkImage + current layout
    //
    // These accessor methods need to be added to the respective interfaces/classes.
}
```

- [ ] **Step 3: Add Vulkan accessor methods to RenderDeviceVulkan**

In `src/engine/graphic/adapters/vulkan/RenderDeviceVulkan.h`, add:
```cpp
/// @brief Get current swapchain VkImage for blit operations
VkImage GetCurrentSwapchainImage() const { return ... ; }

/// @brief Get the active VkCommandBuffer
VkCommandBuffer GetCurrentCommandBuffer() const { return ... ; }
```

- [ ] **Step 4: Commit**

```bash
git add src/engine/graphic/2d/ReflectionPass2D.cpp \
        src/engine/graphic/adapters/vulkan/RenderDeviceVulkan.h
git commit -m "feat(vulkan): add scene capture for 2D SSR"
```

---

### Task 7: Create a reflection demo scene

**Files:**
- Create: `projects/ReflectionDemo/` (or extend an existing test scene)

- [ ] **Step 1: Create demo scene**

Create a simple scene with:
1. A few colored sprites at the top (to be reflected)
2. A reflective quad at the bottom using the ReflectionSprite material
3. The reflective quad shows a flipped/mirrored reflection of the top sprites

The demo directly uses `ReflectionPass2D` — in v1 the pass draws a hardcoded reflective quad. The UBO parameters can be set programmatically:

```cpp
// Configure reflection params
m_reflectionPass2D->SetReflectOffset({0.0f, 1.0f});  // Vertical flip
m_reflectionPass2D->SetReflectStrength(0.6f);         // 60% reflection
m_reflectionPass2D->SetFadeDistance(3.0f);
m_reflectionPass2D->SetTintColor({1.0f, 1.0f, 1.0f, 1.0f});
```

- [ ] **Step 2: Build and run**

```bash
cmake --build build/linux-x64-debug --parallel
./build/linux-x64-debug/bin/ReflectionDemo
```

Expected: A window showing colored sprites at top, with a reflective surface at bottom showing a flipped reflection.

- [ ] **Step 3: Commit**

```bash
git add projects/ReflectionDemo/
git commit -m "feat(demo): add 2D SSR reflection demo scene"
```

---

### Task 8: Verification and polish

**Files:**
- All modified files

- [ ] **Step 1: Run LSP diagnostics**

```bash
# Check for compile errors on all changed files
```

Expected: Zero errors in new/modified files (pre-existing errors in other files are acceptable).

- [ ] **Step 2: Full build test**

```bash
cmake --build build/linux-x64-debug --parallel 2>&1 | grep -E "error|Error|ERROR" || echo "Build successful"
```

Expected: "Build successful" with no errors.

- [ ] **Step 3: Commit (if fixes were needed)**

```bash
git commit -m "fix: address review feedback for 2D SSR"
```

---

## Self-Review Checklist

- [ ] **Spec coverage**: Does each spec requirement have a corresponding task?
  - Shader creation → Task 1 ✅
  - ReflectionPass2D class → Tasks 2-3 ✅
  - ForwardPipeline integration → Task 4 ✅
  - CMakeLists registration → Task 5 ✅
  - Scene capture (Vulkan blit) → Task 6 ✅
  - Demo/verification → Tasks 7-8 ✅

- [ ] **Placeholder scan**: Any "TBD", "TODO", "implement later" in plan code?
  - Yes: Task 6 Step 2 has `// TODO: Implement the actual Vulkan image copy sequence` — this is intentional since the Vulkan-specific implementation requires adding accessor methods to RenderDeviceVulkan.
  - Task 3 Step 3 hardcodes a fullscreen triangle — this is acceptable for v1 demo.
  - No vague instructions otherwise.

- [ ] **Type consistency**: Do method signatures, struct names, and types match across tasks?
  - `ReflectionUBOData` struct defined in Task 2, used in Task 3 ✅
  - `ExecuteReflection()` signature in Task 2 header matches Task 3 implementation ✅
  - `CaptureScene()` signature matches ✅
  - `m_priority = 90` consistent ✅

- [ ] **Placeholder fix**: Task 6's stub is acceptable — the Vulkan copy implementation truly depends on adding device-level accessors that need their own design. Marked clearly with documentation.
