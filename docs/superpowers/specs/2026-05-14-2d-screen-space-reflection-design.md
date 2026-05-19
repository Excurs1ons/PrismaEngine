# 2D Screen-Space Reflection (SSR) Material - Design Spec

**Date**: 2026-05-14
**Author**: Sisyphus (AI Agent)
**Status**: Draft

## Overview

Add real-time screen-space reflection (SSR) material support to the 2D rendering pipeline. After the scene is rendered, reflective surfaces (water, ice, polished floors, mirrors) sample the scene color buffer at reflected/mirrored UV coordinates to create realistic environmental reflections.

## Architecture

### Pipeline Execution Order (Modified)

```
[Before]  DepthPrePass → Light2DPass → OpaquePass → SkyboxPass → TransparentPass → Gizmo
[After]   DepthPrePass → Light2DPass → OpaquePass → [Capture] → ReflectionPass2D → SkyboxPass → TransparentPass → Gizmo
```

The `[Capture]` step copies the current scene color (from the swapchain or intermediate RT) to a `ReflectionSource` texture for use by the reflection pass.

### Data Flow

```
OpaquePass (renders to swapchain/RT)
    │
    ▼ (vkCmdBlitImage / vkCmdCopyImage after render pass ends)
ReflectionSource Texture (RGBA8_UNorm, viewport-sized)
    │
    ▼ (sampled at reflected UV coords in shader)
ReflectionPass2D
    ├── Shader: ReflectionSprite.frag
    │   ├── binding 0: AlbedoMap (surface texture)
    │   ├── binding 1: ReflectionSource (scene color)
    │   ├── binding 2: NormalMap (optional, for distortion)
    │   └── push constants: reflectOffset, strength, fade, tint, distortion
    └── Draws reflective quads with alpha blending onto swapchain
```

## New Files

### Shaders

**`assets/shaders/ReflectionSprite.frag`** (GLSL 450):

```glsl
#version 450
layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D AlbedoMap;
layout(binding = 1) uniform sampler2D ReflectionSource;
layout(binding = 2) uniform sampler2D NormalMap;

// Note: Reflection params use a UBO (binding=3) instead of push constants,
// because the vertex shader (Renderer2D.vert.spv) uses push constants
// at offset 0-79 for MVP+Color. Using a dedicated UBO avoids layout conflicts
// while allowing the same vertex shader to be reused.
layout(binding = 3) uniform ReflectionUBO {
    vec2  u_ReflectOffset;     // Reflection UV offset (e.g., (0,1) = vertical flip)
    float u_ReflectStrength;   // Reflection intensity [0, 1]
    float u_FadeDistance;      // Fade distance from edge
    float u_Distortion;        // Normal map distortion strength
    vec4  u_TintColor;         // Reflection tint color
};

void main() {
    vec4 texColor = texture(AlbedoMap, v_UV);

    // Calculate reflection UV with optional normal map distortion
    vec2 reflectUV = v_UV + u_ReflectOffset;
    
    // Normal map perturbation (optional)
    vec3 normal = texture(NormalMap, v_UV).rgb * 2.0 - 1.0;
    reflectUV += normal.xy * u_Distortion;

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

**`assets/shaders/ReflectionSprite.frag.spv`** — Compiled SPIR-V from above GLSL.

### C++ Source

**`src/engine/graphic/2d/ReflectionPass2D.h`**:

```cpp
#pragma once
#include "../pipelines/forward/ForwardRenderPassBase.h"
#include <memory>

namespace Prisma::Graphic {

class ITexture;
class IShader;
class IPipelineState;
class IRenderDevice;

class ReflectionPass2D : public ForwardRenderPass {
public:
    ReflectionPass2D();
    ~ReflectionPass2D() override;

    void Execute(const PassExecutionContext& context) override;
    void ExecuteReflection(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height);
    void Update(Prisma::Timestep ts) override;

    std::shared_ptr<ITexture> GetReflectionSource() const { return m_reflectionSource; }

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);

    std::shared_ptr<ITexture> m_reflectionSource;
    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_fragmentShader;
    std::shared_ptr<IPipelineState> m_reflectionPSO;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
```

**`src/engine/graphic/2d/ReflectionPass2D.cpp`**:

Key components:
1. **`EnsureResources()`**: Creates `m_reflectionSource` texture (RGBA8_UNorm, viewport-sized, render-target+shader-resource flags), loads `Renderer2D.vert.spv` + `ReflectionSprite.frag.spv`, creates PSO with alpha blending (`SrcAlpha : OneMinusSrcAlpha`).
2. **`ExecuteReflection()`**: 
   - Captures scene content to `m_reflectionSource` via blit
   - Begins render pass on swapchain (uses existing swapchain RT)
   - Sets up reflection PSO
    - Issues draws for reflective quads with UBO binding for reflection params
3. UBO binding layout (binding=3): vec2 reflectOffset (8 bytes) + float strength (4) + float fadeDistance (4) + float distortion (4) + vec4 tintColor (16) = 36 bytes (aligned to 48 bytes in std430).

## Modified Files

### `ForwardPipeline.h`

Add member:
```cpp
std::shared_ptr<ReflectionPass2D> m_reflectionPass2D;
```

### `ForwardPipeline.cpp`

Changes in `Initialize()`:
```cpp
m_reflectionPass2D = std::make_shared<ReflectionPass2D>();
```

Changes in `Execute()` — insert after OpaquePass, before SkyboxPass:
```cpp
// Capture scene → ReflectionSource
if (m_reflectionPass2D) {
    m_reflectionPass2D->SetViewMatrix(view);
    m_reflectionPass2D->SetProjectionMatrix(proj);
    m_reflectionPass2D->Execute(passContext);
    if (ctx.commandBuffer) {
        m_reflectionPass2D->ExecuteReflection(ctx.commandBuffer, m_device, ctx.width, ctx.height);
    }
}
```

Changes in `Shutdown()`:
```cpp
m_reflectionPass2D.reset();
```

### `src/engine/CMakeLists.txt`

Add after line 159 (under `# Graphic - 2D`):
```cmake
graphic/2d/ReflectionPass2D.cpp
```

## Material Integration

### New Material Parameters

Reflective sprites use the existing `Material::SetParam()` system:

| Parameter | Type | Description |
|-----------|------|-------------|
| `"AlbedoMap"` | `shared_ptr<ITexture>` | Surface texture (existing) |
| `"ReflectionSource"` | `shared_ptr<ITexture>` | Scene color (auto-set by ReflectionPass2D) |
| `"NormalMap"` | `shared_ptr<ITexture>` | Optional normal map for distortion |
| `"ReflectParams"` | `IBuffer` (UBO) | Strength, fade, distortion, tint packed as uniform buffer |

### Renderer2D Extension (Optional)

For backward compatibility, `Renderer2D` can be extended with:
- `SetReflectionMap(std::shared_ptr<ITexture>)` — alternative/additional reflection source
- A new reflection material variant in `Flush()`

### How SpriteRenderer Uses It

New `SpriteRenderer` material properties:
- `ReflectionEnabled` (bool) — marks sprite as reflective
- `ReflectStrength` (float) — how strong the reflection is [0, 1]
- `ReflectOffset` (vec2) — UV offset for the reflection lookup
- `NormalMap` (texture) — optional distortion map

## Vulkan Specifics

### Render Pass Management

The scene capture requires:
1. Ending the swapchain render pass (if currently active)
2. Transitioning swapchain image: `COLOR_ATTACHMENT_OPTIMAL → TRANSFER_SRC_OPTIMAL`
3. Transitioning `ReflectionSource`: `UNDEFINED → TRANSFER_DST_OPTIMAL`
4. Issuing `vkCmdBlitImage` or `vkCmdCopyImage`
5. Transitioning back for subsequent use
6. Resuming the swapchain render pass

This follows the existing `EndSwapChainRenderPass()`/`BeginSwapChainRenderPass()` pattern used by Light2DPass.

### Pipeline State

- **Blending**: Alpha blend `(SrcAlpha, OneMinusSrcAlpha)` for semi-transparent reflections
- **Depth**: Depth test ON, depth write OFF (reflections overlay on existing geometry)
- **Culling**: Cull mode NONE (2D surfaces, potentially double-sided)

## Test / Demo Plan

A simple 2D demo scene to verify:
1. **Basic reflection**: A reflective quad at the bottom of the screen showing a flipped reflection of content above it
2. **Strength control**: Multiple quads with different `ReflectStrength` values
3. **Normal map distortion**: A reflective surface with a normal map for "water ripple" effect
4. **Edge fade**: Reflections fade out at the edges of the reflective surface
5. **Tint**: Colored reflections (e.g., blue tint for water)

## Future Extensions (Not in Scope)

- Multiple reflection source textures (layered reflections)
- Blurred reflections (Gaussian blur on ReflectionSource)
- Dynamic reflection probes for off-screen content
- Reflection mask render target for complex reflection shapes
- Integration with 2D light system for specular highlights on reflections

## Open Questions (Resolved)

- **Q**: How to capture scene content on Vulkan? **A**: Use `vkCmdBlitImage` between the swapchain and `ReflectionSource` texture, following the existing `EndSwapChainRenderPass/BeginSwapChainRenderPass` pattern from Light2DPass.
- **Q**: Should reflections be a separate material or an extension of LitSprite? **A**: Separate material (`ReflectionSprite.frag`) for clean separation and maintainability.
- **Q**: Where in the pipeline order? **A**: After OpaquePass (scene is rendered), before SkyboxPass and TransparentPass (reflections should not reflect skybox or transparent objects for simplicity in v1).

## Implementation Order

1. Create `ReflectionSprite.frag` GLSL shader source
2. Compile to `ReflectionSprite.frag.spv`
3. Create `ReflectionPass2D.h` and `ReflectionPass2D.cpp` — resource creation & PSO
4. Implement scene capture (blit from current color buffer)
5. Implement reflective surface rendering
6. Integrate into `ForwardPipeline`
7. Register new source in `CMakeLists.txt`
8. Create test/demo scene to verify
9. Run diagnostics and fix any issues
