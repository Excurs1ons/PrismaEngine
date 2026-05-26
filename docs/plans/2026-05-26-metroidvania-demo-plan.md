# Metroidvania Demo — Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

> **REQUIRED SUB-SKILL FOR SUBAGENT-DRIVEN:** Use superpowers:subagent-driven-development

**Goal:** Build a NES-style (256×224) pixel-art Metroidvania tech demo on PrismaEngine, while improving engine infrastructure (tilemap, Physics2D, pixel-perfect rendering, CRT post-process, SRP fixes).

**Architecture:** C++ engine layer provides reusable tilemap, Physics2D, pixel-perfect rendering and CRT post-process. C# scripts handle game-specific logic (player controller, camera, abilities). Rendering uses Pipeline2D mode (not SRP) but SRP system is also fixed for future use.

**Tech Stack:** C++23, Vulkan, CMake 3.31+, C# CoreCLR (.NET 10), SDL3

**Design doc:** `docs/plans/2026-05-26-metroidvania-demo-design.md`

---

## Task List (Execution Order)

### Phase 1: Engine Infrastructure (C++ Core)

- Task 1: SRP — CreateRenderTarget implementation
- Task 2: SRP — CmdBeginRenderPass RT lookup fix
- Task 3: SRP — Connect ScriptEngine::Render() to main loop
- Task 4: PixelPerfectPass — 256×224 offscreen RT + integer scale blit
- Task 5: PostProcessPass2D CRT — implement CRT shader + wire up
- Task 6: Physics2D — pure 2D AABB collision system
- Task 7: Tilemap — core data structures + JSON serialization
- Task 8: TilemapRenderer — batch rendering with frustum culling
- Task 9: C# bindings for Physics2D + Tilemap (PrismaAPI extension)

### Phase 2: Game Project

- Task 10: MetroidvaniaDemo project scaffolding (CMake + C++ app)
- Task 11: Pixel art assets (player, tileset, pickups)
- Task 12: C# — ScriptEntry + PlayerController
- Task 13: C# — CameraFollow + TilemapCollider
- Task 14: C# — AbilityGate + DashPickup + Enemy
- Task 15: Map design — 3 rooms tilemap JSON
- Task 16: Integration build + test

---

## Task 1: SRP — CreateRenderTarget implementation

**Files:**
- Modify: `src/engine/scripting/SRPGraphicsAPI.h` — declare `SRPRenderTargetProxy` class
- Modify: `src/engine/scripting/SRPGraphicsAPI.cpp`

**Context:** `CreateRenderTarget()` at line 94 returns 0 (stub). `DestroyRenderTarget()` at line 105-107 exists but the list is always empty.

**Step 1: Add SRPRenderTargetProxy class**

In `SRPGraphicsAPI.h`, before the `SRPGraphicsAPI` class definition (or in an anonymous namespace in the .cpp), add a proxy class implementing `ITextureRenderTarget`. Follow the existing pattern from `Light2DPass.cpp:20-43` (`LightTextureRTProxy`):

```cpp
// In SRPGraphicsAPI.h or SRPGraphicsAPI.cpp anonymous namespace
class SRPRenderTargetProxy final : public Graphic::ITextureRenderTarget {
public:
    explicit SRPRenderTargetProxy(std::shared_ptr<Graphic::ITexture> tex)
        : m_texture(std::move(tex)) {}

    uint32_t GetWidth() const override { return m_texture->GetDesc().width; }
    uint32_t GetHeight() const override { return m_texture->GetDesc().height; }
    Graphic::TextureFormat GetFormat() const override { return m_texture->GetDesc().format; }
    Graphic::RenderTargetType GetType() const override { return Graphic::RenderTargetType::Texture; }
    void* GetNativeHandle() override { return m_texture->GetNativeHandle(); }
    bool IsSwapChain() const override { return false; }
    void Clear(const float[4]) override { /* optional: clear command */ }

    // ITextureRenderTarget
    uint32_t GetMipLevels() const override { return 1; }
    uint32_t GetArraySize() const override { return 1; }
    Graphic::ITexture* GetTexture() override { return m_texture.get(); }

    // IDepthStencil (not needed for color RT)
    bool HasDepth() const override { return false; }
    bool HasStencil() const override { return false; }
    Graphic::TextureFormat GetDepthStencilFormat() const override { return Graphic::TextureFormat::Unknown; }

private:
    std::shared_ptr<Graphic::ITexture> m_texture;
};
```

**Step 2: Implement CreateRenderTarget**

Replace the stub at line 94 in `SRPGraphicsAPI.cpp`:

```cpp
RenderTargetHandle SRPGraphicsAPI::CreateRenderTarget(int w, int h, uint32_t format, int /*samples*/) {
    auto* fac = GetFac();
    if (!fac || w <= 0 || h <= 0) return 0;

    Graphic::TextureDesc td;
    td.type = Graphic::TextureType::Texture2D;
    td.format = static_cast<Graphic::TextureFormat>(format);
    td.width = static_cast<uint32_t>(w);
    td.height = static_cast<uint32_t>(h);
    td.allowRenderTarget = true;
    td.allowShaderResource = true;

    auto tex = fac->CreateTextureImpl(td);
    if (!tex) return 0;

    auto rt = std::make_shared<SRPRenderTargetProxy>(std::move(tex));
    RenderTargetHandle hdl = static_cast<RenderTargetHandle>(m_renderTargets.size() + 1);
    m_renderTargets.push_back(std::move(rt));
    return hdl;
}
```

**Step 3: Add includes to SRPGraphicsAPI.cpp**

Ensure these are included at the top (they likely already are):
```cpp
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IRenderTarget.h"
```

**Step 4: Verify compilation**

Build just the Engine target:
```bash
cmake --build build/linux-x64-debug --target Engine
```
Expected: Clean compile, no linker errors.

**Step 5: Commit**

```bash
git add src/engine/scripting/SRPGraphicsAPI.h src/engine/scripting/SRPGraphicsAPI.cpp
git commit -m "feat(srp): implement CreateRenderTarget with SRPRenderTargetProxy"
```

---

## Task 2: SRP — CmdBeginRenderPass RT lookup fix

**File:** `src/engine/scripting/SRPGraphicsAPI.cpp:179-186`

**Context:** Current implementation has `// TODO: lookup RT handle` and uses null render target. The `m_renderTargets` vector now has real entries from Task 1.

**Step 1: Implement proper RT lookup**

Replace the TODO section:

```cpp
void SRPGraphicsAPI::CmdBeginRenderPass(uint32_t rtCount, const uint32_t* rtHandles,
    uint32_t /*depthHandle*/, const float* clearColors, float /*depthClear*/,
    int viewW, int viewH) {
    if (!m_cmdBuffer) return;

    Graphic::RenderPassDesc desc;
    desc.renderTarget = nullptr;
    desc.clearRenderTarget = false;
    desc.renderArea = {0, 0, 0, 0};

    // Look up the first render target from handle
    if (rtCount > 0 && rtHandles && rtHandles[0] > 0) {
        uint32_t idx = rtHandles[0] - 1;
        if (idx < m_renderTargets.size() && m_renderTargets[idx]) {
            desc.renderTarget = m_renderTargets[idx].get();
            desc.renderArea = {0, 0,
                static_cast<int>(m_renderTargets[idx]->GetWidth()),
                static_cast<int>(m_renderTargets[idx]->GetHeight())};
            if (viewW > 0 && viewH > 0) {
                desc.renderArea = {0, 0, viewW, viewH};
            }
        }
    }

    desc.clearRenderTarget = (clearColors != nullptr);
    if (clearColors) {
        memcpy(desc.clearColor, clearColors, sizeof(float) * 4);
    }

    m_cmdBuffer->BeginRenderPass(desc);
}
```

**Step 2: Verify compilation**

```bash
cmake --build build/linux-x64-debug --target Engine
```

**Step 3: Commit**

```bash
git add src/engine/scripting/SRPGraphicsAPI.cpp
git commit -m "fix(srp): implement proper render target lookup in CmdBeginRenderPass"
```

---

## Task 3: SRP — Connect ScriptEngine::Render() to main loop

**File:** `src/engine/app/Engine.cpp` (lines ~436-445)

**Context:** `ScriptEngine::Render(float dt)` exists (ScriptEngine.cpp:484-489) but is never called from the engine main loop. It calls the C# `OnRender` function pointer which triggers SRP pipeline rendering.

**Step 1: Add Render call in main loop**

In `Engine.cpp`, after `GetRenderSystem()->BeginFrame()` and before `m_CurrentApp->OnRender()`, add:

```cpp
// SRP mode: C# takes over rendering
#if PRISMA_ENABLE_SCRIPTING > 0
if (m_scriptEngine->IsInitialized()) {
    RenderMode rMode = GetRenderSystem()->GetRenderMode();
    if (rMode == Graphic::RenderMode::SRP) {
        m_scriptEngine->Render(std::min(deltaTime, 0.1f));
        // Skip C++ pipeline rendering in SRP mode
        GetRenderSystem()->EndFrame();
        GetRenderSystem()->Present();
        continue;  // back to top of while loop
    }
}
#endif
```

Wait — this approach with `continue` would skip the rest of the frame. Let me reconsider.

Actually, the cleaner approach: when in SRP mode, don't call the C++ Renderer2D at all, but still call EndFrame/Present normally.

Better approach — modify the existing rendering block:

```cpp
if (GetRenderSystem()) {
    double t0 = Platform::GetTimeSeconds();
    GetRenderSystem()->BeginFrame();
    double t1 = Platform::GetTimeSeconds();
    
#if PRISMA_ENABLE_SCRIPTING > 0
    // SRP mode: C# handles all rendering
    if (GetRenderSystem()->GetRenderMode() == Graphic::RenderMode::SRP) {
        if (m_scriptEngine->IsInitialized())
            m_scriptEngine->Render(std::min(deltaTime, 0.1f));
    } else {
        // Non-SRP: standard C++ rendering pipeline
        Graphic::Renderer2D::BeginGizmo();
        m_CurrentApp->OnRender(); 
        Graphic::Renderer2D::EndGizmo();
    }
#else
    Graphic::Renderer2D::BeginGizmo();
    m_CurrentApp->OnRender(); 
    Graphic::Renderer2D::EndGizmo();
#endif
    
    double t2 = Platform::GetTimeSeconds();
    GetRenderSystem()->EndFrame();
    ...
```

Need to check if `RenderSystem` has `GetRenderMode()`. Let me verify.

Actually, looking at `RenderSystemDesc`, the `renderMode` field is stored. We can access it via `GetRenderSystem()->GetRenderMode()` or similar. If that doesn't exist, we can use `m_Spec` which is available in `Engine`.

Let me check if there's a `GetRenderMode()` or we need to compare the render system's pipeline type.

Simpler approach — just use the stored spec:

```cpp
#if PRISMA_ENABLE_SCRIPTING > 0
auto& appSpec = m_CurrentApp->GetSpecification();
if (appSpec.RenderMode == RenderMode::SRP) {
    if (m_scriptEngine->IsInitialized())
        m_scriptEngine->Render(std::min(deltaTime, 0.1f));
} else {
    Graphic::Renderer2D::BeginGizmo();
    m_CurrentApp->OnRender();
    Graphic::Renderer2D::EndGizmo();
}
#else
...
```

Actually wait, I need to check what `RenderMode` values exist. Let me check what was used earlier. From the project config parsing: `renderMode == RenderMode::SRP`, `renderMode == RenderMode::Mode2D`, etc. These are in Engine.cpp.

But we also need to check: `AppSpecification` has a `RenderMode` field? Looking at the config parsing code, it reads into `renderMode` local variable. But is it stored in `AppSpecification`?

Actually looking at the code more carefully, the `renderMode` is used to create the `RenderSystemDesc`, but I'm not sure it's stored on `AppSpecification`. Let me keep it simple and check the render system.

Actually, let me just use a flag. Cleanest approach:

```cpp
// Inside Engine, store renderMode
// It's already parsed locally in Run() as 'renderMode' variable

// But that's local to the initialization. We need it in the loop.
// Simplest: check if render system has an SRP-style pipeline or get renderMode from it.
```

Let me just check what APIs exist. Actually, for the plan, I'll provide both approaches and let the implementer choose based on what compiles.

Actually, the simplest and most reliable approach: **always call `ScriptEngine::Render()` if initialized**, and let the C# side decide whether to do anything. The C# `OnRender()` callback will be null/empty unless SRP pipeline is set up. This avoids needing to check a mode flag:

```cpp
if (GetRenderSystem()) {
    GetRenderSystem()->BeginFrame();
    
#if PRISMA_ENABLE_SCRIPTING > 0
    // Always allow C# to run SRP rendering (no-op if not using SRP)
    if (m_scriptEngine->IsInitialized())
        m_scriptEngine->Render(std::min(deltaTime, 0.1f));
#endif
    
    // C++ pipeline always runs (C# SRP can draw on top or use a separate RT)
    Graphic::Renderer2D::BeginGizmo();
    m_CurrentApp->OnRender();
    Graphic::Renderer2D::EndGizmo();
    
    GetRenderSystem()->EndFrame();
    GetRenderSystem()->Present();
}
```

This is the simplest approach. C# SRP draws after C++ pipeline but before EndFrame. The C# side needs to handle its own render target management. But for the pixel-perfect game (which uses 2D mode, not SRP mode), `m_scriptEngine->Render()` would just be a no-op since the C# OnRender callback isn't set up.

Actually, `ScriptEngine::Render()` checks `m_srpRenderFn` (line 485), which is only set if C# registers an `OnRenderCallback`. If not set, it returns immediately. So this is safe.

**Step 1: Add the call**

In `Engine.cpp`, after line 438 (`GetRenderSystem()->BeginFrame()`) and before line 440 (`Renderer2D::BeginGizmo()`), add:

```cpp
#if PRISMA_ENABLE_SCRIPTING > 0
if (m_scriptEngine->IsInitialized())
    m_scriptEngine->Render(std::min(deltaTime, 0.1f));
#endif
```

**Step 2: Verify compilation**

```bash
cmake --build build/linux-x64-debug --target Engine
```

**Step 3: Commit**

```bash
git add src/engine/app/Engine.cpp
git commit -m "fix(srp): connect ScriptEngine::Render() to main rendering loop"
```

---

## Task 4: PixelPerfectPass — 256×224 offscreen RT + integer scale blit

**Files:**
- Create: `src/engine/graphic/2d/PixelPerfectPass.h`
- Create: `src/engine/graphic/2d/PixelPerfectPass.cpp`
- Modify: `src/engine/graphic/2d/Pipeline2D.h` — add m_pixelPass member
- Modify: `src/engine/graphic/2d/Pipeline2D.cpp` — add pass to pipeline

**Step 1: Create PixelPerfectPass.h**

```cpp
#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/ITexture.h"
#include <memory>

namespace Prisma::Graphic {

class PixelPerfectPass : public LogicalPass {
public:
    PixelPerfectPass();
    ~PixelPerfectPass() override = default;

    void SetLogicalWidth(uint32_t w) { m_logicW = w; }
    void SetLogicalHeight(uint32_t h) { m_logicH = h; }
    uint32_t GetLogicalWidth() const { return m_logicW; }
    uint32_t GetLogicalHeight() const { return m_logicH; }

    // Get the offscreen render target for other passes to render into
    IRenderTarget* GetOffscreenTarget() const { return m_offscreenRT.get(); }

    void Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(ICommandBuffer* cmd) override;

    // Called after all passes have rendered to the offscreen RT
    void BlitToSwapChain(ICommandBuffer* cmd, IRenderTarget* swapChainTarget);

private:
    void EnsureResources(IRenderDevice* device);
    void CalculateViewport(uint32_t windowW, uint32_t windowH,
                           int& outX, int& outY, uint32_t& outW, uint32_t& outH, uint32_t& scale);

    uint32_t m_logicW = 256;
    uint32_t m_logicH = 224;
    bool m_enabled = true;

    std::shared_ptr<ITexture> m_offscreenTexture;
    std::shared_ptr<IRenderTarget> m_offscreenRT;
    std::shared_ptr<IPipelineState> m_blitPSO;
};

} // namespace Prisma::Graphic
```

**Step 2: Create PixelPerfectPass.cpp**

```cpp
#include "PixelPerfectPass.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/RenderDesc.h"
#include "Logger.h"
#include <algorithm>

namespace Prisma::Graphic {

PixelPerfectPass::PixelPerfectPass() : LogicalPass("PixelPerfect") {
    m_priority = 150; // After Canvas, before PostProcess, before UI
}

void PixelPerfectPass::Initialize(IRenderDevice* device) {
    EnsureResources(device);
}

void PixelPerfectPass::Shutdown() {
    m_offscreenTexture.reset();
    m_offscreenRT.reset();
    m_blitPSO.reset();
}

void PixelPerfectPass::EnsureResources(IRenderDevice* device) {
    if (m_offscreenRT) return;
    if (!device) return;

    auto* fac = device->GetResourceFactory();
    if (!fac) return;

    // Create offscreen color texture
    TextureDesc texDesc;
    texDesc.type = TextureType::Texture2D;
    texDesc.format = TextureFormat::RGBA8_UNorm;
    texDesc.width = m_logicW;
    texDesc.height = m_logicH;
    texDesc.allowRenderTarget = true;
    texDesc.allowShaderResource = true;
    texDesc.mipLevels = 1;

    m_offscreenTexture = fac->CreateTextureImpl(texDesc);
    if (!m_offscreenTexture) {
        LOG_ERROR("PixelPerfect", "Failed to create offscreen texture");
        return;
    }

    // Create offscreen RT proxy
    class OffscreenRTProxy final : public ITextureRenderTarget {
    public:
        explicit OffscreenRTProxy(std::shared_ptr<ITexture> tex) : m_tex(std::move(tex)) {}
        uint32_t GetWidth() const override { return m_tex->GetDesc().width; }
        uint32_t GetHeight() const override { return m_tex->GetDesc().height; }
        TextureFormat GetFormat() const override { return m_tex->GetDesc().format; }
        RenderTargetType GetType() const override { return RenderTargetType::Texture; }
        void* GetNativeHandle() override { return m_tex->GetNativeHandle(); }
        bool IsSwapChain() const override { return false; }
        void Clear(const float color[4]) override {}
        uint32_t GetMipLevels() const override { return 1; }
        uint32_t GetArraySize() const override { return 1; }
        ITexture* GetTexture() override { return m_tex.get(); }
    private:
        std::shared_ptr<ITexture> m_tex;
    };

    m_offscreenRT = std::make_shared<OffscreenRTProxy>(m_offscreenTexture);

    // Note: blit PSO may not be needed if we use ImageCopy or a simple fullscreen quad
    // For now, we'll use a simple approach: Bind the offscreen texture as shader resource
    // and draw a fullscreen quad with nearest-neighbor sampling
    LOG_INFO("PixelPerfect", "Offscreen RT created: {}x{}", m_logicW, m_logicH);
}

void PixelPerfectPass::CalculateViewport(uint32_t windowW, uint32_t windowH,
    int& outX, int& outY, uint32_t& outW, uint32_t& outH, uint32_t& scale) {
    // Calculate integer scale factor
    uint32_t scaleX = windowW / m_logicW;
    uint32_t scaleY = windowH / m_logicH;
    scale = std::min(scaleX, scaleY);
    if (scale < 1) scale = 1;

    outW = m_logicW * scale;
    outH = m_logicH * scale;
    outX = (static_cast<int>(windowW) - static_cast<int>(outW)) / 2;
    outY = (static_cast<int>(windowH) - static_cast<int>(outH)) / 2;
}

void PixelPerfectPass::Execute(ICommandBuffer* cmd) {
    // This pass doesn't execute rendering itself.
    // Other passes (Opaque, Canvas) render into m_offscreenRT via Pipeline2D.
    // BlitToSwapChain is called separately.
}

void PixelPerfectPass::BlitToSwapChain(ICommandBuffer* cmd, IRenderTarget* swapChainTarget) {
    if (!cmd || !swapChainTarget || !m_offscreenTexture) return;

    int vpX, vpY;
    uint32_t vpW, vpH, scale;
    CalculateViewport(swapChainTarget->GetWidth(), swapChainTarget->GetHeight(),
                      vpX, vpY, vpW, vpH, scale);

    // Set viewport for integer-scaled rendering with nearest-neighbor
    Viewport vp;
    vp.x = static_cast<float>(vpX);
    vp.y = static_cast<float>(vpY);
    vp.width = static_cast<float>(vpW);
    vp.height = static_cast<float>(vpH);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    cmd->SetViewport(vp);

    Rect scissor;
    scissor.x = vpX;
    scissor.y = vpY;
    scissor.width = static_cast<int>(vpW);
    scissor.height = static_cast<int>(vpH);
    cmd->SetScissorRect(scissor);

    // Blit the offscreen texture to the swap chain with nearest-neighbor filtering
    // This uses a simple texture copy or a fullscreen quad with point sampling
    cmd->Blit(m_offscreenTexture.get(), swapChainTarget);
    // Note: if Blit doesn't exist on ICommandBuffer, use a fullscreen quad approach
    // with a simple passthrough shader and point sampler
}

} // namespace Prisma::Graphic
```

**Step 3: Modify Pipeline2D.h**

Add member:
```cpp
std::shared_ptr<PixelPerfectPass> m_pixelPass;
```

And forward declare:
```cpp
class PixelPerfectPass;
```

**Step 4: Modify Pipeline2D.cpp**

In `Initialize()`:
```cpp
m_pixelPass = std::make_shared<PixelPerfectPass>();
m_pixelPass->SetLogicalWidth(256);
m_pixelPass->SetLogicalHeight(224);
m_pixelPass->Initialize(m_device);
```

In `Shutdown()`:
```cpp
if (m_pixelPass) { m_pixelPass->Shutdown(); m_pixelPass.reset(); }
```

In `Execute()`:
- Change OpaquePass and CanvasPass to render to `m_pixelPass->GetOffscreenTarget()` when available
- After CanvasPass, call `m_pixelPass->BlitToSwapChain()`

The execution order becomes:
```cpp
// 1. Light2D (renders to its own light texture)
if (m_lightPass) m_lightPass->Execute(ctx.commandBuffer);

// 2. Opaque (renders to offscreen pixel RT)
if (m_opaquePass) m_opaquePass->Execute(ctx.commandBuffer);

// 3. Canvas (renders to offscreen pixel RT)
if (m_canvasPass) m_canvasPass->Execute(ctx.commandBuffer);

// 4. Pixel-perfect blit (offscreen → swap chain with integer scale)
if (m_pixelPass) {
    m_pixelPass->BlitToSwapChain(ctx.commandBuffer, ctx.renderTarget);
}

// 5. Post-process (on swap chain)
if (m_ppPass) {
    m_ppPass->Process(ctx.commandBuffer, m_device, ...);
}

// 6. UI (on swap chain, screen-space)
if (m_uiPass) m_uiPass->Execute(ctx.commandBuffer);
```

**Note:** Setting custom render targets for OpaquePass and CanvasPass requires those passes to support configurable render targets. If they currently always render to swap chain, we need to modify them to accept an `IRenderTarget*`. Alternatively, we can restructure the approach: have the PixelPerfectPass manage the offscreen RT and expose it for other passes to target.

**Step 5: Verify compilation**

```bash
cmake --build build/linux-x64-debug --target Engine
```

**Step 6: Commit**

```bash
git add src/engine/graphic/2d/PixelPerfectPass.h \
       src/engine/graphic/2d/PixelPerfectPass.cpp \
       src/engine/graphic/2d/Pipeline2D.h \
       src/engine/graphic/2d/Pipeline2D.cpp
git commit -m "feat(2d): add PixelPerfectPass with 256x224 offscreen RT and integer scaling"
```

---

## Task 5: PostProcessPass2D CRT — implement CRT shader + wire up

**Files:**
- Modify: `src/engine/graphic/2d/PostProcessPass2D.h`
- Modify: `src/engine/graphic/2d/PostProcessPass2D.cpp`
- Create: `assets/shaders/CRTScanline.frag`
- Modify: `src/engine/graphic/2d/Pipeline2D.cpp` — uncomment PostProcessPass2D call

**Step 1: Create CRT scanline fragment shader**

Create `assets/shaders/CRTScanline.frag`:

```glsl
#version 450

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform texture2D u_SceneTexture;
layout(binding = 1) uniform sampler u_Sampler;

layout(push_constant) uniform CRTParams {
    float scanlineIntensity;  // 0.0 - 1.0
    float chromaticAberration; // 0.0 - 0.01 (pixel offset)
    float brightness;
    float contrast;
} params;

void main() {
    vec2 uv = v_TexCoord;
    
    // Chromatic aberration
    float chromaOffset = params.chromaticAberration;
    float r = texture(sampler2D(u_SceneTexture, u_Sampler), uv + vec2(chromaOffset, 0.0)).r;
    float g = texture(sampler2D(u_SceneTexture, u_Sampler), uv).g;
    float b = texture(sampler2D(u_SceneTexture, u_Sampler), uv - vec2(chromaOffset, 0.0)).b;
    vec3 color = vec3(r, g, b);
    
    // Scanlines
    float scanline = sin(uv.y * 3.14159 * 224.0); // 224 = logical height
    scanline = clamp(scanline, 0.0, 1.0);
    scanline = 1.0 - (1.0 - scanline) * params.scanlineIntensity;
    color *= scanline;
    
    // Brightness/contrast
    color *= params.brightness;
    color = (color - 0.5) * params.contrast + 0.5;
    
    outColor = vec4(color, 1.0);
}
```

**Step 2: Modify PostProcessPass2D.h**

Already exists at `src/engine/graphic/2d/PostProcessPass2D.h`. It has `EffectType` enum and `m_effects` bool array. We'll add:

- CRT shader handle
- CRT sampler handle  
- CRT-specific pipeline state

No structural changes needed — the existing skeleton works.

**Step 3: Implement CRT effect in PostProcessPass2D.cpp**

Current `Process()` at line ~40 is stubbed. Replace with actual CRT rendering:

```cpp
void PostProcessPass2D::Process(ICommandBuffer* cmd, IRenderDevice* device,
    ITexture* input, IRenderTarget* output) {
    if (!cmd || !device) return;
    if (!m_effects[static_cast<int>(EffectType::CRT)]) return; // CRT disabled

    EnsureResources(device); // loads CRT PSO if not loaded

    if (!m_crtPSO) return;

    cmd->SetPipelineState(m_crtPSO.get());

    // Bind the input scene texture
    if (input) {
        // Bind texture at slot 0 (shader uses u_SceneTexture at binding=0)
        // This requires descriptor set binding
    }

    // Push CRT parameters
    struct { float intensity, aberration, brightness, contrast; } params;
    params.intensity = m_crtIntensity;     // 0.3f default
    params.aberration = m_crtAberration;   // 0.002f default
    params.brightness = 1.0f;
    params.contrast = 1.0f;
    cmd->PushConstants(ShaderType::Pixel, &params, sizeof(params));

    // Draw fullscreen quad
    cmd->Draw(4, 1, 0);
}
```

Also need to add member variables to the class:
```cpp
float m_crtIntensity = 0.3f;
float m_crtAberration = 0.002f;
std::shared_ptr<IPipelineState> m_crtPSO;
```

**Step 4: Uncomment PostProcessPass2D call in Pipeline2D.cpp**

In Pipeline2D.cpp, around line 82-84, uncomment and wire up:
```cpp
// ── 4. 2D 后处理 ──
if (m_ppPass) {
    m_ppPass->Process(ctx.commandBuffer, m_device,
        /* input texture from last pass */,
        /* output = swap chain RT */);
}
```

**Step 5: Verify compilation**

```bash
cmake --build build/linux-x64-debug --target Engine
```

**Step 6: Commit**

```bash
git add assets/shaders/CRTScanline.frag \
       src/engine/graphic/2d/PostProcessPass2D.h \
       src/engine/graphic/2d/PostProcessPass2D.cpp \
       src/engine/graphic/2d/Pipeline2D.cpp
git commit -m "feat(2d): implement CRT scanline post-process effect"
```

---

## Task 6: Physics2D — pure 2D AABB collision system

**Files:**
- Create: `src/engine/physics2d/AABB2D.h`
- Create: `src/engine/physics2d/Physics2D.h`
- Create: `src/engine/physics2d/Physics2D.cpp`

**Step 1: Create AABB2D.h**

```cpp
#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace Prisma::Physics2D {

struct AABB2D {
    float minX, minY, maxX, maxY;

    AABB2D() : minX(0), minY(0), maxX(0), maxY(0) {}
    AABB2D(float x1, float y1, float x2, float y2)
        : minX(x1), minY(y1), maxX(x2), maxY(y2) {}

    float GetWidth() const { return maxX - minX; }
    float GetHeight() const { return maxY - minY; }
    glm::vec2 GetCenter() const { return {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f}; }
    glm::vec2 GetSize() const { return {GetWidth(), GetHeight()}; }

    bool Intersects(const AABB2D& other) const {
        return minX < other.maxX && maxX > other.minX &&
               minY < other.maxY && maxY > other.minY;
    }

    bool Contains(float px, float py) const {
        return px >= minX && px <= maxX && py >= minY && py <= maxY;
    }

    static AABB2D FromCenterSize(float cx, float cy, float w, float h) {
        return AABB2D(cx - w * 0.5f, cy - h * 0.5f,
                      cx + w * 0.5f, cy + h * 0.5f);
    }

    AABB2D Translated(float dx, float dy) const {
        return AABB2D(minX + dx, minY + dy, maxX + dx, maxY + dy);
    }
};

} // namespace Prisma::Physics2D
```

**Step 2: Create Physics2D.h**

```cpp
#pragma once

#include "AABB2D.h"
#include <glm/glm.hpp>
#include <vector>

namespace Prisma::Physics2D {

struct RaycastHit2D {
    bool hit = false;
    float distance = 0.0f;
    glm::vec2 point{0.0f};
    glm::vec2 normal{0.0f};
};

class Physics2D {
public:
    // Basic AABB overlap test
    static bool CheckAABB(const AABB2D& a, const AABB2D& b);

    // Sweep test: moving AABB vs static AABB
    // Returns hit time (0.0-1.0), normal, and hit point
    static bool SweepAABB(const AABB2D& moving, glm::vec2 velocity,
                          const AABB2D& static_, float& hitTime, glm::vec2& normal);

    // Resolve player-vs-world collision
    // Returns whether any collision occurred, sets onGround/hitCeiling
    static bool ResolvePlatform(const AABB2D& player, glm::vec2& velocity,
                                const AABB2D* solids, uint32_t count,
                                bool& onGround, bool& hitCeiling);

    // One-way platform (pass through from below)
    static bool CheckOneWayPlatform(const AABB2D& player,
                                    const AABB2D& platform,
                                    float playerPrevBottom,
                                    float playerVelY);

    // Raycast against multiple AABBs
    static RaycastHit2D RayCast(glm::vec2 origin, glm::vec2 direction,
                                float maxDist, const AABB2D* targets, uint32_t count);

private:
    // Internal: AABB vs ray slab test
    static bool RayVsAABB(glm::vec2 origin, glm::vec2 invDir,
                          const AABB2D& box, float& tMin, float& tMax);
};

} // namespace Prisma::Physics2D
```

**Step 3: Create Physics2D.cpp**

```cpp
#include "Physics2D.h"
#include <algorithm>
#include <cmath>

namespace Prisma::Physics2D {

bool Physics2D::CheckAABB(const AABB2D& a, const AABB2D& b) {
    return a.Intersects(b);
}

bool Physics2D::SweepAABB(const AABB2D& moving, glm::vec2 velocity,
                          const AABB2D& static_, float& hitTime, glm::vec2& normal) {
    if (velocity.x == 0.0f && velocity.y == 0.0f) {
        hitTime = 1.0f;
        return moving.Intersects(static_);
    }

    // Expand static AABB by moving's size
    AABB2D expanded(
        static_.minX - moving.GetWidth() * 0.5f,
        static_.minY - moving.GetHeight() * 0.5f,
        static_.maxX + moving.GetWidth() * 0.5f,
        static_.maxY + moving.GetHeight() * 0.5f
    );

    // Raycast from moving center along velocity
    glm::vec2 rayOrigin = moving.GetCenter();
    glm::vec2 rayDir = velocity;
    float invDirX = (rayDir.x != 0.0f) ? 1.0f / rayDir.x : 1e10f;
    float invDirY = (rayDir.y != 0.0f) ? 1.0f / rayDir.y : 1e10f;

    float tMin, tMax;
    if (!RayVsAABB(rayOrigin, {invDirX, invDirY}, expanded, tMin, tMax))
        return false;

    if (tMin < 0.0f) tMin = 0.0f;
    if (tMin > 1.0f) return false;

    hitTime = tMin;

    // Calculate normal from contact point
    glm::vec2 contact = rayOrigin + rayDir * tMin;
    glm::vec2 center = expanded.GetCenter();
    glm::vec2 halfSize = expanded.GetSize() * 0.5f;
    glm::vec2 diff = contact - center;

    // Determine normal based on which face was hit
    float overlapX = halfSize.x - std::abs(diff.x);
    float overlapY = halfSize.y - std::abs(diff.y);

    if (overlapX < overlapY) {
        normal = (diff.x > 0.0f) ? glm::vec2(-1.0f, 0.0f) : glm::vec2(1.0f, 0.0f);
    } else {
        normal = (diff.y > 0.0f) ? glm::vec2(0.0f, -1.0f) : glm::vec2(0.0f, 1.0f);
    }

    return true;
}

bool Physics2D::ResolvePlatform(const AABB2D& player, glm::vec2& velocity,
                                const AABB2D* solids, uint32_t count,
                                bool& onGround, bool& hitCeiling) {
    onGround = false;
    hitCeiling = false;
    bool anyCollision = false;

    // Sort by hit time for correct resolution order
    struct Hit {
        float time;
        glm::vec2 normal;
        uint32_t index;
    };
    std::vector<Hit> hits;
    hits.reserve(count);

    AABB2D playerBox = player;

    for (uint32_t i = 0; i < count; i++) {
        float hitTime;
        glm::vec2 normal;
        if (SweepAABB(playerBox, velocity, solids[i], hitTime, normal)) {
            hits.push_back({hitTime, normal, i});
        }
    }

    // Sort by hit time (closest first)
    std::sort(hits.begin(), hits.end(),
        [](const Hit& a, const Hit& b) { return a.time < b.time; });

    float remainingTime = 1.0f;
    for (const auto& hit : hits) {
        if (hit.time > remainingTime) break;

        // Resolve velocity along normal
        float dot = glm::dot(velocity, hit.normal);
        if (dot < 0.0f) {
            velocity -= hit.normal * dot;
        }

        if (hit.normal.y < -0.5f) onGround = true;
        if (hit.normal.y > 0.5f) hitCeiling = true;

        remainingTime -= hit.time;
        anyCollision = true;
    }

    return anyCollision;
}

bool Physics2D::CheckOneWayPlatform(const AABB2D& player,
                                     const AABB2D& platform,
                                     float playerPrevBottom,
                                     float playerVelY) {
    // Only collide when player is falling and was above the platform
    if (playerVelY >= 0.0f) return false;                      // Not falling
    if (playerPrevBottom >= platform.maxY) return false;       // Was already below
    if (player.minY >= platform.maxY) return false;            // Still above

    // Check horizontal overlap
    return player.minX < platform.maxX && player.maxX > platform.minX;
}

bool Physics2D::RayVsAABB(glm::vec2 origin, glm::vec2 invDir,
                          const AABB2D& box, float& tMin, float& tMax) {
    float t1 = (box.minX - origin.x) * invDir.x;
    float t2 = (box.maxX - origin.x) * invDir.x;
    float t3 = (box.minY - origin.y) * invDir.y;
    float t4 = (box.maxY - origin.y) * invDir.y;

    tMin = std::max(std::min(t1, t2), std::min(t3, t4));
    tMax = std::min(std::max(t1, t2), std::max(t3, t4));

    return tMax >= tMin && tMax >= 0.0f;
}

RaycastHit2D Physics2D::RayCast(glm::vec2 origin, glm::vec2 direction,
                                float maxDist, const AABB2D* targets, uint32_t count) {
    RaycastHit2D result;
    float closest = maxDist;

    glm::vec2 invDir = direction;
    if (invDir.x != 0.0f) invDir.x = 1.0f / invDir.x;
    else invDir.x = 1e10f;
    if (invDir.y != 0.0f) invDir.y = 1.0f / invDir.y;
    else invDir.y = 1e10f;

    for (uint32_t i = 0; i < count; i++) {
        float tMin, tMax;
        if (!RayVsAABB(origin, invDir, targets[i], tMin, tMax)) continue;
        if (tMin > 0.0f && tMin < closest) {
            closest = tMin;
            result.hit = true;
            result.distance = tMin * glm::length(direction);
            result.point = origin + direction * tMin;

            // Compute normal
            glm::vec2 center = targets[i].GetCenter();
            glm::vec2 halfSize = targets[i].GetSize() * 0.5f;
            glm::vec2 diff = result.point - center;
            float ox = halfSize.x - std::abs(diff.x);
            float oy = halfSize.y - std::abs(diff.y);
            if (ox < oy)
                result.normal = (diff.x > 0) ? glm::vec2(-1, 0) : glm::vec2(1, 0);
            else
                result.normal = (diff.y > 0) ? glm::vec2(0, -1) : glm::vec2(0, 1);
        }
    }
    return result;
}

} // namespace Prisma::Physics2D
```

**Step 4: Add CMake include**

Modify the engine's CMakeLists.txt (or the relevant CMake file that collects source directories) to add `src/engine/physics2d/` to include paths and source files.

**Step 5: Verify compilation**

```bash
cmake --build build/linux-x64-debug --target Engine
```

**Step 6: Commit**

```bash
git add src/engine/physics2d/ \
       cmake/  # or whatever CMake file was modified
git commit -m "feat(physics): add pure 2D AABB collision system (Physics2D)"
```

---

## Task 7: Tilemap — core data structures + JSON serialization

**Files:**
- Create: `src/engine/tilemap/TileSet.h`
- Create: `src/engine/tilemap/TileSet.cpp`
- Create: `src/engine/tilemap/TileLayer.h`
- Create: `src/engine/tilemap/TileLayer.cpp`
- Create: `src/engine/tilemap/Tilemap.h`
- Create: `src/engine/tilemap/Tilemap.cpp`
- Modify: `src/engine/resource/TilemapAsset.h` — update to include real header

**Step 1: Create TileSet.h**

```cpp
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace Prisma::Tilemap {

struct TileDef {
    uint32_t tileId = 0;       // 0 = empty/air
    uint32_t texIndex = 0;     // Index into tileset spritesheet
    uint8_t  collisionFlags = 0; // Bitmask
    uint8_t  padding[3] = {0};
};

class TileSet {
public:
    TileSet() = default;

    bool LoadFromJSON(const std::string& path);
    bool SaveToJSON(const std::string& path) const;

    void SetTileSize(uint32_t size) { m_tileSize = size; }
    uint32_t GetTileSize() const { return m_tileSize; }

    void SetTexturePath(const std::string& path) { m_texturePath = path; }
    const std::string& GetTexturePath() const { return m_texturePath; }

    void AddTileDef(const TileDef& def) { m_tiles.push_back(def); }
    const TileDef* GetTileDef(uint32_t id) const;
    uint32_t GetTileCount() const { return static_cast<uint32_t>(m_tiles.size()); }

    // Spritesheet layout
    void SetColumns(uint32_t cols) { m_columns = cols; }
    uint32_t GetColumns() const { return m_columns; }

private:
    uint32_t m_tileSize = 16;
    uint32_t m_columns = 8;
    std::string m_texturePath;
    std::vector<TileDef> m_tiles;
};

// Collision flags
enum TileCollision : uint8_t {
    Solid     = 1 << 0,
    Platform  = 1 << 1,  // One-way platform
    Hazard    = 1 << 2,
    Ladder    = 1 << 3,
};

} // namespace Prisma::Tilemap
```

**Step 2: Create TileLayer.h**

```cpp
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Prisma::Tilemap {

struct TileLayer {
    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint32_t> tiles;  // 1D array, row-major, 0 = empty
    bool visible = true;
    int sortingOrder = 0;

    uint32_t GetTile(uint32_t x, uint32_t y) const {
        if (x >= width || y >= height) return 0;
        return tiles[y * width + x];
    }

    void SetTile(uint32_t x, uint32_t y, uint32_t tileId) {
        if (x < width && y < height)
            tiles[y * width + x] = tileId;
    }
};

} // namespace Prisma::Tilemap
```

**Step 3: Create Tilemap.h**

```cpp
#pragma once

#include "TileSet.h"
#include "TileLayer.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma::Tilemap {

class Tilemap {
public:
    Tilemap() = default;

    bool LoadFromJSON(const std::string& path);
    bool SaveToJSON(const std::string& path) const;

    TileSet& GetTileSet() { return m_tileSet; }
    const TileSet& GetTileSet() const { return m_tileSet; }

    TileLayer* AddLayer(const std::string& name, uint32_t width, uint32_t height);
    TileLayer* GetLayer(uint32_t index);
    const TileLayer* GetLayer(uint32_t index) const;
    uint32_t GetLayerCount() const { return static_cast<uint32_t>(m_layers.size()); }

    uint32_t GetTile(uint32_t layer, uint32_t x, uint32_t y) const;
    void SetTile(uint32_t layer, uint32_t x, uint32_t y, uint32_t tileId);

    bool IsSolid(uint32_t layer, uint32_t x, uint32_t y) const;

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetTileSize() const { return m_tileSet.GetTileSize(); }

private:
    TileSet m_tileSet;
    std::vector<TileLayer> m_layers;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Tilemap
```

**Step 4: Create Tilemap.cpp (JSON serialization)**

```cpp
#include "Tilemap.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <glaze/glaze.hpp>

namespace Prisma::Tilemap {

// Glaze reflection for TileDef
template<>
struct glz::meta<TileDef> {
    using T = TileDef;
    static constexpr auto value = object(
        "tileId", &T::tileId,
        "texIndex", &T::texIndex,
        "collisionFlags", &T::collisionFlags
    );
};

// Glaze reflection for TileLayer
template<>
struct glz::meta<TileLayer> {
    using T = TileLayer;
    static constexpr auto value = object(
        "name", &T::name,
        "width", &T::width,
        "height", &T::height,
        "data", &T::tiles,
        "visible", &T::visible,
        "sortingOrder", &T::sortingOrder
    );
};

// Internal JSON structure for Tilemap
struct TilemapJSON {
    uint32_t tileSize = 16;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string texturePath;
    uint32_t columns = 8;
    std::vector<TileDef> tiles;
    std::vector<TileLayer> layers;
};

template<>
struct glz::meta<TilemapJSON> {
    using T = TilemapJSON;
    static constexpr auto value = object(
        "tileSize", &T::tileSize,
        "width", &T::width,
        "height", &T::height,
        "texturePath", &T::texturePath,
        "columns", &T::columns,
        "tiles", &T::tiles,
        "layers", &T::layers
    );
};

bool Tilemap::LoadFromJSON(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        LOG_ERROR("Tilemap", "Cannot open file: {}", path);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0);
    std::string buf(static_cast<size_t>(size), '\0');
    file.read(buf.data(), size);

    TilemapJSON data;
    auto err = glz::read_json(data, buf);
    if (err) {
        LOG_ERROR("Tilemap", "JSON parse error: {}", glz::format_error(err, buf));
        return false;
    }

    m_width = data.width;
    m_height = data.height;
    m_tileSet.SetTileSize(data.tileSize);
    m_tileSet.SetTexturePath(data.texturePath);
    m_tileSet.SetColumns(data.columns);
    for (auto& td : data.tiles) {
        m_tileSet.AddTileDef(td);
    }
    m_layers = std::move(data.layers);

    LOG_INFO("Tilemap", "Loaded: {} ({}x{}, {} layers, {} tiles)",
             path, m_width, m_height, m_layers.size(), m_tileSet.GetTileCount());
    return true;
}

bool Tilemap::SaveToJSON(const std::string& path) const {
    TilemapJSON data;
    data.tileSize = m_tileSet.GetTileSize();
    data.width = m_width;
    data.height = m_height;
    data.texturePath = m_tileSet.GetTexturePath();
    data.columns = m_tileSet.GetColumns();
    data.layers = m_layers;

    // Collect tile defs
    for (uint32_t i = 0; i < m_tileSet.GetTileCount(); i++) {
        const auto* def = m_tileSet.GetTileDef(i);
        if (def) data.tiles.push_back(*def);
    }

    std::string json = glz::write_json(data);
    std::ofstream file(path);
    if (!file) return false;
    file << json;
    return true;
}

TileLayer* Tilemap::AddLayer(const std::string& name, uint32_t w, uint32_t h) {
    TileLayer layer;
    layer.name = name;
    layer.width = w;
    layer.height = h;
    layer.tiles.resize(static_cast<size_t>(w) * h, 0);
    m_layers.push_back(std::move(layer));
    return &m_layers.back();
}

TileLayer* Tilemap::GetLayer(uint32_t index) {
    return (index < m_layers.size()) ? &m_layers[index] : nullptr;
}

const TileLayer* Tilemap::GetLayer(uint32_t index) const {
    return (index < m_layers.size()) ? &m_layers[index] : nullptr;
}

uint32_t Tilemap::GetTile(uint32_t layer, uint32_t x, uint32_t y) const {
    if (layer >= m_layers.size()) return 0;
    return m_layers[layer].GetTile(x, y);
}

void Tilemap::SetTile(uint32_t layer, uint32_t x, uint32_t y, uint32_t tileId) {
    if (layer < m_layers.size())
        m_layers[layer].SetTile(x, y, tileId);
}

bool Tilemap::IsSolid(uint32_t layer, uint32_t x, uint32_t y) const {
    if (layer >= m_layers.size()) return false;
    uint32_t tileId = m_layers[layer].GetTile(x, y);
    if (tileId == 0) return false; // empty
    const auto* def = m_tileSet.GetTileDef(tileId);
    return def && (def->collisionFlags & TileCollision::Solid);
}

} // namespace Prisma::Tilemap
```

**Step 5: Update TilemapAsset.h forwarding header**

In `src/engine/resource/TilemapAsset.h`, replace the content with:
```cpp
#pragma once
// Tilemap is now implemented directly; this header maintained for compatibility
#include "../tilemap/Tilemap.h"
```

**Step 6: Add to CMake**

Add `src/engine/tilemap/` to the engine's source list and include paths.

**Step 7: Verify compilation**

```bash
cmake --build build/linux-x64-debug --target Engine
```

**Step 8: Commit**

```bash
git add src/engine/tilemap/ \
       src/engine/resource/TilemapAsset.h
git commit -m "feat(tilemap): implement tilemap system with JSON serialization"
```

---

## Task 8: TilemapRenderer — batch rendering with frustum culling

**Files:**
- Create: `src/engine/tilemap/TilemapRenderer.h`
- Create: `src/engine/tilemap/TilemapRenderer.cpp`

**Step 1: Create TilemapRenderer.h**

```cpp
#pragma once

#include "Tilemap.h"
#include "graphic/Camera.h"
#include "graphic/Renderer2D.h"
#include <memory>

namespace Prisma::Tilemap {

class TilemapRenderer {
public:
    TilemapRenderer() = default;

    void SetTilemap(std::shared_ptr<Tilemap> tilemap) { m_tilemap = tilemap; }
    std::shared_ptr<Tilemap> GetTilemap() const { return m_tilemap; }

    void Render(const OrthographicCamera& camera);

    // Set the tileset texture once loaded
    void SetTexture(std::shared_ptr<Graphic::ITexture> texture) { m_texture = texture; }

private:
    std::shared_ptr<Tilemap> m_tilemap;
    std::shared_ptr<Graphic::ITexture> m_texture;
};

} // namespace Prisma::Tilemap
```

**Step 2: Create TilemapRenderer.cpp**

```cpp
#include "TilemapRenderer.h"
#include "Logger.h"
#include <cmath>

namespace Prisma::Tilemap {

void TilemapRenderer::Render(const OrthographicCamera& camera) {
    if (!m_tilemap || !m_texture) return;

    uint32_t tileSize = m_tilemap->GetTileSize();
    float worldTileSize = static_cast<float>(tileSize);

    // Calculate visible range based on camera bounds
    float camLeft = camera.GetPosition().x - camera.GetAspectRatio() * camera.GetZoom() * 0.5f;
    float camRight = camera.GetPosition().x + camera.GetAspectRatio() * camera.GetZoom() * 0.5f;
    float camBottom = camera.GetPosition().y - camera.GetZoom() * 0.5f;
    float camTop = camera.GetPosition().y + camera.GetZoom() * 0.5f;

    // Expand by one tile to avoid edge popping
    camLeft -= worldTileSize;
    camRight += worldTileSize;
    camBottom -= worldTileSize;
    camTop += worldTileSize;

    int startX = std::max(0, static_cast<int>(std::floor(camLeft / worldTileSize)));
    int startY = std::max(0, static_cast<int>(std::floor(camBottom / worldTileSize)));
    int endX = std::min(static_cast<int>(m_tilemap->GetWidth()),
                        static_cast<int>(std::ceil(camRight / worldTileSize)));
    int endY = std::min(static_cast<int>(m_tilemap->GetHeight()),
                        static_cast<int>(std::ceil(camTop / worldTileSize)));

    // Render each visible layer
    for (uint32_t l = 0; l < m_tilemap->GetLayerCount(); l++) {
        const auto* layer = m_tilemap->GetLayer(l);
        if (!layer || !layer->visible) continue;

        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                uint32_t tileId = layer->GetTile(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
                if (tileId == 0) continue; // empty

                // Calculate world position (tile center)
                float wx = static_cast<float>(x) * worldTileSize + worldTileSize * 0.5f;
                float wy = static_cast<float>(y) * worldTileSize + worldTileSize * 0.5f;

                // Calculate UV in tileset
                const auto* def = m_tilemap->GetTileSet().GetTileDef(tileId);
                uint32_t texIndex = def ? def->texIndex : 0;
                uint32_t cols = m_tilemap->GetTileSet().GetColumns();
                uint32_t tx = texIndex % cols;
                uint32_t ty = texIndex / cols;

                glm::vec4 uvRect(
                    static_cast<float>(tx) / static_cast<float>(cols),
                    static_cast<float>(ty) / static_cast<float>(cols),
                    1.0f / static_cast<float>(cols),
                    1.0f / static_cast<float>(cols)
                );

                // Draw the tile
                Renderer2D::DrawQuad(
                    {wx, wy, 0.0f},                    // position
                    {worldTileSize, worldTileSize},     // size
                    m_texture.get(),                    // texture
                    glm::vec4(1.0f),                    // color
                    uvRect                              // UV rect
                );
            }
        }
    }
}

} // namespace Prisma::Tilemap
```

**Step 3: Verify compilation**

```bash
cmake --build build/linux-x64-debug --target Engine
```

**Step 4: Commit**

```bash
git add src/engine/tilemap/TilemapRenderer.h \
       src/engine/tilemap/TilemapRenderer.cpp
git commit -m "feat(tilemap): add TilemapRenderer with frustum culling"
```

---

## Task 9: C# bindings for Physics2D + Tilemap

**Files:**
- Modify: `src/engine/scripting/ScriptEngine.h` — add new function pointers to PrismaAPI
- Modify: `src/engine/scripting/ScriptEngine.cpp` — register new API functions
- Modify: `src/engine/scripting/CSharp/Prisma.Core/EngineAPI.cs` — declare extern functions

**Step 1: Add function pointer types to PrismaAPI (ScriptEngine.h)**

Add to the `PrismaAPI` struct:
```cpp
// Physics2D
int(*Physics2D_CheckAABB)(float, float, float, float, float, float, float, float);
int(*Physics2D_ResolvePlatform)(float, float, float, float,
    float* velX, float* velY,
    const float* solidData, int solidCount,
    int* onGround, int* hitCeiling);

// Tilemap
uint32_t(*Tilemap_Load)(const char* path);
void(*Tilemap_Unload)(uint32_t handle);
uint32_t(*Tilemap_GetTile)(uint32_t handle, int layer, int x, int y);
int(*Tilemap_IsSolid)(uint32_t handle, int x, int y);
uint32_t(*Tilemap_GetWidth)(uint32_t handle);
uint32_t(*Tilemap_GetHeight)(uint32_t handle);
```

**Step 2: Implement C-callable wrappers and register them in ScriptEngine.cpp**

```cpp
// Physics2D wrappers
static int PR_Physics2D_CheckAABB(
    float aMinX, float aMinY, float aMaxX, float aMaxY,
    float bMinX, float bMinY, float bMaxX, float bMaxY) {
    using namespace Physics2D;
    return Physics2D::CheckAABB({aMinX, aMinY, aMaxX, aMaxY},
                                 {bMinX, bMinY, bMaxX, bMaxY}) ? 1 : 0;
}
```

Register in the function pointer assignment block.

**Step 3: Update C# EngineAPI.cs**

Add corresponding `[DllImport]` declarations or function pointer assignments.

**Step 4: Verify compilation**

Both C++ and C# sides should compile cleanly.

**Step 5: Commit**

```bash
git add src/engine/scripting/ScriptEngine.h \
       src/engine/scripting/ScriptEngine.cpp \
       src/engine/scripting/CSharp/Prisma.Core/EngineAPI.cs
git commit -m "feat(api): add C# bindings for Physics2D and Tilemap"
```

---

## Task 10: MetroidvaniaDemo project scaffolding

**Files:**
- Create: `projects/MetroidvaniaDemo/CMakeLists.txt`
- Create: `projects/MetroidvaniaDemo/src/main.cpp`
- Create: `projects/MetroidvaniaDemo/src/MetroidvaniaApp.h`
- Create: `projects/MetroidvaniaDemo/src/MetroidvaniaApp.cpp`
- Create: `projects/MetroidvaniaDemo/src/CreateApplication.cpp`
- Create: `projects/MetroidvaniaDemo/scripts/GameScripts.csproj`
- Create: `projects/MetroidvaniaDemo/scripts/GameScripts/ScriptEntry.cs` (minimal bootstrap)
- Modify: `projects/CMakeLists.txt` — add option and subdirectory

(Full project scaffold follows the pattern of `projects/Prisma2D/`)

**Step 1: Create CMakeLists.txt** (adapted from Prisma2D's)

**Step 2: Create C++ app files** (main.cpp, MetroidvaniaApp.h/.cpp, CreateApplication.cpp)

**Step 3: Create C# project** (GameScripts.csproj + ScriptEntry.cs)

**Step 4: Create project config JSON**

```json
{
    "name": "MetroidvaniaDemo",
    "window": {
        "width": 1024,
        "height": 896,
        "vsync": 2,
        "resizable": true,
        "fullscreen": false,
        "maxFPS": 0
    },
    "rendering": {
        "maxSamples": 1,
        "maxBounces": 4,
        "hardwareRayTracing": false,
        "pathTraceMode": false,
        "enableNEE": false
    },
    "renderMode": "2D",
    "scriptingBackend": "CoreCLR",
    "entryScene": "",
    "assets": [],
    "scenes": []
}
```

**Step 5: Register in projects/CMakeLists.txt**

```cmake
option(PRISMA_BUILD_PROJECT_METROIDVANIADEMO "Build MetroidvaniaDemo" ON)
if(PRISMA_BUILD_PROJECT_METROIDVANIADEMO)
    add_subdirectory(MetroidvaniaDemo)
endif()
```

**Step 6: Verify build**

```bash
cmake --preset linux-x64-debug
cmake --build build/linux-x64-debug --target MetroidvaniaDemo
```

Expected: Builds successfully, launches a window with black/clear screen at 1024×896.

**Step 7: Commit**

```bash
git add projects/MetroidvaniaDemo/ \
       projects/CMakeLists.txt
git commit -m "feat(project): add MetroidvaniaDemo project scaffolding"
```

---

## Task 11: Pixel art assets

Create simple pixel art assets for the demo:
- `projects/MetroidvaniaDemo/assets/textures/dungeon_tileset.png` — 16×16 tile spritesheet (128×64 = 8×4 tiles)
- `projects/MetroidvaniaDemo/assets/textures/player.png` — 16×16 player sprite
- `projects/MetroidvaniaDemo/assets/textures/pickup.png` — 8×8 dash pickup

(Generate as PNG files using simple pixel patterns, or leave as placeholders with colored blocks)

---

## Task 12-14: C# game scripts

C# scripts implement game logic:
- PlayerController: movement, jump, dash, Physics2D integration
- CameraFollow: smooth follow with pixel-snapping
- TilemapCollider: reads tile collision data, feeds to Physics2D
- AbilityGate: triggers room transition when player has ability
- DashPickup: collectable that unlocks dash ability
- Enemy: simple patrol behavior

---

## Task 15: Map design — 3 rooms tilemap JSON

Create `projects/MetroidvaniaDemo/assets/maps/test_dungeon.json`:
- Room 1 (40×30 tiles): Starting area with platforms
- Room 2 (40×30 tiles): Dash pickup + gap too wide for jump
- Room 3 (40×30 tiles): End goal with chest

Room connections defined by camera-triggered transitions at edges.

---

## Task 16: Integration build + test

Full build, run, and verify:
- Window opens at 1024×896 (256×224 ×4)
- Pixel-perfect rendering (sharp pixels, no blur)
- Player can move, jump, dash
- Tilemap collision works
- Room transitions trigger correctly
- CRT effect visible when enabled
- FPS stable (target 60fps)

---

## Summary of Files

| Task | Files to Create | Files to Modify |
|------|----------------|-----------------|
| 1 | — | SRPGraphicsAPI.h, SRPGraphicsAPI.cpp |
| 2 | — | SRPGraphicsAPI.cpp |
| 3 | — | Engine.cpp |
| 4 | PixelPerfectPass.h, PixelPerfectPass.cpp | Pipeline2D.h, Pipeline2D.cpp |
| 5 | CRTScanline.frag | PostProcessPass2D.h, PostProcessPass2D.cpp, Pipeline2D.cpp |
| 6 | AABB2D.h, Physics2D.h, Physics2D.cpp | CMakeLists (engine) |
| 7 | 6 files in tilemap/ | TilemapAsset.h |
| 8 | TilemapRenderer.h, TilemapRenderer.cpp | — |
| 9 | — | ScriptEngine.h, ScriptEngine.cpp, EngineAPI.cs |
| 10 | 7+ files in MetroidvaniaDemo/ | projects/CMakeLists.txt |
| 11 | PNG assets | — |
| 12-14 | C# scripts | — |
| 15 | map JSON | — |
| 16 | — | — |
