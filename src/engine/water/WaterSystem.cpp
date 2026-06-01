#include "water/WaterSystem.h"
#include "Logger.h"
#include "Engine.h"
#include "SceneManager.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IPipeline.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/ICamera.h"
#include "graphic/RenderDesc.h"
#include "graphic/interfaces/ISwapChain.h"
#include <algorithm>

namespace Prisma::Water {

// ============================================================================
// MirrorCameraAdapter — temporary ICamera impl wrapping cached matrices
// Used by CaptureReflection / CaptureRefraction to render scene into textures
// ============================================================================

class MirrorCameraAdapter : public Graphic::ICamera {
public:
    void SetMatrices(const Matrix4x4& view, const Matrix4x4& proj, const Vector3& pos) {
        m_ViewMatrix  = view;
        m_ProjMatrix  = proj;
        m_Position    = pos;
            m_Forward     = Vector3(-view[0][2], -view[1][2], -view[2][2]);
        m_Up          = Vector3(view[0][1],  view[1][1],  view[2][1]);
    }

    PrismaMath::mat4 GetViewMatrix()            const override { return m_ViewMatrix; }
    PrismaMath::mat4 GetProjectionMatrix()      const override { return m_ProjMatrix; }
    PrismaMath::mat4 GetViewProjectionMatrix()  const override { return m_ProjMatrix * m_ViewMatrix; }
    PrismaMath::vec3 GetPosition()              const override { return m_Position; }
    PrismaMath::vec3 GetForward()               const override { return m_Forward; }
    PrismaMath::vec3 GetUp()                    const override { return m_Up; }
    PrismaMath::vec3 GetRight()                 const override {
        return glm::normalize(glm::cross(m_Forward, m_Up));
    }
    float GetFOV()                              const override { return 70.0f * DEG_TO_RAD; }
    float GetNearPlane()                        const override { return 0.1f; }
    float GetFarPlane()                         const override { return 1000.0f; }
    float GetAspectRatio()                      const override { return 1.0f; }
    void  SetFOV(float)                               override {}
    void  SetNearFarPlanes(float, float)               override {}
    void  SetAspectRatio(float)                        override {}
    void  SetViewport(uint32_t, uint32_t)              override {}
    void  Update(Timestep)                             override {}
    bool  IsActive() const                             override { return false; }
    void  SetActive(bool)                              override {}
    PrismaMath::vec4 GetClearColor()            const override { return {0.1f, 0.1f, 0.3f, 1.0f}; }
    void  SetClearColor(float, float, float, float)    override {}

private:
    Matrix4x4 m_ViewMatrix = Matrix4x4(1.0f);
    Matrix4x4 m_ProjMatrix = Matrix4x4(1.0f);
    Vector3 m_Position     = Vector3(0.0f);
    Vector3 m_Forward      = Vector3(0.0f, 0.0f, -1.0f);
    Vector3 m_Up           = Vector3(0.0f, 1.0f, 0.0f);
};

// ============================================================================
// 构造 / 析构
// ============================================================================

WaterSystem::WaterSystem()
{
    m_Mesh = std::make_unique<WaterMesh>();
}

WaterSystem::~WaterSystem()
{
    Shutdown();
}

// ============================================================================
// ISubSystem 接口
// ============================================================================

int WaterSystem::Initialize()
{
    if (m_Initialized) return 0;

    LOG_INFO("Water", "Water system initializing...");

    InitMesh();
    InitWaves();
    CreateFrameBuffers();
    CreateWaterResources();

    // Register water render callback with the render system
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (renderSystem) {
        renderSystem->SetWaterRenderCallback(
            [this](Graphic::ICommandBuffer* cmd, Graphic::IRenderDevice* device) {
                RenderWater(cmd, device);
            }
        );
        m_CallbackRegistered = true;
        LOG_INFO("Water", "Water render callback registered");
    }

    m_Initialized = true;
    LOG_INFO("Water", "Water system initialized");
    return 0;
}

void WaterSystem::Shutdown()
{
    if (!m_Initialized) return;

    LOG_INFO("Water", "Water system shutting down...");

    // Unregister callback
    if (m_CallbackRegistered) {
        auto& engine = Engine::Get();
        auto* renderSystem = engine.GetRenderSystem();
        if (renderSystem) {
            renderSystem->SetWaterRenderCallback(nullptr);
        }
        m_CallbackRegistered = false;
    }

    // Release GPU resources (reverse order of creation)
    m_WaveSSBO.reset();
    m_WaterUBO.reset();
    m_CameraUBO.reset();

    m_ReflectionCubeTex.reset();
    m_ReflectionDepthTex.reset();
    m_ReflectionColorTex.reset();
    m_RefractionDepthTex.reset();
    m_RefractionColorTex.reset();

    m_DescriptorSet.reset();
    m_DescriptorSetLayout.reset();
    m_ReflectionSampler.reset();
    m_LinearSampler.reset();

    m_IndexBuffer.reset();
    m_VertexBuffer.reset();
    m_WaterPipeline.reset();
    m_WaterFragShader.reset();
    m_WaterVertShader.reset();
    m_SceneDepthTexture.reset();

    m_Mesh->Clear();
    m_Interaction.Clear();

    m_Initialized = false;
    LOG_INFO("Water", "Water system shut down");
}

void WaterSystem::Update(Timestep ts)
{
    if (!m_Initialized || !m_Enabled) return;

    float dt = ts.GetSeconds();
    if (dt <= 0.0f || dt > 0.1f) dt = 0.1f;

    auto& engine = Engine::Get();
    auto* sceneManager = engine.GetSceneManager();
    if (sceneManager) {
        auto* scene = sceneManager->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) {
                m_CameraPosition = camera->GetPosition();
                m_CameraForward  = camera->GetForward();
                m_CameraUp       = camera->GetUp();
                m_CameraFOV      = camera->GetFOV();
                m_CameraAspect   = camera->GetAspectRatio();
            }
        }
    }

    m_WaveSimulation.AdvanceTime(dt);

    if (m_Config.enableInteraction) {
        m_Interaction.Update(dt, m_Config.rippleSpeed, m_Config.rippleDecay);
    }

    m_Mesh->Generate(
        m_CameraPosition,
        m_Config.render.tileSize,
        m_Config.render.baseSegments,
        m_Config.render.farSegments,
        m_Config.render.lodDistance
    );

    CaptureReflection();
    CaptureRefraction();

}

void WaterSystem::SpawnRipple(const Vector3& position, float strength)
{
    if (!m_Config.enableInteraction) return;
    m_Interaction.SpawnRipple(position, strength);
}

float WaterSystem::GetWaveHeight(const Vector2& worldPos) const
{
    WaveDisplacement displacement = m_WaveSimulation.Evaluate(worldPos);
    float height = displacement.position.y;

    if (m_Config.enableInteraction) {
        height += m_Interaction.GetDisplacement(worldPos);
    }

    return m_Config.render.waterLevel + height * m_Config.render.waveHeightScale;
}

Vector3 WaterSystem::GetWaveDisplacement(const Vector2& worldPos) const
{
    WaveDisplacement displacement = m_WaveSimulation.Evaluate(worldPos);

    Vector3 result;
    result.x = displacement.position.x;
    result.y = m_Config.render.waterLevel + displacement.position.y * m_Config.render.waveHeightScale;
    result.z = displacement.position.z;

    if (m_Config.enableInteraction) {
        result.y += m_Interaction.GetDisplacement(worldPos);
    }

    return result;
}

// ============================================================================
// Internal Setup Methods
// ============================================================================

void WaterSystem::InitMesh()
{
    m_Mesh->Generate(
        Vector3(0.0f),
        m_Config.render.tileSize,
        m_Config.render.baseSegments,
        m_Config.render.farSegments,
        m_Config.render.lodDistance
    );
    LOG_INFO("Water", "Water mesh created: {} vertices, {} triangles",
             m_Mesh->GetVertexCount(), m_Mesh->GetTriangleCount());
}

void WaterSystem::InitWaves()
{
    m_WaveSimulation.SetGerstnerWaves(m_Config.gerstnerWaves);
    m_WaveSimulation.SetUseFFT(m_Config.useFFT);
    if (m_Config.useFFT) {
        m_WaveSimulation.SetFFTConfig(m_Config.fft);
        LOG_INFO("Water", "FFT wave simulation enabled (resolution: {})",
                 m_Config.fft.resolution);
    }
    LOG_INFO("Water", "Wave simulation initialized with {} Gerstner waves",
             m_WaveSimulation.GetWaveCount());
}

void WaterSystem::CreateFrameBuffers()
{
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* device = renderSystem->GetDevice();
    if (!device) return;

    m_ReflectionResolution = 512;
    float aspect = (m_CameraAspect > 0.01f) ? m_CameraAspect : 16.0f / 9.0f;
    m_RefractionResolution = static_cast<uint32_t>(
        static_cast<float>(m_ReflectionResolution) / aspect);

    LOG_INFO("Water", "Reflection/refraction framebuffers: {}x{}, {}x{}",
             m_ReflectionResolution, m_ReflectionResolution,
             m_RefractionResolution, m_RefractionResolution);
}

// ============================================================================
// GPU Resource Creation Helpers
// ============================================================================

void WaterSystem::CreateWaterResources()
{
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) {
        LOG_WARNING("Water", "Render system unavailable, skipping GPU resource creation");
        return;
    }
    auto* device = renderSystem->GetDevice();
    if (!device) {
        LOG_WARNING("Water", "Render device unavailable, skipping GPU resource creation");
        return;
    }
    auto* factory = device->GetResourceFactory();
    if (!factory) {
        LOG_WARNING("Water", "Resource factory unavailable, skipping GPU resource creation");
        return;
    }

    // Camera UBO (binding 0)
    {
        Graphic::BufferDesc desc;
        desc.type = Graphic::BufferType::Constant;
        desc.size = sizeof(CameraUniforms);
        desc.usage = Graphic::BufferUsage::Dynamic;
        desc.name = "WaterCameraUBO";
        m_CameraUBO = std::shared_ptr<Graphic::IBuffer>(factory->CreateBufferImpl(desc));
        if (!m_CameraUBO) {
            LOG_ERROR("Water", "Failed to create camera UBO");
            return;
        }
    }

    // Water params UBO (binding 1)
    {
        Graphic::BufferDesc desc;
        desc.type = Graphic::BufferType::Constant;
        desc.size = sizeof(WaterUniforms);
        desc.usage = Graphic::BufferUsage::Dynamic;
        desc.name = "WaterParamsUBO";
        m_WaterUBO = std::shared_ptr<Graphic::IBuffer>(factory->CreateBufferImpl(desc));
        if (!m_WaterUBO) {
            LOG_ERROR("Water", "Failed to create water UBO");
            return;
        }
    }

    // Wave SSBO (binding 2) — 32 vec4 = 512 bytes
    {
        size_t ssboSize = 32 * sizeof(Vector4);
        Graphic::BufferDesc desc;
        desc.type  = Graphic::BufferType::Structured;
        desc.size  = ssboSize;
        desc.usage = Graphic::BufferUsage::Dynamic;
        desc.stride = static_cast<uint32_t>(sizeof(Vector4));
        desc.name  = "WaterWaveSSBO";
        m_WaveSSBO = std::shared_ptr<Graphic::IBuffer>(factory->CreateBufferImpl(desc));
        if (!m_WaveSSBO) {
            LOG_ERROR("Water", "Failed to create wave SSBO");
            return;
        }
    }

    // Samplers
    {
        Graphic::SamplerDesc linearDesc;
        linearDesc.filter    = Graphic::TextureFilter::Linear;
        linearDesc.addressU  = Graphic::TextureAddressMode::Clamp;
        linearDesc.addressV  = Graphic::TextureAddressMode::Clamp;
        linearDesc.addressW  = Graphic::TextureAddressMode::Clamp;
        m_LinearSampler = std::shared_ptr<Graphic::ISampler>(
            factory->CreateSamplerImpl(linearDesc));
        if (!m_LinearSampler) {
            LOG_ERROR("Water", "Failed to create linear sampler");
            return;
        }

        Graphic::SamplerDesc reflectDesc;
        reflectDesc.filter   = Graphic::TextureFilter::Linear;
        reflectDesc.addressU = Graphic::TextureAddressMode::Mirror;
        reflectDesc.addressV = Graphic::TextureAddressMode::Mirror;
        reflectDesc.addressW = Graphic::TextureAddressMode::Mirror;
        m_ReflectionSampler = std::shared_ptr<Graphic::ISampler>(
            factory->CreateSamplerImpl(reflectDesc));
        if (!m_ReflectionSampler) {
            LOG_ERROR("Water", "Failed to create reflection sampler");
            return;
        }
    }

    CreateReflectionTextures();
    CreateRefractionTextures();
    SetupDescriptorSet();
    UploadWaveData();

    LOG_INFO("Water", "GPU resources created successfully");
}

void WaterSystem::CreateReflectionTextures()
{
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;
    auto* device = renderSystem->GetDevice();
    if (!device) return;
    auto* factory = device->GetResourceFactory();
    if (!factory) return;

    uint32_t res = m_ReflectionResolution;
    if (res == 0) res = 512;

    // Color render target
    {
        Graphic::TextureDesc desc;
        desc.type              = Graphic::TextureType::Texture2D;
        desc.format            = Graphic::TextureFormat::RGBA8_UNorm;
        desc.width             = res;
        desc.height            = res;
        desc.allowRenderTarget = true;
        desc.allowShaderResource = true;
        desc.name              = "WaterReflectionColor";
        m_ReflectionColorTex = std::shared_ptr<Graphic::ITexture>(
            factory->CreateTextureImpl(desc));
        if (!m_ReflectionColorTex) {
            LOG_ERROR("Water", "Failed to create reflection color texture");
            return;
        }
    }

    {
        Graphic::TextureDesc desc;
        desc.type               = Graphic::TextureType::Texture2D;
        desc.format             = Graphic::TextureFormat::D32_Float;
        desc.width              = res;
        desc.height             = res;
        desc.allowDepthStencil  = true;
        desc.name               = "WaterReflectionDepth";
        m_ReflectionDepthTex = std::shared_ptr<Graphic::ITexture>(
            factory->CreateTextureImpl(desc));
        if (!m_ReflectionDepthTex) {
            LOG_ERROR("Water", "Failed to create reflection depth texture");
            return;
        }
    }

    // Reflection cubemap (used by water.frag at binding 3 as samplerCube)
    // 6 faces wide, each face = res x res
    {
        Graphic::TextureDesc desc;
        desc.type              = Graphic::TextureType::TextureCube;
        desc.format            = Graphic::TextureFormat::RGBA8_UNorm;
        desc.width             = res;
        desc.height            = res;
        desc.arraySize         = 6;
        desc.allowRenderTarget = true;
        desc.allowShaderResource = true;
        desc.name              = "WaterReflectionCube";
        m_ReflectionCubeTex = std::shared_ptr<Graphic::ITexture>(
            factory->CreateTextureImpl(desc));
        if (!m_ReflectionCubeTex) {
            LOG_ERROR("Water", "Failed to create reflection cubemap");
            return;
        }
    }

    LOG_INFO("Water", "Reflection textures created: {}x{}", res, res);
}

void WaterSystem::CreateRefractionTextures()
{
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;
    auto* device = renderSystem->GetDevice();
    if (!device) return;
    auto* factory = device->GetResourceFactory();
    if (!factory) return;

    uint32_t res = m_RefractionResolution;
    if (res == 0) res = 512;

    // Refraction color render target
    {
        Graphic::TextureDesc desc;
        desc.type              = Graphic::TextureType::Texture2D;
        desc.format            = Graphic::TextureFormat::RGBA8_UNorm;
        desc.width             = res;
        desc.height            = res;
        desc.allowRenderTarget = true;
        desc.allowShaderResource = true;
        desc.name              = "WaterRefractionColor";
        m_RefractionColorTex = std::shared_ptr<Graphic::ITexture>(
            factory->CreateTextureImpl(desc));
        if (!m_RefractionColorTex) {
            LOG_ERROR("Water", "Failed to create refraction color texture");
            return;
        }
    }

    // Refraction depth buffer
    {
        Graphic::TextureDesc desc;
        desc.type               = Graphic::TextureType::Texture2D;
        desc.format             = Graphic::TextureFormat::D32_Float;
        desc.width              = res;
        desc.height             = res;
        desc.allowDepthStencil  = true;
        desc.name               = "WaterRefractionDepth";
        m_RefractionDepthTex = std::shared_ptr<Graphic::ITexture>(
            factory->CreateTextureImpl(desc));
        if (!m_RefractionDepthTex) {
            LOG_ERROR("Water", "Failed to create refraction depth texture");
            return;
        }
    }

    LOG_INFO("Water", "Refraction textures created: {}x{}", res, res);
}

void WaterSystem::SetupDescriptorSet()
{
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;
    auto* device = renderSystem->GetDevice();
    if (!device) return;
    auto* factory = device->GetResourceFactory();
    if (!factory) return;

    // ========================================================================
    //  Descriptor set layout (set 0):
    //    Binding 0: CameraUniforms   (UniformBuffer)
    //    Binding 1: WaterUniforms    (UniformBuffer)
    //    Binding 2: GerstnerBuffer   (StorageBuffer)
    //    Binding 3: u_ReflectionMap  (SamplerCube)
    //    Binding 4: u_RefractionMap  (Sampler2D)
    //    Binding 5: u_DepthMap       (Sampler2D)
    // ========================================================================
    std::vector<Graphic::ShaderResource> resources;

    {
        Graphic::ShaderResource res;
        res.Name         = "CameraUniforms";
        res.ResourceType = Graphic::ShaderResource::Type::UniformBuffer;
        res.Set          = 0;
        res.Binding      = 0;
        res.Size         = static_cast<uint32_t>(sizeof(CameraUniforms));
        resources.push_back(res);
    }
    {
        Graphic::ShaderResource res;
        res.Name         = "WaterUniforms";
        res.ResourceType = Graphic::ShaderResource::Type::UniformBuffer;
        res.Set          = 0;
        res.Binding      = 1;
        res.Size         = static_cast<uint32_t>(sizeof(WaterUniforms));
        resources.push_back(res);
    }
    {
        Graphic::ShaderResource res;
        res.Name         = "GerstnerBuffer";
        res.ResourceType = Graphic::ShaderResource::Type::StorageBuffer;
        res.Set          = 0;
        res.Binding      = 2;
        res.Size         = static_cast<uint32_t>(32 * sizeof(Vector4));
        resources.push_back(res);
    }
    {
        Graphic::ShaderResource res;
        res.Name         = "u_ReflectionMap";
        res.ResourceType = Graphic::ShaderResource::Type::SamplerCube;
        res.Set          = 0;
        res.Binding      = 3;
        resources.push_back(res);
    }
    {
        Graphic::ShaderResource res;
        res.Name         = "u_RefractionMap";
        res.ResourceType = Graphic::ShaderResource::Type::Sampler2D;
        res.Set          = 0;
        res.Binding      = 4;
        resources.push_back(res);
    }
    {
        Graphic::ShaderResource res;
        res.Name         = "u_DepthMap";
        res.ResourceType = Graphic::ShaderResource::Type::Sampler2D;
        res.Set          = 0;
        res.Binding      = 5;
        resources.push_back(res);
    }

    m_DescriptorSetLayout = factory->CreateDescriptorSetLayout(resources);
    if (!m_DescriptorSetLayout) {
        LOG_ERROR("Water", "Failed to create descriptor set layout");
        return;
    }

    m_DescriptorSet = factory->CreateDescriptorSet(m_DescriptorSetLayout.get());
    if (!m_DescriptorSet) {
        LOG_ERROR("Water", "Failed to create descriptor set");
        return;
    }

    if (m_CameraUBO) {
        m_DescriptorSet->BindBuffer(0, m_CameraUBO.get(), 0,
            sizeof(CameraUniforms), Graphic::DescriptorType::UniformBuffer);
    }
    if (m_WaterUBO) {
        m_DescriptorSet->BindBuffer(1, m_WaterUBO.get(), 0,
            sizeof(WaterUniforms), Graphic::DescriptorType::UniformBuffer);
    }
    if (m_WaveSSBO) {
        m_DescriptorSet->BindBuffer(2, m_WaveSSBO.get(), 0,
            32 * sizeof(Vector4), Graphic::DescriptorType::StorageBuffer);
    }

    if (m_ReflectionCubeTex && m_ReflectionSampler) {
        m_DescriptorSet->BindTexture(3, m_ReflectionCubeTex.get(), m_ReflectionSampler.get());
    }
    if (m_RefractionColorTex && m_LinearSampler) {
        m_DescriptorSet->BindTexture(4, m_RefractionColorTex.get(), m_LinearSampler.get());
    }
    if (m_SceneDepthTexture && m_LinearSampler) {
        m_DescriptorSet->BindTexture(5, m_SceneDepthTexture.get(), m_LinearSampler.get());
    }

    m_DescriptorSet->Update();

    LOG_INFO("Water", "Descriptor set created and bound (6 bindings)");
}

void WaterSystem::UploadWaveData()
{
    if (!m_WaveSSBO) return;

    const auto& waves = m_WaveSimulation.GetWaves();
    size_t waveCount = std::min(waves.size(), size_t(16));

    Vector4 waveData[16];
    Vector4 wavePhase[16];

    for (int i = 0; i < 16; ++i) {
        waveData[i]  = Vector4(0.0f);
        wavePhase[i] = Vector4(0.0f);
    }

    for (size_t i = 0; i < waveCount; ++i) {
        const auto& w = waves[i];
        Vector2 dir = glm::normalize(w.direction);
        waveData[i]  = Vector4(dir.x, dir.y, w.amplitude, w.frequency);
        wavePhase[i] = Vector4(w.speed, w.steepness, 0.0f, 0.0f);
    }

    m_WaveSSBO->UpdateData(waveData, sizeof(Vector4) * 16, 0);
    m_WaveSSBO->UpdateData(wavePhase, sizeof(Vector4) * 16, sizeof(Vector4) * 16);

    LOG_DEBUG("Water", "Uploaded {} / {} Gerstner waves to GPU SSBO",
              waveCount, size_t(16));
}

// ============================================================================
// Camera Construction
// ============================================================================

void WaterSystem::BuildReflectionCamera(Matrix4x4& outView, Matrix4x4& outProj,
                                         Vector3& outPos) const
{
    float waterLevel = m_Config.render.waterLevel;

    float distToWater = m_CameraPosition.y - waterLevel;
    outPos = m_CameraPosition;
    outPos.y = waterLevel - distToWater;

    Vector3 reflectedForward = Vector3(m_CameraForward.x, -m_CameraForward.y, m_CameraForward.z);
    Vector3 reflectedUp      = Vector3(m_CameraUp.x,      -m_CameraUp.y,      m_CameraUp.z);
    reflectedForward = glm::normalize(reflectedForward);
    reflectedUp      = glm::normalize(reflectedUp);

    outView = Prisma::Math::LookAt(outPos, outPos + reflectedForward, reflectedUp);

    float nearPlane = 0.1f;
    float farPlane  = 1000.0f;
    outProj = Prisma::Math::Perspective(m_CameraFOV, m_CameraAspect, nearPlane, farPlane);
}

void WaterSystem::BuildRefractionCamera(Matrix4x4& outView, Matrix4x4& outProj,
                                          Vector3& outPos) const
{
    float waterLevel = m_Config.render.waterLevel;

    outPos = m_CameraPosition;
    if (outPos.y < waterLevel + 0.1f) {
        outPos.y = waterLevel + 0.1f;
    }

    Vector3 lookTarget = outPos + m_CameraForward;
    lookTarget.y = waterLevel - 1.0f;

    outView = Prisma::Math::LookAt(outPos, lookTarget, m_CameraUp);

    float nearPlane = 0.1f;
    float farPlane  = 1000.0f;
    outProj = Prisma::Math::Perspective(m_CameraFOV, m_CameraAspect, nearPlane, farPlane);
}

// ============================================================================
// Reflection / Refraction Capture
// ============================================================================

void WaterSystem::CaptureReflection()
{
    BuildReflectionCamera(m_ReflectionView, m_ReflectionProj, m_ReflectionPos);
    m_ReflectionValid = true;

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* sceneManager = engine.GetSceneManager();
    if (!sceneManager) return;

    auto* scene = sceneManager->GetCurrentScene();
    if (!scene || !m_ReflectionColorTex) return;

    MirrorCameraAdapter mirrorCam;
    mirrorCam.SetMatrices(m_ReflectionView, m_ReflectionProj, m_ReflectionPos);

    renderSystem->RenderScene(scene, &mirrorCam, m_ReflectionColorTex.get());

    if (m_ReflectionCubeTex && m_ReflectionColorTex) {
        for (uint32_t face = 0; face < 6; ++face) {
            m_ReflectionCubeTex->CopyFrom(
                m_ReflectionColorTex.get(), 0, 0, 0, face);
        }
    }
}

void WaterSystem::CaptureRefraction()
{
    BuildRefractionCamera(m_RefractionView, m_RefractionProj, m_RefractionPos);
    m_RefractionValid = true;

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* sceneManager = engine.GetSceneManager();
    if (!sceneManager) return;

    auto* scene = sceneManager->GetCurrentScene();
    if (!scene || !m_RefractionColorTex) return;

    MirrorCameraAdapter refractCam;
    refractCam.SetMatrices(m_RefractionView, m_RefractionProj, m_RefractionPos);

    renderSystem->RenderScene(scene, &refractCam, m_RefractionColorTex.get());
}

// ============================================================================
// Render callback registered with RenderSystem
// ============================================================================

void WaterSystem::RenderWater(Graphic::ICommandBuffer* cmd,
                               Graphic::IRenderDevice* device)
{
    if (!cmd || !device) return;
    if (!m_Mesh || !m_Mesh->IsValid()) return;

    // Ensure all GPU resources are ready
    if (!EnsureWaterPipeline(device)) return;
    if (!EnsureWaterDescriptorSet(device)) return;
    if (!EnsureWaterMeshBuffers(device)) return;

    if (m_CameraUBO) {
        CameraUniforms camData{};

        auto& engine = Engine::Get();
        auto* sceneManager = engine.GetSceneManager();
        if (sceneManager) {
            auto* scene = sceneManager->GetCurrentScene();
            if (scene) {
                auto camera = scene->GetMainCamera();
                if (camera) {
                    auto viewMat = camera->GetViewMatrix();
                    auto projMat = camera->GetProjectionMatrix();
                    auto vpMat   = projMat * viewMat;

                    memcpy(&camData.viewProjection, &vpMat,    sizeof(Matrix4x4));
                    memcpy(&camData.view,           &viewMat,  sizeof(Matrix4x4));
                    memcpy(&camData.projection,     &projMat,  sizeof(Matrix4x4));

                    Vector3 camPos = camera->GetPosition();
                    camData.cameraPos = Vector4(camPos.x, camPos.y, camPos.z,
                                                camera->GetNearPlane());

                    Vector3 camFwd = camera->GetForward();
                    camData.cameraForward = Vector4(camFwd.x, camFwd.y, camFwd.z,
                                                    camera->GetFarPlane());
                }
            }
        }

        m_CameraUBO->UpdateData(&camData, sizeof(CameraUniforms), 0);
    }

    if (m_WaterUBO) {
        WaterUniforms waterData{};
        waterData.deepColor      = Vector4(m_Config.render.deepColor.x,
                                           m_Config.render.deepColor.y,
                                           m_Config.render.deepColor.z, 0.0f);
        waterData.shallowColor   = Vector4(m_Config.render.shallowColor.x,
                                           m_Config.render.shallowColor.y,
                                           m_Config.render.shallowColor.z, 0.0f);
        waterData.fogColor       = Vector4(m_Config.render.fogColor.x,
                                           m_Config.render.fogColor.y,
                                           m_Config.render.fogColor.z,
                                           m_Config.render.fogDensity);
        waterData.waterLevel     = m_Config.render.waterLevel;
        waterData.waveHeightScale = m_Config.render.waveHeightScale;
        waterData.fresnelPower   = m_Config.render.fresnelPower;
        waterData.fresnelStrength = m_Config.render.fresnelStrength;
        waterData.specularStrength = m_Config.render.specularStrength;
        waterData.specularPower  = m_Config.render.specularPower;
        waterData.transparency   = m_Config.render.transparency;
        waterData.refractionScale = m_Config.render.refractionScale;
        waterData.sunGlowStrength = m_Config.render.sunGlowStrength;
        waterData.sunGlowPower   = m_Config.render.sunGlowPower;
        waterData.time           = m_WaveSimulation.GetTime();
        waterData.fogDensity     = m_Config.render.fogDensity;
        waterData.shallowDepth   = m_Config.render.shallowDepth;
        waterData.deepDepth      = m_Config.render.deepDepth;
        waterData._padding[0]    = 0.0f;
        waterData._padding[1]    = 0.0f;

        m_WaterUBO->UpdateData(&waterData, sizeof(WaterUniforms), 0);
    }

    if (m_DescriptorSet) {
        if (m_SceneDepthTexture && m_LinearSampler) {
            m_DescriptorSet->BindTexture(5, m_SceneDepthTexture.get(), m_LinearSampler.get());
        }
        m_DescriptorSet->Update();
    }

    cmd->SetPipelineState(m_WaterPipeline.get());

    Graphic::Viewport vp;
    vp.x = 0.0f;
    vp.y = 0.0f;
    vp.width  = 1600.0f;
    vp.height = 900.0f;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    if (device->GetSwapChain()) {
        vp.width  = static_cast<float>(device->GetSwapChain()->GetWidth());
        vp.height = static_cast<float>(device->GetSwapChain()->GetHeight());
    }
    cmd->SetViewport(vp);

    Graphic::Rect scissor;
    scissor.x      = 0;
    scissor.y      = 0;
    scissor.width  = static_cast<int>(vp.width);
    scissor.height = static_cast<int>(vp.height);
    cmd->SetScissorRect(scissor);

    if (m_DescriptorSet) {
        cmd->BindDescriptorSet(0, m_DescriptorSet.get());
    }

    if (m_VertexBuffer) {
        cmd->SetVertexBuffer(m_VertexBuffer.get(), 0);
    }
    if (m_IndexBuffer) {
        cmd->SetIndexBuffer(m_IndexBuffer.get(), true);
    }

    uint32_t indexCount = static_cast<uint32_t>(m_Mesh->GetIndexCount());
    if (indexCount > 0) {
        cmd->DrawIndexed(indexCount);
    }
}

bool WaterSystem::EnsureWaterPipeline(Graphic::IRenderDevice* device)
{
    if (m_WaterPipeline && m_WaterPipeline->IsValid()) return true;

    auto* factory = device->GetResourceFactory();
    if (!factory) return false;

    auto& engine = Engine::Get();
    auto* resourceManager = engine.GetRenderResourceManager();
    if (!resourceManager) return false;

    m_WaterVertShader = resourceManager->LoadShaderSync(
        "assets/shaders/water.vert.spv", "main");
    if (!m_WaterVertShader) {
        LOG_ERROR("Water", "Failed to load vertex shader: assets/shaders/water.vert.spv");
        return false;
    }

    m_WaterFragShader = resourceManager->LoadShaderSync(
        "assets/shaders/water.frag.spv", "main");
    if (!m_WaterFragShader) {
        LOG_ERROR("Water", "Failed to load fragment shader: assets/shaders/water.frag.spv");
        return false;
    }

    auto pso = factory->CreatePipelineStateImpl();
    if (!pso) {
        LOG_ERROR("Water", "Failed to create PSO object");
        return false;
    }

    pso->SetShader(Graphic::ShaderType::Vertex, m_WaterVertShader);
    pso->SetShader(Graphic::ShaderType::Pixel,  m_WaterFragShader);
    pso->SetPrimitiveTopology(Graphic::PrimitiveTopology::TriangleList);

    std::vector<Graphic::VertexInputAttribute> attrs;
    {
        Graphic::VertexInputAttribute a;
        a.semanticName     = "POSITION";
        a.semanticIndex    = 0;
        a.format           = Graphic::TextureFormat::RGB32_Float;
        a.inputSlot        = 0;
        a.alignedByteOffset = 0;
        attrs.push_back(a);
    }
    {
        Graphic::VertexInputAttribute a;
        a.semanticName     = "NORMAL";
        a.semanticIndex    = 0;
        a.format           = Graphic::TextureFormat::RGB32_Float;
        a.inputSlot        = 0;
        a.alignedByteOffset = 12;
        attrs.push_back(a);
    }
    {
        Graphic::VertexInputAttribute a;
        a.semanticName     = "TEXCOORD";
        a.semanticIndex    = 0;
        a.format           = Graphic::TextureFormat::RG32_Float;
        a.inputSlot        = 0;
        a.alignedByteOffset = 24;
        attrs.push_back(a);
    }
    {
        Graphic::VertexInputAttribute a;
        a.semanticName     = "TANGENT";
        a.semanticIndex    = 0;
        a.format           = Graphic::TextureFormat::RGB32_Float;
        a.inputSlot        = 0;
        a.alignedByteOffset = 32;
        attrs.push_back(a);
    }
    pso->SetInputLayout(attrs);

    Graphic::RasterizerState rs;
    rs.cullMode = Graphic::CullMode::None;
    rs.frontCounterClockwise = true;
    rs.fillMode = Graphic::FillMode::Solid;
    pso->SetRasterizerState(rs);

    Graphic::DepthStencilState ds{};
    ds.depthEnable      = true;
    ds.depthWriteEnable = false;
    ds.depthFunc        = Graphic::ComparisonFunc::LessEqual;
    pso->SetDepthStencilState(ds);

    Graphic::BlendState blend{};
    blend.blendEnable     = true;
    blend.srcBlend        = Graphic::BlendFactorType::SrcAlpha;
    blend.destBlend       = Graphic::BlendFactorType::InvSrcAlpha;
    blend.blendOp         = Graphic::BlendOp::Add;
    blend.srcBlendAlpha   = Graphic::BlendFactorType::One;
    blend.destBlendAlpha  = Graphic::BlendFactorType::Zero;
    blend.blendOpAlpha    = Graphic::BlendOp::Add;
    pso->SetBlendState(blend);

    if (!pso->Create(device)) {
        LOG_ERROR("Water", "Failed to create water pipeline: {}", pso->GetErrors());
        return false;
    }

    m_WaterPipeline = std::shared_ptr<Graphic::IPipelineState>(std::move(pso));
    LOG_INFO("Water", "Water rendering pipeline created");
    return true;
}

bool WaterSystem::EnsureWaterDescriptorSet(Graphic::IRenderDevice* device)
{
    if (m_DescriptorSet) return true;

    SetupDescriptorSet();
    return m_DescriptorSet != nullptr;
}

bool WaterSystem::EnsureWaterMeshBuffers(Graphic::IRenderDevice* device)
{
    if (!m_Mesh || !m_Mesh->IsValid()) return false;

    auto* factory = device->GetResourceFactory();
    if (!factory) return false;

    const auto& vertices = m_Mesh->GetVertices();
    const auto& indices  = m_Mesh->GetIndices();

    size_t vertexCount = vertices.size();
    size_t indexCount  = indices.size();

    bool needsUpdate = false;
    if (!m_VertexBuffer || m_LastVertexCount != vertexCount) needsUpdate = true;
    if (!m_IndexBuffer  || m_LastIndexCount  != indexCount)  needsUpdate = true;
    if (!needsUpdate) return true;

    // Vertex buffer
    if (vertexCount > 0) {
        size_t vbSize = vertexCount * sizeof(WaterVertex);
        Graphic::BufferDesc desc;
        desc.type        = Graphic::BufferType::Vertex;
        desc.size        = vbSize;
        desc.usage       = Graphic::BufferUsage::Immutable;
        desc.initialData = vertices.data();
        desc.stride      = sizeof(WaterVertex);
        desc.name        = "WaterVertexBuffer";
        m_VertexBuffer   = std::shared_ptr<Graphic::IBuffer>(
            factory->CreateBufferImpl(desc));
        if (!m_VertexBuffer) {
            LOG_ERROR("Water", "Failed to create vertex buffer ({} bytes)", vbSize);
            return false;
        }
    }

    // Index buffer
    if (indexCount > 0) {
        size_t ibSize = indexCount * sizeof(WaterIndex);
        Graphic::BufferDesc desc;
        desc.type        = Graphic::BufferType::Index;
        desc.size        = ibSize;
        desc.usage       = Graphic::BufferUsage::Immutable;
        desc.initialData = indices.data();
        desc.name        = "WaterIndexBuffer";
        m_IndexBuffer    = std::shared_ptr<Graphic::IBuffer>(
            factory->CreateBufferImpl(desc));
        if (!m_IndexBuffer) {
            LOG_ERROR("Water", "Failed to create index buffer ({} bytes)", ibSize);
            m_VertexBuffer.reset();
            return false;
        }
    }

    m_LastVertexCount = vertexCount;
    m_LastIndexCount  = indexCount;

    LOG_DEBUG("Water", "Mesh GPU buffers updated: {} vertices, {} indices",
              vertexCount, indexCount);
    return true;
}

// ============================================================================
// Forwards to active command buffer (legacy entry point)
// ============================================================================

void WaterSystem::SubmitDrawCalls()
{
    if (!m_Mesh || !m_Mesh->IsValid()) return;

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* device = renderSystem->GetDevice();
    if (!device) return;

    auto* cmd = device->GetCurrentCommandBuffer();
    if (!cmd) return;

    RenderWater(cmd, device);
}

} // namespace Prisma::Water
