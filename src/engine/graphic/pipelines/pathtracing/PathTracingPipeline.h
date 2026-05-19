#pragma once

#include "Export.h"
#include "interfaces/IPipeline.h"
#include "interfaces/IShader.h"
#include "interfaces/IPipelineState.h"
#include "interfaces/IDescriptorSet.h"
#include "interfaces/IComputePipeline.h"
#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

namespace Prisma::Graphic {

class IBuffer;
class ITexture;
class ISampler;

// 场景 SSBO 数据结构 — 必须与 pathtrace.comp 中的 SceneObject 布局完全一致（std430）
struct PTSceneObject {
    float p0[4];    // type=0(plane): point + type; type=1(sphere): center; type=2(box): center
    float p1[4];    // plane: normal; sphere: (r,0,0,0); box: (hx,hy,hz,0)
    float p2[4];    // plane: (umin,vmin,umax,vmax); box: (cosA,sinA,0,0); sphere: unused
    float color[4]; // rgb + emissive in w
};

struct PathTracingSceneData {
    int objectCount = 0;
    float pad1 = 0, pad2 = 0, pad3 = 0;
    PTSceneObject objects[32]{};
};

// Camera UBO 数据结构 — 必须与 pathtrace.comp 中的 CameraUBO 布局完全一致（std140）
struct PathTracingCameraUBO {
    float cameraPos[4]{};
    float cameraDir[4]{};
    float cameraUp[4]{};
    float cameraRight[4]{};
    float fov = 70.0f;
    float aspectRatio = 1.0f;
    int frameCount = 0;
    int maxBounces = 8;
    int samplesPerPixel = 1;
    int useAccumulation = 1;
    int resetAccumulation = 0;
    int enableNEE = 0;
};

class ENGINE_API PathTracingPipeline : public IPipeline {
public:
    using OverlayCallback = std::function<void(ICommandBuffer*)>;

    PathTracingPipeline();
    ~PathTracingPipeline() override;

    // IPipeline 接口
    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;

    // 设置计算和 present 着色器 SPIR-V（必须在 Initialize 之前调用）
    void SetComputeShaderSPIRV(const void* data, size_t size);
    void SetPresentShadersSPIRV(const void* vertData, size_t vertSize,
                                const void* fragData, size_t fragSize);

    // 场景数据
    void SetSceneData(const PathTracingSceneData& data);
    void ResetAccumulation();
    void EnableNEE(bool enabled) { m_enableNEE = enabled; }
    void SetMaxBounces(uint32_t bounces) { m_maxBounces = bounces; }
    void SetMaxSamples(uint32_t samples) { m_maxSamples = samples; }
    void SetConverged(uint32_t samples) { m_converged = m_frameCount >= samples; }

    // 输出保存（headless 模式）
    bool SaveOutput(const std::string& path);

    // 状态
    uint32_t GetFrameCount() const { return m_frameCount; }
    bool IsConverged() const { return m_converged; }

    // Overlay 回调
    void SetOverlayCallback(OverlayCallback cb) { m_overlayCB = std::move(cb); }

private:
    bool CreateResources();
    void DestroyResources();

    IRenderDevice* m_device = nullptr;

    // 着色器 SPIR-V 数据
    std::vector<uint8_t> m_computeSPIRV;
    std::vector<uint8_t> m_presentVertSPIRV;
    std::vector<uint8_t> m_presentFragSPIRV;

    // 路径追踪资源
    std::unique_ptr<ITexture> m_storageTexture;
    std::unique_ptr<IBuffer> m_cameraUBO;
    std::unique_ptr<IBuffer> m_sceneSSBO;
    std::unique_ptr<IComputePipeline> m_computePipeline;
    std::shared_ptr<IDescriptorSet> m_descriptorSet;

    // Present 资源
    std::shared_ptr<IShader> m_presentVertShader;
    std::shared_ptr<IShader> m_presentFragShader;
    std::shared_ptr<IPipelineState> m_presentPSO;
    std::shared_ptr<IDescriptorSetLayout> m_presentDSLayout;
    std::unique_ptr<ISampler> m_presentSampler;
    std::shared_ptr<IDescriptorSet> m_presentDescriptorSet;

    // Overlay 回调
    OverlayCallback m_overlayCB;

    // 场景数据缓存（用于延迟初始化后重上传）
    PathTracingSceneData m_cachedSceneData{};

    // 状态
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_frameCount = 0;
    uint32_t m_maxSamples = 512;
    uint32_t m_maxBounces = 8;
    bool m_enableNEE = false;
    bool m_converged = false;
    bool m_resetAccumulation = false;
    bool m_initialized = false;
    bool m_shadersSet = false;
};

} // namespace Prisma::Graphic
