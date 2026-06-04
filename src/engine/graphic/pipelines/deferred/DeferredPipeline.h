#pragma once

#include "Export.h"
#include "graphic/LogicalPass.h"
#include "graphic/LogicalPipeline.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IGBuffer.h"
#include "graphic/interfaces/IPipeline.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ICamera;
class GBuffer;
class GeometryPass;
class LightingPass;
class SkyboxPass;
class TransparentPass;
class CompositionPass;

class ENGINE_API DeferredPipeline : public LogicalDeferredPipeline, public IPipeline {
public:
    // 光源类型
    enum class LightType {
        Directional = 0,
        Point = 1,
        Spot = 2,
        Ambient = 3
    };

    // 光源结构
    struct Light {
        LightType type = LightType::Point;
        PrismaMath::vec3 position = {0.0f, 0.0f, 0.0f};
        PrismaMath::vec3 direction = {0.0f, -1.0f, -1.0f};
        PrismaMath::vec3 color = {1.0f, 1.0f, 1.0f};
        float intensity = 1.0f;
        float range = 10.0f;
        float spotAngle = 30.0f;
        bool castShadows = false;
    };

    // 后处理效果
    enum class PostProcessEffect {
        None = 0,
        ToneMapping = 1 << 0,
        GammaCorrection = 1 << 1,
        Bloom = 1 << 2,
        MotionBlur = 1 << 3,
        SMAA = 1 << 4
    };

public:
    DeferredPipeline();
    ~DeferredPipeline() override;

    // IPipeline 接口
    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;
    RenderMode GetMode() const override { return RenderMode::Mode3D_Deferred; }

    // 初始化管线
    /// 创建并添加所有 Pass
    bool Initialize();

    // 设置视口尺寸并重建 GBuffer
    void SetResolution(uint32_t width, uint32_t height);

    // 更新管线数据
    void Update(Prisma::Timestep ts, Prisma::Graphic::ICamera* camera);

    // 执行管线渲染
    void Execute(const PassExecutionContext& context) override;

    // === Pass 访问 ===

    // 获取几何 Pass
    GeometryPass* GetGeometryPass() const { return m_geometryPass.get(); }

    // 获取天空盒 Pass
    SkyboxPass* GetSkyboxPass() const { return m_skyboxPass.get(); }

    // 获取光照 Pass
    LightingPass* GetLightingPass() const { return m_lightingPass.get(); }

    // 获取透明物体 Pass
    TransparentPass* GetTransparentPass() const { return m_transparentPass.get(); }

    // 获取合成 Pass
    CompositionPass* GetCompositionPass() const { return m_compositionPass.get(); }

    // === 光照设置 ===

    // 添加光源
    void AddLight(const Light& light);

    // 清除所有光源
    void ClearLights();

    // 设置光源列表
    void SetLights(const std::vector<Light>& lights);

    // 获取光源列表
    const std::vector<Light>& GetLights() const { return m_lights; }

    // 设置环境光
    void SetAmbientLight(const PrismaMath::vec3& ambient);
    const PrismaMath::vec3& GetAmbientLight() const { return m_ambientLight; }

    // === 后处理设置 ===

    // 设置后处理效果
    void SetPostProcessEffect(PostProcessEffect effect, bool enable);

    // 检查后处理效果是否启用
    bool IsPostProcessEffectEnabled(PostProcessEffect effect) const;

    // === 渲染统计 ===

    struct RenderStats {
        uint32_t geometryPassObjects = 0;
        uint32_t geometryPassTriangles = 0;
        uint32_t lightingPassLights = 0;
        uint32_t transparentObjects = 0;
        uint32_t postProcessEffects = 0;
        float lastFrameTime = 0.0f;
        float geometryPassTime = 0.0f;
        float lightingPassTime = 0.0f;
        float transparentPassTime = 0.0f;
        float compositionPassTime = 0.0f;
    };

    const RenderStats& GetRenderStats() const { return m_stats; }

private:
    // 更新所有 Pass 的相机数据
    void UpdatePassesCameraData(Prisma::Graphic::ICamera* camera);
    void UpdatePassesCameraData(const PrismaMath::mat4& view, const PrismaMath::mat4& projection);

    // 收集渲染统计
    void CollectStats();

private:
    std::shared_ptr<GBuffer> m_gBufferOwner;

    // Pass 实例
    std::shared_ptr<GeometryPass> m_geometryPass;
    std::shared_ptr<SkyboxPass> m_skyboxPass;
    std::shared_ptr<LightingPass> m_lightingPass;
    std::shared_ptr<TransparentPass> m_transparentPass;
    std::shared_ptr<CompositionPass> m_compositionPass;

    // 相机接口
    Prisma::Graphic::ICamera* m_camera;

    IRenderDevice* m_device = nullptr;

    // 光照数据
    std::vector<Light> m_lights;
    PrismaMath::vec3 m_ambientLight;

    // 渲染统计
    RenderStats m_stats;
};

} // namespace Prisma::Graphic
