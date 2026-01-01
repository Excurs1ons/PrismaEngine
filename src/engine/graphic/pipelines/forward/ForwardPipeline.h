#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/LogicalPipeline.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/ICamera.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

// 前置声明
namespace PrismaEngine {
    class UIPass;
}

namespace PrismaEngine::Graphic {

/// @brief 前向渲染管线实现
/// 管理和执行前向渲染的所有 Pass
/// 顺序：DepthPrePass → OpaquePass → SkyboxPass → TransparentPass → UIPass
class ForwardPipeline : public LogicalForwardPipeline {
public:
    ForwardPipeline();
    ~ForwardPipeline() override;

    /// @brief 初始化管线
    /// 创建并添加所有 Pass
    bool Initialize();

    /// @brief 更新管线数据
    /// @param deltaTime 时间增量
    /// @param camera 相机接口
    void Update(float deltaTime, class PrismaEngine::Graphic::ICamera* camera);

    /// @brief 执行管线渲染
    /// @param context 执行上下文
    void Execute(const PassExecutionContext& context) override;

    // === Pass 访问 ===

    /// @brief 获取深度预渲染 Pass
    class DepthPrePass* GetDepthPrePass() const { return m_depthPrePass.get(); }

    /// @brief 获取不透明物体 Pass
    class OpaquePass* GetOpaquePass() const { return m_opaquePass.get(); }

    /// @brief 获取天空盒 Pass
    class SkyboxPass* GetSkyboxPass() const { return m_skyboxPass.get(); }

    /// @brief 获取透明物体 Pass
    class TransparentPass* GetTransparentPass() const { return m_transparentPass.get(); }

    /// @brief 获取 UI Pass
    PrismaEngine::UIPass* GetUIPass() const { return m_uiPass.get(); }

    // === 渲染统计 ===

    struct RenderStats {
        uint32_t totalDrawCalls = 0;
        uint32_t totalTriangles = 0;
        uint32_t opaqueObjects = 0;
        uint32_t transparentObjects = 0;
        float lastFrameTime = 0.0f;
    };

    const RenderStats& GetRenderStats() const { return m_stats; }

private:
    /// @brief 更新所有 Pass 的相机数据
    void UpdatePassesCameraData(class PrismaEngine::Graphic::ICamera* camera);

    /// @brief 收集渲染统计
    void CollectStats();

private:
    // Pass 实例（使用 shared_ptr 管理生命周期）
    std::shared_ptr<class DepthPrePass> m_depthPrePass;
    std::shared_ptr<class OpaquePass> m_opaquePass;
    std::shared_ptr<class SkyboxPass> m_skyboxPass;
    std::shared_ptr<class TransparentPass> m_transparentPass;
    std::shared_ptr<PrismaEngine::UIPass> m_uiPass;

    // 相机接口
    class PrismaEngine::Graphic::ICamera* m_camera;

    // 渲染统计
    RenderStats m_stats;
};

} // namespace PrismaEngine::Graphic
