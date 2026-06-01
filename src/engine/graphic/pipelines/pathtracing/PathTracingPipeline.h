#pragma once

#include "Export.h"
#include "interfaces/IPipeline.h"
#include "interfaces/IShader.h"
#include "interfaces/IPipelineState.h"
#include "interfaces/IDescriptorSet.h"
#include "interfaces/IComputePipeline.h"
#include "VulkanRTBackend.h"
#include "AccelStructBuilder.h"
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
    float worldMatrix[16];    // 4x4 列主序世界矩阵
    float invWorldMatrix[16]; // 4x4 列主序世界矩阵的逆
};

struct PathTracingSceneData {
    int objectCount = 0;
    float pad1 = 0, pad2 = 0, pad3 = 0;
    PTSceneObject objects[32]{};
};

// 三角形 SSBO 数据结构 — 必须与 pathtrace.comp 中的 Triangle 布局完全一致（std430）
// 注意：GLSL std430 中 vec3 对齐为 16 字节，所以每顶点必须使用 4 个 float 来填充对齐
// 每个顶点 32 字节：position (vec3 + pad) + normal (vec3 + pad)
struct PTVertex {
    float pos[4];  // position xyz + padding
    float nrm[4];  // normal xyz + padding
};

struct PTTriangle {
    PTVertex vertices[3];
};

struct PathTracingTriangleData {
    int triangleCount = 0;
    float pad1 = 0, pad2 = 0, pad3 = 0;
    static constexpr uint32_t MAX_TRIANGLES = 65536;
    PTTriangle triangles[MAX_TRIANGLES]{};
};

// BVH 节点 (32 字节, 与 GLSL BVHNode 布局一致)
// aabbMin.xyz = 包围盒最小值, aabbMin.w = float(triangleCount)  (0 = 内部节点)
// aabbMax.xyz = 包围盒最大值, aabbMax.w = float(childOrTriStart) (内部: 右子节点索引; 叶子: 首个三角形索引)
struct alignas(16) BVHNode {
    float aabbMin[4];
    float aabbMax[4];
};

static constexpr uint32_t MAX_BVH_NODES = 65536 * 2;

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
    void OnSceneLoaded(::Prisma::Scene* scene) override;
    RenderMode GetMode() const override { return RenderMode::Mode3D_PathTracing; }

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

    // 从引擎场景重新构建路径追踪数据（切换 PrimitiveComponent ↔ MeshRenderer 后调用）
    void ReloadSceneData();
    void SetMode(PathTraceMode mode);
    void CycleMode();
    const char* GetModeName() const;

    // 每帧更新场景对象的 worldMatrix（本地空间顶点 → 世界空间变换）
    void UpdateTransforms(::Prisma::Scene* scene);

    void ResetAccumulation();
    void EnableNEE(bool enabled) { m_enableNEE = enabled; }
    void SetMaxBounces(uint32_t bounces) { m_maxBounces = bounces; }
    void SetMaxSamples(uint32_t samples) { m_maxSamples = samples; }
    uint32_t GetFrameCount() const { return m_frameCount; }
    uint32_t GetMaxSamples() const { return m_maxSamples; }
    bool IsConverged() const { return m_converged; }
    bool SaveOutput(const std::string& path);

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
    bool ResizeResources(); // 仅重建尺寸相关资源（纹理+描述符），不碰着色器/管线

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
    std::unique_ptr<IBuffer> m_bvhBuffer;         // BVH 节点 SSBO (binding 5)
    std::unique_ptr<IBuffer> m_triToObjectBuffer;  // 三角形→对象索引 SSBO (binding 6)
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
    void BuildBVH();  // 构建 CPU BVH 加速结构
    void RebuildBVHForScene();  // BVH 模式：预变换顶点到世界空间 + 重建 BVH

    // 硬件光线追踪辅助方法
    bool LoadRTHardwareShaders();                // 加载 rgen/rchit/rmiss SPIR-V
    bool BuildRTResources(Scene* scene);         // 构建 BLAS/TLAS/RT管线/SBT
    void DestroyRTResources();                    // 清理 RT 资源
    void ExecuteHardwareRT(ICommandBuffer* cmd);  // HardwareRT 模式执行
    void UpdateTLASInstances(ICommandBuffer* cmd); // 更新 TLAS 实例变换

    // Ray Query 辅助方法
    bool BuildRayQueryResources(Scene* scene);
    void DestroyRayQueryResources();
    void ExecuteRayQuery(ICommandBuffer* cmd);
    void UpdateRayQueryTLAS(VkCommandBuffer vkCmd);

    // 场景数据缓存（用于延迟初始化后重上传）
    PathTracingSceneData m_cachedSceneData{};
    PathTracingTriangleData m_cachedTriangleData{};
    std::vector<BVHNode> m_bvhNodes;    // BVH 节点缓存
    std::vector<int> m_triToObject;     // 三角形→对象索引映射
    uint32_t m_bvhNodeCount = 0;
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
    bool m_textureInitialized = false; // 存储纹理是否已有有效数据（用于 PipelineBarrier 状态跟踪）
    PathTraceMode m_mode       = PathTraceMode::BVH;  // 默认 BVH 模式
    PathTraceMode m_targetMode = PathTraceMode::BVH;  // 等待激活的目标模式

    // ======== 硬件光线追踪资源 ========
    std::unique_ptr<VulkanRTBackend> m_rtBackend;
    std::shared_ptr<IDescriptorSetLayout> m_rtDescLayout;  // RT 描述符集布局（用于 resize 重建）
    std::shared_ptr<IDescriptorSet> m_rtRhiDescriptorSet; // 通过 RHI 创建的 RT 描述符集
    std::vector<uint8_t> m_rgenSPIRV;
    std::vector<uint8_t> m_rchitSPIRV;
    std::vector<uint8_t> m_rmissSPIRV;
    bool m_rtResourcesBuilt = false;   // BLAS/TLAS/RT pipeline 是否已构建
    bool m_sceneChangedSinceLastRTBuild = true; // 场景变更后标记需重建 AS

    // ======== Ray Query 资源 ========
    std::unique_ptr<AccelStructBuilder> m_rqBackend;
    std::shared_ptr<IDescriptorSet> m_rqRhiDescriptorSet;
    bool m_rqResourcesBuilt = false;
};

} // namespace Prisma::Graphic
