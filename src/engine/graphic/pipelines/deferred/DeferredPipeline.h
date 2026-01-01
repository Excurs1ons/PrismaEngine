#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/LogicalPipeline.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IGBuffer.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace PrismaEngine {
namespace Graphic {

// 前置声明
class ICamera;

} // namespace Graphic
} // namespace Engine

namespace PrismaEngine::Graphic {

// 前置声明
class GeometryPass;
class LightingPass;
class SkyboxPass;
class TransparentPass;
class CompositionPass;

/// @brief 延迟渲染管线实现
/// 管理和执行延迟渲染的所有 Pass
/// 顺序：GeometryPass → SkyboxPass → LightingPass → TransparentPass → CompositionPass
class DeferredPipeline : public LogicalDeferredPipeline {
public:
    // 光源类型
    enum class LightType {
        Directional = 0,
        Point = 1,
        Spot = 2
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

    /// @brief 初始化管线
    /// 创建并添加所有 Pass
    bool Initialize();

    /// @brief 更新管线数据
    /// @param deltaTime 时间增量
    /// @param camera 相机接口
    void Update(float deltaTime, PrismaEngine::Graphic::ICamera* camera);

    /// @brief 执行管线渲染
    /// @param context 执行上下文
    void Execute(const PassExecutionContext& context) override;

    // === Pass 访问 ===

    /// @brief 获取几何 Pass
    GeometryPass* GetGeometryPass() const { return m_geometryPass.get(); }

    /// @brief 获取天空盒 Pass
    SkyboxPass* GetSkyboxPass() const { return m_skyboxPass.get(); }

    /// @brief 获取光照 Pass
    LightingPass* GetLightingPass() const { return m_lightingPass.get(); }

    /// @brief 获取透明物体 Pass
    TransparentPass* GetTransparentPass() const { return m_transparentPass.get(); }

    /// @brief 获取合成 Pass
    CompositionPass* GetCompositionPass() const { return m_compositionPass.get(); }

    // === 光照设置 ===

    /// @brief 添加光源
    void AddLight(const Light& light);

    /// @brief 清除所有光源
    void ClearLights();

    /// @brief 设置光源列表
    void SetLights(const std::vector<Light>& lights);

    /// @brief 获取光源列表
    const std::vector<Light>& GetLights() const { return m_lights; }

    /// @brief 设置环境光
    void SetAmbientLight(const PrismaMath::vec3& ambient);
    const PrismaMath::vec3& GetAmbientLight() const { return m_ambientLight; }

    // === 后处理设置 ===

    /// @brief 设置后处理效果
    void SetPostProcessEffect(PostProcessEffect effect, bool enable);

    /// @brief 检查后处理效果是否启用
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
    /// @brief 更新所有 Pass 的相机数据
    void UpdatePassesCameraData(PrismaEngine::Graphic::ICamera* camera);

    /// @brief 收集渲染统计
    void CollectStats();

private:
    // Pass 实例
    std::shared_ptr<GeometryPass> m_geometryPass;
    std::shared_ptr<SkyboxPass> m_skyboxPass;
    std::shared_ptr<LightingPass> m_lightingPass;
    std::shared_ptr<TransparentPass> m_transparentPass;
    std::shared_ptr<CompositionPass> m_compositionPass;

    // 相机接口
    PrismaEngine::Graphic::ICamera* m_camera;

    // 光照数据
    std::vector<Light> m_lights;
    PrismaMath::vec3 m_ambientLight;

    // 渲染统计
    RenderStats m_stats;
};

} // namespace PrismaEngine::Graphic
