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

namespace Prisma { class Scene; }

namespace Prisma::Graphic {

class IBuffer;
class ITexture;
class ISampler;
class ICamera;

// 场景 SSBO 数据结构 — 必须与 pathtrace.comp 中的 SceneObject 布局完全一致（std430）
struct PTSceneObject {
    float p0[4];    // type=0(plane): point + type; type=1(sphere): center; type=2(box): center; type=4(mesh): unused
    float p1[4];    // plane: normal; sphere: (r,0,0,0); box: (hx,hy,hz,0); mesh: (firstTriangle,triangleCount,0,0)
    float p2[4];    // plane: (umin,vmin,umax,vmax); box/cone: (cosA,sinA,0,0); sphere/mesh: unused
    float color[4]; // rgb + emissive in w
    float worldMatrix[16]; // 4x4 列主序世界矩阵，每帧由 UpdateTransforms 更新
};

struct PathTracingSceneData {
    int objectCount = 0;
    float pad1 = 0, pad2 = 0, pad3 = 0;
    PTSceneObject objects[32]{};
};

// 三角形 SSBO 数据结构 — 必须与 pathtrace.comp 中的 Triangle 布局完全一致（std430）
struct PTTriangle {
    float v0[3], v1[3], v2[3];
};

struct PathTracingTriangleData {
    int triangleCount = 0;
    float pad1 = 0, pad2 = 0, pad3 = 0;
    static constexpr uint32_t MAX_TRIANGLES = 65536;
    PTTriangle triangles[MAX_TRIANGLES]{};
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

    // 设置预创建的着色器对象（替代 SPIR-V 方式，优先使用）
    void SetComputeShader(std::shared_ptr<IShader> shader) { m_computeShader = std::move(shader); }
    void SetPresentShaders(std::shared_ptr<IShader> vert, std::shared_ptr<IShader> frag) {
        m_presentVertShader = std::move(vert);
        m_presentFragShader = std::move(frag);
    }

    // 场景数据
    void SetSceneData(const PathTracingSceneData& data);
    void SetTriangleData(const PathTracingTriangleData& data);

    // 从引擎 ECS 场景构建路径追踪数据
    void BuildFromScene(::Prisma::Scene* scene);

    // 每帧更新场景对象的 worldMatrix（本地空间顶点 → 世界空间变换）
    void UpdateTransforms(::Prisma::Scene* scene);

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

    // Overlay 回调（仅用于应用自定义 HUD 文本，gizmo 由管线内部处理）
    void SetOverlayCallback(OverlayCallback cb) { m_overlayCB = std::move(cb); }

    // Gizmo overlay 着色器（应用通过此接口传入，管线负责创建 PSO 和渲染）
    void SetOverlayShaders(std::shared_ptr<IShader> vert, std::shared_ptr<IShader> frag) {
        m_gizmoVertShader = std::move(vert);
        m_gizmoFragShader = std::move(frag);
    }



private:
    bool CreateResources();
    void DestroyResources();

    IRenderDevice* m_device = nullptr;

    // 着色器 SPIR-V 数据
    std::vector<uint8_t> m_computeSPIRV;
    std::vector<uint8_t> m_presentVertSPIRV;
    std::vector<uint8_t> m_presentFragSPIRV;

    // 预创建的着色器对象（优先于 SPIR-V 数据）
    std::shared_ptr<IShader> m_computeShader;

    // 路径追踪资源
    std::unique_ptr<ITexture> m_storageTexture;
    std::unique_ptr<IBuffer> m_cameraUBO;
    std::unique_ptr<IBuffer> m_sceneSSBO;
    std::unique_ptr<IBuffer> m_triangleBuffer;
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

    // Gizmo overlay 资源（管线内部管理）
    std::shared_ptr<IShader> m_gizmoVertShader;
    std::shared_ptr<IShader> m_gizmoFragShader;
    std::shared_ptr<IPipelineState> m_gizmoPSO;
    std::shared_ptr<ICamera> m_gizmoCamera;

    // 内部辅助方法
    void InitOverlayResources();
    void RenderOverlay(ICommandBuffer* cmd);
    void LoadDefaultShaders();

    // 场景数据缓存（用于延迟初始化后重上传）
    PathTracingSceneData m_cachedSceneData{};
    PathTracingTriangleData m_cachedTriangleData{};
    uint32_t m_cachedNodeHandles[32]{}; // objectIdx → node handle (用于每帧 UpdateTransforms)
    Scene* m_scene = nullptr;           // 当前关联场景（用于每帧读取世界变换）

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
