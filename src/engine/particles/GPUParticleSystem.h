#pragma once

#include "math/MathTypes.h"
#include "graphic/interfaces/RenderTypes.h"
#include "graphic/ICamera.h"
#include "Export.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace Prisma::Graphic {
class IBuffer;
class IComputePipeline;
class IGraphicsPipeline;
class IRenderDevice;
class IDescriptorSet;
class IDescriptorSetLayout;
class IShader;
class ICommandBuffer;
} // namespace Prisma::Graphic

namespace Prisma::Particles {

// GPU 粒子数据布局（匹配 compute shader 的 SSBO 布局）
// 每个粒子 48 字节（4 个 vec4）
struct GPUParticleData {
    Vector4 position;  // xyz = position, w = size
    Vector4 velocity;  // xyz = velocity, w = rotation
    Vector4 color;     // rgba
    Vector4 life;      // x = age, y = lifetime, z = padding, w = padding
};

// 间接绘制参数（匹配 VkDrawIndirectCommand）
struct ParticleDrawIndirect {
    uint32_t vertexCount   = 0;  // 每个粒子的顶点数（4 个顶点构成一个 quad）
    uint32_t instanceCount = 0;  // 存活粒子数
    uint32_t firstVertex   = 0;
    uint32_t firstInstance = 0;
};

// GPU 粒子系统 — 通过 compute shader 更新，SSBO 存储
class ENGINE_API GPUParticleSystem {
public:
    GPUParticleSystem();
    ~GPUParticleSystem();

    // 初始化 GPU 资源（需要渲染设备就绪后调用）
    bool Initialize(Graphic::IRenderDevice* device, uint32_t maxParticles = 65536);
    void Shutdown();

    // 每帧更新（发射新粒子 + dispatch compute shader）
    void UpdateParticles(float dt, const Vector3& cameraPosition,
                         const Vector3& emitterPosition, const EmitterConfig& config);

    // 渲染粒子
    void RenderParticles(Graphic::ICamera* camera,
                         Graphic::ICommandBuffer* cmdBuffer);

    // 发射粒子（填充 CPU staging buffer，然后上传到 GPU）
    void SpawnParticles(uint32_t count, const EmitterConfig& config);

    // 访问
    uint32_t GetAliveCount() const { return m_AliveCount; }
    uint32_t GetMaxParticles() const { return m_MaxParticles; }
    bool     IsInitialized() const { return m_Initialized; }

    // 设置/获取混合模式
    void SetBlendMode(ParticleBlendMode mode) { m_BlendMode = mode; }
    ParticleBlendMode GetBlendMode() const { return m_BlendMode; }

private:
    // GPU 资源
    Graphic::IRenderDevice* m_Device = nullptr;
    std::shared_ptr<Graphic::IBuffer> m_ParticleBuffer;      // SSBO: 粒子数据
    std::shared_ptr<Graphic::IBuffer> m_IndirectBuffer;      // 间接绘制参数
    std::shared_ptr<Graphic::IBuffer> m_StagingBuffer;       // CPU staging 上传
    std::shared_ptr<Graphic::IBuffer> m_UniformBuffer;       // 每帧 uniform 数据

    // 管线
    std::shared_ptr<Graphic::IComputePipeline>  m_ComputePipeline;
    std::shared_ptr<Graphic::IGraphicsPipeline> m_GraphicsPipeline;
    std::shared_ptr<Graphic::IDescriptorSet>    m_DescriptorSet;
    std::shared_ptr<Graphic::IDescriptorSetLayout> m_DescriptorSetLayout;

    // 着色器
    std::shared_ptr<Graphic::IShader> m_ComputeShader;
    std::shared_ptr<Graphic::IShader> m_VertexShader;
    std::shared_ptr<Graphic::IShader> m_FragmentShader;

    // CPU 端数据
    std::vector<GPUParticleData> m_ParticleCPUData;
    std::mt19937 m_RNG;

    // 状态
    uint32_t m_MaxParticles = 65536;
    uint32_t m_AliveCount   = 0;
    uint32_t m_FreeHead     = 0;
    bool     m_Initialized  = false;
    float    m_SpawnAccumulator = 0.0f;

    // 混合模式
    ParticleBlendMode m_BlendMode = ParticleBlendMode::Alpha;

    // 内部辅助函数
    bool CreateBuffers();
    bool CreateShaders();
    bool CreatePipelines();
    void UpdateIndirectBuffer();
    void UploadParticleData();
};

} // namespace Prisma::Particles
