# MobileLumenPipeline Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 将 MobileLumenPipeline 从空骨架升级为最小可渲染管线,显示程序化物理天空(Rayleigh+Mie)+ 地面平面 + 太阳,UE3 默认场景风格。

**Architecture:** 方案 B 独立精简骨架(不复用/不继承 ForwardPipeline)。MobileLumenPipeline 自持 `DepthPrePass` + `OpaquePass` + 新建 `ProceduralSkyPass`,Execute 7 步:BeginSwapChain -> UpdateGI(空 hook) -> DepthPrePass -> OpaquePass -> ProceduralSkyPass -> EndSwapChain。ProceduralSkyPass 自建 PSO(参考 EnsureGizmoPSO 模式)+ 立方体 mesh(参考 SkyboxPass)+ 自有 Rayleigh/Mie shader。MLGISystem 预留空 hook,不接实际功能。

**Tech Stack:** C++23, Vulkan, GLSL 450, PrismaEngine ForwardRenderPass/IPipeline 框架,glslangValidator 编译 SPIR-V。

**设计文档:** `docs/plans/2026-07-08-mobilelumen-pipeline-design.md`(已提交 dev 6a91854b)

**验证策略:** 项目无测试框架(PRISMA_BUILD_TESTING=OFF)。用 Engine target 编译通过 + MLGIPipeline MobileLumen 模式运行显示正确画面代替 TDD。

---

## 关键 API 参考(实现时参照)

### ForwardPipeline::Execute 模板(src/engine/graphic/pipelines/forward/ForwardPipeline.cpp:190-358)
- `if (!ctx.targetTexture) ctx.device->BeginSwapChainRenderPass(ctx.clearColor);`
- `TextureRenderTargetProxy proxy(ctx.targetTexture);` (定义 .cpp:38-69)
- `Renderer::GetCommandQueue()` 返回 `const std::vector<RenderCommand>&`
- 构造 `SceneData` + `PassExecutionContext`(deviceContext/sceneData/renderTarget)
- `RenderCommandContext fallbackContext; IDeviceContext* deviceContext = &fallbackContext;`
- DepthPrePass: `SetViewMatrix(view); SetProjectionMatrix(proj); Execute(passContext);`
- OpaquePass: `SetViewMatrix; SetProjectionMatrix; SetLights(ctx.lights); Execute(ctx.commandBuffer, commands);` (旧版 Execute 签名)
- SkyboxPass: `SetViewMatrix; SetProjectionMatrix; Execute(passContext);`
- `if (!ctx.targetTexture) ctx.device->EndSwapChainRenderPass();`

### EnsureGizmoPSO - PSO 创建模板(ForwardPipeline.cpp:165-188)
```cpp
auto rm = Engine::Get().GetRenderResourceManager();
m_vertShader = rm->LoadShaderSync("assets/shaders/procedural_sky.vert.spv");
m_fragShader = rm->LoadShaderSync("assets/shaders/procedural_sky.frag.spv");
auto pso = m_device->GetResourceFactory()->CreatePipelineStateImpl();
pso->SetShader(ShaderType::Vertex, m_vertShader);
pso->SetShader(ShaderType::Pixel, m_fragShader);
pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
RasterizerState rs; rs.cullMode = CullMode::None;
pso->SetRasterizerState(rs);
DepthStencilState ds{}; ds.depthEnable = false; ds.depthWriteEnable = false;
pso->SetDepthStencilState(ds);
if (pso->Create(m_device)) m_pso = std::shared_ptr<IPipelineState>(std::move(pso));
```

### SkyboxPass mesh 模板(SkyboxRenderPass.cpp:65-182)
24 顶点立方体 + 36 索引,`InitializeSkyboxMesh()` 填充 m_vertices/m_indices。Execute 用 `SetConstantData(0, &viewProj, sizeof(mat4))` + `SetVertexData` + `SetIndexData` + `DrawIndexed`。ProceduralSkyPass 复制此 mesh,但额外 `SetPipelineState(m_pso.get())`。

### 关键类型
- `ForwardRenderPass`(ForwardRenderPassBase.h):基类,提供 `m_view/m_projection/m_viewProjection` + SetViewMatrix/SetProjectionMatrix。继承自 LogicalPass -> IPass。
- `PassExecutionContext`(IPass.h:66-77):`{ IDeviceContext* deviceContext; IRenderTarget* renderTarget; IDepthStencil* depthStencil; const SceneData* sceneData; }`
- `SceneData`(IPass.h:14-62):camera{view,projection,viewProjection,position,direction,nearPlane,farPlane} / time{ts,totalTime} / viewport{width,height} / lighting{ambientColor,ambientIntensity}
- `IDeviceContext`(IDeviceContext.h):SetPipelineState / SetViewport / SetVertexData / SetIndexData / SetConstantData / SetTexture / DrawIndexed
- `RenderContext`(IPipeline.h:32-49):device/commandBuffer/targetTexture/camera{viewMatrix,projectionMatrix,position,nearPlane,farPlane}/clearColor/lights/frameIndex/deltaTime/width/height
- `Light`(RenderTypes.h:582-586):direction.xyz + w(< 0.5 方向光)/ color.rgb + w(intensity)
- Logger 宏(Logger.h):`LOG_INFO(category, fmt, ...)` / `LOG_ERROR(category, fmt, ...)` - category 是字符串

### Shader 编译集成(src/engine/CMakeLists.txt:917-994)
三段编译模式(粒子/water/assets):`find_program(GLSLANG_VALIDATOR glslangValidator)` -> 循环 .vert/.frag -> `set(SPV_OUTPUT "${SHADER_DIR}/${NAME}.spv")` -> `add_custom_command`。procedural_sky 参考此模式注册。**实现 Task 3 时先读 CMakeLists.txt:970-994 确认 ASSETS_SHADER_DIR 物理路径,再决定 shader 源码放置位置。**

---

### Task 1: procedural_sky 着色器

**Files:**
- Create: `resources/common/shaders/glsl/procedural_sky.vert`(源码,与 skybox.frag 同级)
- Create: `resources/common/shaders/glsl/procedural_sky.frag`

**Step 1: 写 vertex shader**

全屏立方体方法:顶点位置作为方向向量传给 frag,移除 viewProj 平移,z=w 保证最远深度。

```glsl
#version 450

layout(location = 0) in vec4 aPos;

layout(location = 0) out vec3 vDir;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 viewProj;
};

void main() {
    vDir = aPos.xyz;
    mat4 vp = viewProj;
    vp[3] = vec4(0.0, 0.0, 0.0, 1.0);  // 移除平移,天空盒跟随相机
    vec4 pos = vp * vec4(aPos.xyz, 1.0);
    gl_Position = pos.xyww;  // z = w -> 深度 1.0 (最远)
}
```

**Step 2: 写 fragment shader**

Preetham 风格 Rayleigh + Mie 单次散射近似。太阳方向从 uniform 传入(由 RenderContext.lights 提取)。

```glsl
#version 450

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform SkyParams {
    vec3 sunDirection;   // 归一化方向光方向
    float sunIntensity;  // 太阳强度
    vec3 sunColor;
    float _pad0;
};

// Rayleigh/Mie 系数 (Preetham 近似)
const vec3 rayleighCoeff = vec3(5.8e-6, 13.5e-6, 33.1e-6) * 1.0;
const float mieCoeff = 21e-6;
const float rayleighScaleHeight = 8.4e3;   // 米
const float mieScaleHeight = 1.2e3;

// 相位函数
float rayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * 3.14159265)) * (1.0 + cosTheta * cosTheta);
}
float miePhase(float cosTheta, float g) {
    float g2 = g * g;
    return (3.0 / (8.0 * 3.14159265)) * ((1.0 - g2) * (1.0 + cosTheta * cosTheta))
         / ((2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

void main() {
    vec3 dir = normalize(vDir);
    vec3 sunDir = normalize(sunDirection);

    // 简化:假设观察方向穿过整层大气(地面观察)
    float cosTheta = dot(dir, sunDir);

    // Rayleigh 散射
    float rayleighOptical = 1.0;  // 简化:整层
    vec3 rayleighScatter = rayleighCoeff * rayleighPhase(cosTheta);
    // 衰减
    vec3 rayleighTransmit = exp(-(rayleighCoeff * 1.0));

    // Mie 散射
    float mieOptical = 1.0;
    float g = 0.76;
    float mieScatter = mieCoeff * miePhase(cosTheta, g);
    float mieTransmit = exp(-(mieCoeff * 1.0));

    // 天空颜色 = 散射光 * 透射
    vec3 skyColor = (rayleighScatter * rayleighTransmit + mieScatter * mieTransmit) * sunColor * sunIntensity;

    // 太阳圆盘(强光晕)
    float sunDisk = smoothstep(0.9995, 0.9999, cosTheta);
    skyColor += sunColor * sunIntensity * sunDisk * 200.0;

    // 地平线增亮(简化)
    float horizon = 1.0 - abs(dir.y);
    skyColor += vec3(0.3, 0.2, 0.1) * horizon * horizon * 0.3;

    // 最低亮度保底
    skyColor = max(skyColor, vec3(0.01, 0.02, 0.05));

    FragColor = vec4(skyColor, 1.0);
}
```

**Step 3: 验证 GLSL 语法**

Run: `glslangValidator -V resources/common/shaders/glsl/procedural_sky.vert resources/common/shaders/glsl/procedural_sky.frag`
Expected: 编译成功生成 .spv(或报告语法错误需修正)

**Step 4: Commit**

```bash
git add resources/common/shaders/glsl/procedural_sky.vert resources/common/shaders/glsl/procedural_sky.frag
git commit -m "feat(mobilelumen): add procedural sky shaders (Rayleigh+Mie)"
```

---

### Task 2: ProceduralSkyPass 类

**Files:**
- Create: `src/engine/graphic/pipelines/ProceduralSkyPass.h`
- Create: `src/engine/graphic/pipelines/ProceduralSkyPass.cpp`

**Step 1: 写 ProceduralSkyPass.h**

继承 ForwardRenderPass(与 SkyboxPass 同基类)。持立方体 mesh + 自有 PSO + shader + 太阳参数。

```cpp
#pragma once

#include "forward/ForwardRenderPassBase.h"
#include "graphic/Mesh.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/ITexture.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class IRenderDevice;
class IShader;
class IPipelineState;

// 程序化物理天空 Pass (Rayleigh + Mie 单次散射)
// 独立于 SkyboxPass,使用自有 PSO + shader,不依赖外部绑定。
class ProceduralSkyPass : public ForwardRenderPass {
public:
    ProceduralSkyPass();
    ~ProceduralSkyPass() override = default;

    // IPass 接口
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    // 初始化 PSO + shader(需 device,在管线 Initialize 阶段调用)
    bool Initialize(IRenderDevice* device);
    void Shutdown();

    // 设置太阳参数(从 RenderContext.lights 提取)
    void SetSunDirection(const PrismaMath::vec3& dir) { m_sunDirection = dir; }
    void SetSunColor(const PrismaMath::vec3& color) { m_sunColor = color; }
    void SetSunIntensity(float intensity) { m_sunIntensity = intensity; }

private:
    void InitializeSkyboxMesh();

    // PSO + shader
    IRenderDevice* m_device = nullptr;
    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_fragmentShader;
    std::shared_ptr<IPipelineState> m_pso;
    bool m_psoReady = false;

    // 立方体 mesh (复制自 SkyboxPass)
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    bool m_meshInitialized = false;

    // 太阳参数
    PrismaMath::vec3 m_sunDirection = {0.0f, -1.0f, 0.0f};
    PrismaMath::vec3 m_sunColor = {1.0f, 1.0f, 1.0f};
    float m_sunIntensity = 1.0f;

    // UBO 数据 (viewProj + sun params)
    struct SkyUBO {
        PrismaMath::mat4 viewProj;
        PrismaMath::vec4 sunDirIntensity;  // xyz=dir, w=intensity
        PrismaMath::vec4 sunColor;         // rgb=color, a=pad
    };
};

} // namespace Prisma::Graphic
```

**Step 2: 写 ProceduralSkyPass.cpp**

mesh 复制自 SkyboxRenderPass.cpp:65-182。Execute:绑 PSO -> SetConstantData(0, &SkyUBO) -> SetVertexData -> SetIndexData -> DrawIndexed。PSO 创建仿 EnsureGizmoPSO。

```cpp
#include "ProceduralSkyPass.h"
#include "app/Engine.h"
#include "graphic/Renderer.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "logger/Logger.h"

namespace Prisma::Graphic {

ProceduralSkyPass::ProceduralSkyPass()
    : ForwardRenderPass("ProceduralSkyPass") {
    m_priority = 200;
    InitializeSkyboxMesh();
}

void ProceduralSkyPass::Update(Timestep ts) {
    UpdateTime(ts);
}

bool ProceduralSkyPass::Initialize(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;

    auto rm = Engine::Get().GetRenderResourceManager();
    if (!rm) {
        LOG_ERROR("ProceduralSkyPass", "RenderResourceManager 不可用");
        return false;
    }

    m_vertexShader = rm->LoadShaderSync("assets/shaders/procedural_sky.vert.spv");
    m_fragmentShader = rm->LoadShaderSync("assets/shaders/procedural_sky.frag.spv");
    if (!m_vertexShader || !m_fragmentShader) {
        LOG_ERROR("ProceduralSkyPass", "无法加载 procedural_sky shader");
        return false;
    }

    auto pso = m_device->GetResourceFactory()->CreatePipelineStateImpl();
    if (!pso) return false;
    pso->SetShader(ShaderType::Vertex, m_vertexShader);
    pso->SetShader(ShaderType::Pixel, m_fragmentShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    RasterizerState rs; rs.cullMode = CullMode::None;
    pso->SetRasterizerState(rs);
    DepthStencilState ds{}; ds.depthEnable = false; ds.depthWriteEnable = false;
    pso->SetDepthStencilState(ds);
    if (!pso->Create(m_device)) {
        LOG_ERROR("ProceduralSkyPass", "PSO 创建失败");
        return false;
    }
    m_pso = std::shared_ptr<IPipelineState>(std::move(pso));
    m_psoReady = true;
    LOG_INFO("ProceduralSkyPass", "ProceduralSkyPass 初始化完成");
    return true;
}

void ProceduralSkyPass::Shutdown() {
    m_pso.reset();
    m_vertexShader.reset();
    m_fragmentShader.reset();
    m_psoReady = false;
    m_device = nullptr;
}

void ProceduralSkyPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext || !m_meshInitialized || !m_psoReady || !m_pso) {
        return;
    }

    // 视口
    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    // 移除平移的 viewProj
    PrismaMath::mat4 modifiedViewProjection = m_viewProjection;
    modifiedViewProjection[3] = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // 绑定自有 PSO
    context.deviceContext->SetPipelineState(m_pso.get());

    // UBO: viewProj + sun params
    SkyUBO ubo{};
    ubo.viewProj = modifiedViewProjection;
    ubo.sunDirIntensity = PrismaMath::vec4(m_sunDirection, m_sunIntensity);
    ubo.sunColor = PrismaMath::vec4(m_sunColor, 0.0f);
    context.deviceContext->SetConstantData(0, &ubo, sizeof(SkyUBO));

    // mesh
    context.deviceContext->SetVertexData(
        m_vertices.data(),
        static_cast<uint32_t>(m_vertices.size() * sizeof(Vertex)),
        static_cast<uint32_t>(sizeof(Vertex)));
    context.deviceContext->SetIndexData(
        m_indices.data(),
        static_cast<uint32_t>(m_indices.size() * sizeof(uint32_t)),
        false);
    context.deviceContext->DrawIndexed(static_cast<uint32_t>(m_indices.size()));
}

void ProceduralSkyPass::InitializeSkyboxMesh() {
    // [复制自 SkyboxRenderPass.cpp:84-178, 完整 24 顶点立方体 + 36 索引]
    // 实现 Task 时从 SkyboxRenderPass.cpp:84-178 复制 InitializeSkyboxMesh 函数体
    // m_vertices.resize(24); ... 6 面 ... m_indices = {0,1,2,...};
    m_meshInitialized = true;
}

} // namespace Prisma::Graphic
```

**注意:** `InitializeSkyboxMesh` 函数体从 `SkyboxRenderPass.cpp:84-178` 完整复制(24 顶点 + 36 索引)。实现时直接复制。

**Step 3: Commit**

```bash
git add src/engine/graphic/pipelines/ProceduralSkyPass.h src/engine/graphic/pipelines/ProceduralSkyPass.cpp
git commit -m "feat(mobilelumen): add ProceduralSkyPass with Rayleigh+Mie PSO"
```

---

### Task 3: CMake 注册

**Files:**
- Modify: `src/engine/CMakeLists.txt`

**Step 1: 先读 CMakeLists.txt:965-995 确认 assets shader 编译段物理路径**

Run: 读 `src/engine/CMakeLists.txt` 行 965-995,确认 `ASSETS_SHADER_DIR` 定义和 shader 源码物理目录。

**Step 2: 注册 ProceduralSkyPass.cpp 到 CORE_SOURCES**

在 `src/engine/CMakeLists.txt` 行 172(`graphic/pipelines/mobilelumen/MobileLumenPipeline.cpp`)附近,加:
```cmake
    graphic/pipelines/ProceduralSkyPass.cpp
```

**Step 3: 注册 procedural_sky shader 编译**

参考 CMakeLists.txt:975-994 assets shader 编译段,将 `procedural_sky.vert` 和 `procedural_sky.frag` 加入编译列表,输出 `.spv` 到 `assets/shaders/` 运行时加载目录。

**注意:** shader 源码在 `resources/common/shaders/glsl/`,但 LoadShaderSync 加载 `assets/shaders/*.spv`。实现时确认 CMake 编译输出目录与 LoadShaderSync 路径一致。若引擎有运行时 GLSL->SPIRV 编译(ShaderFactory.cpp glslang),.spv 可能运行时生成。

**Step 4: Commit**

```bash
git add src/engine/CMakeLists.txt
git commit -m "build(mobilelumen): register ProceduralSkyPass + procedural_sky shaders"
```

---

### Task 4: MobileLumenPipeline 重写

**Files:**
- Modify: `src/engine/graphic/pipelines/mobilelumen/MobileLumenPipeline.h`
- Modify: `src/engine/graphic/pipelines/mobilelumen/MobileLumenPipeline.cpp`

**Step 1: 重写 MobileLumenPipeline.h**

```cpp
#pragma once

#include "Export.h"
#include "interfaces/IPipeline.h"
#include <memory>

namespace Prisma::Graphic {

class IRenderDevice;
class DepthPrePass;
class OpaquePass;
class ProceduralSkyPass;

class ENGINE_API MobileLumenPipeline : public IPipeline {
public:
    MobileLumenPipeline() = default;
    ~MobileLumenPipeline() override;

    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;
    void OnSceneLoaded(Scene* scene) override {}
    RenderMode GetMode() const override { return RenderMode::Mode3D_MobileLumen; }

    // MLGI hook (Phase 2 预留,当前空实现)
    virtual void UpdateGI(const RenderContext& /*ctx*/) {}

private:
    IRenderDevice* m_device = nullptr;
    bool m_initialized = false;

    std::shared_ptr<DepthPrePass> m_depthPrePass;
    std::shared_ptr<OpaquePass> m_opaquePass;
    std::shared_ptr<ProceduralSkyPass> m_skyPass;
};

} // namespace Prisma::Graphic
```

**Step 2: 重写 MobileLumenPipeline.cpp**

仿 ForwardPipeline::Execute(ForwardPipeline.cpp:190-358)精简版。

```cpp
#include "MobileLumenPipeline.h"
#include "ProceduralSkyPass.h"
#include "forward/DepthPrePass.h"
#include "forward/OpaquePass.h"
#include "forward/ForwardPipeline.h"  // TextureRenderTargetProxy (private, 见下)
#include "graphic/Renderer.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/adapters/vulkan/VulkanResources.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "interfaces/ICommandBuffer.h"
#include "logger/Logger.h"

namespace Prisma::Graphic {

// TextureRenderTargetProxy 定义在 ForwardPipeline.cpp 内部(匿名),
// MobileLumenPipeline 需复制一份(或提取到共享头)。
// 此处复制 ForwardPipeline.cpp:38-69 的 TextureRenderTargetProxy。
namespace {
class TextureRenderTargetProxy final : public ITextureRenderTarget {
public:
    TextureRenderTargetProxy(ITexture* texture) : m_texture(texture) {}
    uint32_t GetWidth() const override { return m_texture ? static_cast<uint32_t>(m_texture->GetWidth()) : 0; }
    uint32_t GetHeight() const override { return m_texture ? static_cast<uint32_t>(m_texture->GetHeight()) : 0; }
    TextureFormat GetFormat() const override { return m_texture ? m_texture->GetFormat() : TextureFormat::Unknown; }
    TextureType GetType() const override { return m_texture ? m_texture->GetTextureType() : TextureType::Texture2D; }
    void* GetNativeHandle() const override {
        if (!m_texture) return nullptr;
        auto vkTexture = dynamic_cast<Vulkan::VulkanTexture*>(m_texture);
        return vkTexture ? reinterpret_cast<void*>(vkTexture->GetVkImageView()) : nullptr;
    }
    bool IsSwapChain() const override { return false; }
    void Clear(const float color[4]) override {
        if (m_texture) m_texture->Clear(Color(color[0], color[1], color[2], color[3]));
    }
    uint32_t GetMipLevels() const override { return m_texture ? m_texture->GetMipLevels() : 0; }
    uint32_t GetArraySize() const override { return m_texture ? m_texture->GetArraySize() : 0; }
    ITexture* GetTexture() override { return m_texture; }
private:
    ITexture* m_texture;
};
} // namespace

MobileLumenPipeline::~MobileLumenPipeline() { Shutdown(); }

int MobileLumenPipeline::Initialize(IRenderDevice* device) {
    if (!device) {
        LOG_ERROR("MobileLumenPipeline", "Initialize 失败:device 为空");
        return -1;
    }
    m_device = device;

    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_opaquePass = std::make_shared<OpaquePass>();
    m_opaquePass->SetDevice(device);

    m_skyPass = std::make_shared<ProceduralSkyPass>();
    if (!m_skyPass->Initialize(device)) {
        LOG_WARNING("MobileLumenPipeline", "ProceduralSkyPass 初始化失败,天空将不渲染");
        m_skyPass.reset();
    }

    m_initialized = true;
    LOG_INFO("MobileLumenPipeline", "MobileLumen 管线初始化完成 (DepthPrePass + OpaquePass + ProceduralSky)");
    return 0;
}

void MobileLumenPipeline::Shutdown() {
    if (!m_initialized) return;
    if (m_skyPass) { m_skyPass->Shutdown(); m_skyPass.reset(); }
    m_opaquePass.reset();
    m_depthPrePass.reset();
    m_device = nullptr;
    m_initialized = false;
    LOG_INFO("MobileLumenPipeline", "MobileLumen 管线关闭");
}

void MobileLumenPipeline::Execute(const RenderContext& ctx) {
    if (!m_initialized || !ctx.device) return;

    // 1. Begin swapchain render pass (离屏模式跳过)
    if (!ctx.targetTexture) {
        ctx.device->BeginSwapChainRenderPass(ctx.clearColor);
    }

    TextureRenderTargetProxy proxy(ctx.targetTexture);

    const auto& commands = Renderer::GetCommandQueue();
    auto view = ctx.camera.viewMatrix;
    auto proj = ctx.camera.projectionMatrix;

    RenderCommandContext fallbackContext;
    IDeviceContext* deviceContext = &fallbackContext;

    SceneData sceneData;
    sceneData.camera.view = view;
    sceneData.camera.projection = proj;
    sceneData.camera.viewProjection = proj * view;
    sceneData.camera.position = ctx.camera.position;
    sceneData.camera.nearPlane = ctx.camera.nearPlane;
    sceneData.camera.farPlane = ctx.camera.farPlane;
    sceneData.time.ts = ctx.deltaTime;
    sceneData.viewport.width = ctx.width;
    sceneData.viewport.height = ctx.height;

    PassExecutionContext passContext;
    passContext.deviceContext = deviceContext;
    passContext.sceneData = &sceneData;
    passContext.renderTarget = ctx.targetTexture ? &proxy : nullptr;

    // 2. MLGI update hook (Phase 2 预留)
    UpdateGI(ctx);

    // 3. Depth pre-pass
    if (m_depthPrePass) {
        m_depthPrePass->SetViewMatrix(view);
        m_depthPrePass->SetProjectionMatrix(proj);
        m_depthPrePass->Execute(passContext);
    }

    // 4. Opaque pass (旧版 Execute 签名,与 ForwardPipeline 一致)
    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(proj);
        m_opaquePass->SetLights(ctx.lights);
        if (ctx.commandBuffer) {
            m_opaquePass->Execute(ctx.commandBuffer, commands);
        }
    }

    // 5. Procedural sky pass (从方向光提取太阳参数)
    if (m_skyPass) {
        m_skyPass->SetViewMatrix(view);
        m_skyPass->SetProjectionMatrix(proj);
        for (const auto& light : ctx.lights) {
            if (light.direction.w < 0.5f) {  // 方向光
                m_skyPass->SetSunDirection(PrismaMath::vec3(light.direction.x,
                                                            light.direction.y,
                                                            light.direction.z));
                m_skyPass->SetSunColor(PrismaMath::vec3(light.color.x,
                                                        light.color.y,
                                                        light.color.z));
                m_skyPass->SetSunIntensity(light.color.w);
                break;
            }
        }
        m_skyPass->Execute(passContext);
    }

    // 6. End swapchain render pass (离屏模式跳过)
    if (!ctx.targetTexture) {
        ctx.device->EndSwapChainRenderPass();
    }
}

} // namespace Prisma::Graphic
```

**注意:**
- `TextureRenderTargetProxy` 是 ForwardPipeline.cpp 内部私有类,MobileLumenPipeline 复制一份(后续可重构提取到共享头)。
- 包含 `#include "forward/ForwardPipeline.h"` 可能不需要(Proxy 是复制的)。实现时确认 include 最小化。
- `RenderCommandContext` / `IDeviceContext` / `SceneData` / `PassExecutionContext` 的 include 参照 ForwardPipeline.cpp:1-23。

**Step 3: Commit**

```bash
git add src/engine/graphic/pipelines/mobilelumen/MobileLumenPipeline.h src/engine/graphic/pipelines/mobilelumen/MobileLumenPipeline.cpp
git commit -m "feat(mobilelumen): implement minimal rendering pipeline (depth+opaque+sky)"
```

---

### Task 5: default 场景

**Files:**
- Create: `projects/MLGIPipeline/assets/scenes/default.scene.json`

**Step 1: 写场景 JSON**

地面平面 + 摄像机 + 方向光(太阳)。参考 cornell_box.scene.json 格式。天空由管线程序化生成,无需场景组件。

```json
{
    "name": "Default",
    "nodes": [
        {
            "name": "Main Camera",
            "position": [0.0, 1.0, 5.0],
            "rotation": [0.0, 0.0, 0.0, 1.0],
            "components": [{
                "type": "Camera",
                "data": { "fov": 60.0 }
            }]
        },
        {
            "name": "Floor",
            "components": [
                {"type": "PrimitiveComponent", "data": {"shape": "Plane", "material": "assets/materials/gray.mat", "emissive": [0.0, 0.0, 0.0]}},
                {"type": "MeshRenderer", "data": {"mesh": "assets/models/plane.obj", "material": "assets/materials/gray.mat", "emissive": [0.0, 0.0, 0.0]}}
            ],
            "position": [0.0, -1.0, 0.0],
            "rotation": [0.0, 0.0, 0.0, 1.0],
            "scale": [5.0, 1.0, 5.0]
        },
        {
            "name": "Sun",
            "components": [{
                "type": "Light",
                "data": {
                    "type": "Directional",
                    "direction": [-0.5, -1.0, -0.3],
                    "color": [1.0, 1.0, 1.0],
                    "intensity": 3.0
                }
            }]
        }
    ]
}
```

**注意:** Light 组件 JSON 格式需实现时确认(参考 math_wonderland 场景的方向光定义,或 LightComponent 反序列化代码)。direction 是光照方向(指向被照物),shader 里 sunDirection 直接用此值。

**Step 2: Commit**

```bash
git add projects/MLGIPipeline/assets/scenes/default.scene.json
git commit -m "feat(mlgipipeline): add default scene with floor + directional sun"
```

---

### Task 6: MLGIPipeline.jsonc entry scene

**Files:**
- Modify: `projects/MLGIPipeline/assets/MLGIPipeline.jsonc`

**Step 1: 修改 entry scene 指向 default**

将 entry scene 从 `scenes/cornell_box.scene.json` 改为 `scenes/default.scene.json`(保留 cornell_box 文件不删)。

**Step 2: Commit**

```bash
git add projects/MLGIPipeline/assets/MLGIPipeline.jsonc
git commit -m "chore(mlgipipeline): switch entry scene to default"
```

---

### Task 7: 编译验证

**Step 1: 配置 CMake**

Run:
```bash
cmake --preset windows-x64-debug
```
Expected: 配置成功

**Step 2: 编译 Engine target**

Run:
```bash
cmake --build build/windows-x64-debug --target Engine --parallel
```
Expected: 编译成功,无错误。clangd LSP 误报(`<numbers>`/Vulkan SDK 路径)忽略。

**Step 3: 修复编译错误(如有)**

常见问题:
- include 路径(参考 ForwardPipeline.cpp include 列表)
- `TextureRenderTargetProxy` 方法签名匹配 ITextureRenderTarget 接口
- `RenderCommandContext` 可用性(ForwardPipeline.cpp:204 用了)
- `Color` 类型(ForwardPipeline.cpp:59 用 `Color(r,g,b,a)`)

修复后重新编译直到通过。

**Step 4: Commit 修复**

```bash
git add -A
git commit -m "fix(mobilelumen): resolve compilation errors"
```

---

### Task 8: 运行验证

**Step 1: 编译 MLGIPipeline target**

Run:
```bash
cmake --build build/windows-x64-debug --target MLGIPipeline --parallel
```
Expected: 编译成功

**Step 2: 运行 MLGIPipeline**

Run MLGIPipeline 可执行文件。Expected:
- 窗口打开,显示蓝色物理天空(顶部深蓝,地平线浅蓝)
- 太阳光晕可见(沿方向光方向)
- 地面平面可见(灰色)
- 无崩溃/黑屏

**Step 3: 验证日志**

检查日志输出含:
- `MobileLumen 管线初始化完成 (DepthPrePass + OpaquePass + ProceduralSky)`
- `ProceduralSkyPass 初始化完成`
- 无 ERROR 级日志

**Step 4: 调试(如画面不正确)**

- 黑屏:检查 swapchain render pass / PSO / shader 加载日志
- 无天空:检查 ProceduralSkyPass Initialize 是否成功(m_psoReady)
- 无平面:检查 OpaquePass Execute / 场景加载
- shader 绑定:SetConstantData(0) 对应 binding 0,若不匹配调整 shader layout

**Step 5: Final commit**

```bash
git add -A
git commit -m "feat(mobilelumen): minimal rendering pipeline complete"
```

---

## 不做(YAGNI 边界)

- MLGI 实际功能(UpdateGI 保持空 hook)
- 修改 cornell_box 场景
- 修改 skybox.frag / SkyboxPass
- 透明物体 pass / 阴影 / IBL / 后处理
- TextureRenderTargetProxy 提取到共享头(后续重构)
- 动其他 5 主题本地改动
