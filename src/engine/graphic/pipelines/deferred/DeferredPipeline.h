#pragma once

#include "graphic/ScriptableRenderPipeline.h"
#include "graphic/pipelines/SkyboxRenderPass.h"
#include "graphic/pipelines/forward/TransparentPass.h"
#include "graphic/ICamera.h"
#include "GBuffer.h"
#include "GeometryPass.h"
#include "LightingPass.h"
#include "CompositionPass.h"
#include <memory>
#include <vector>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

// 延迟渲染管线
// 渲染流程:
// 1. 深度预渲染 (可选)
// 2. 几何通道 - 将场景几何信息渲染到G-Buffer
// 3. 天空盒渲染 - 渲染天空盒到G-Buffer
// 4. 光照通道 - 使用G-Buffer计算场景光照
// 5. 透明物体通道 - 前向渲染透明物体
// 6. 合成通道 - 应用后处理效果并输出最终图像
class DeferredPipeline
{
public:
    DeferredPipeline();
    ~DeferredPipeline();

    // 初始化延迟渲染管线
    bool Initialize(ScriptableRenderPipeline* renderPipe);

    // 关闭渲染管线
    void Shutdown();

    // 更新渲染管线（每帧调用）
    void Update(float deltaTime);

    // 设置相机
    void SetCamera(ICamera* camera);

    // 添加光源
    void AddLight(const Light& light);
    void ClearLights();
    void SetLights(const std::vector<Light>& lights);

    // 设置环境光
    void SetAmbientLight(const DirectX::XMFLOAT3& ambient);

    // 启用/禁用后处理效果
    void SetPostProcessEffect(PostProcessEffect effect, bool enable);
    bool IsPostProcessEffectEnabled(PostProcessEffect effect) const;

    // 获取渲染统计信息
    struct RenderStats {
        // 几何通道统计
        uint32_t geometryPassObjects = 0;
        uint32_t geometryPassTriangles = 0;

        // 光照通道统计
        uint32_t lightingPassLights = 0;
        uint32_t shadowCastingLights = 0;

        // 透明物体统计
        uint32_t transparentObjects = 0;

        // 合成通道统计
        uint32_t postProcessEffects = 0;

        // 性能统计
        float lastFrameTime = 0.0f;
        float geometryPassTime = 0.0f;
        float lightingPassTime = 0.0f;
        float transparentPassTime = 0.0f;
        float compositionPassTime = 0.0f;

        // RTXGI统计 (预留)
        uint32_t rtxgiActiveProbes = 0;
        float rtxgiUpdateTime = 0.0f;
        float rtxgiMemoryUsage = 0.0f;
    };

    const RenderStats& GetRenderStats() const { return m_stats; }

    // ========== RTXGI集成接口 (预留) ==========

    /**
     * @brief 初始化RTXGI DDGI系统
     * @param config RTXGI配置参数
     * @return 是否初始化成功
     *
     * 此接口为NVIDIA RTXGI集成预留，未来将支持：
     * - 实时全局光照 (DDGI)
     * - 光线追踪反射
     * - 动态环境光照
     * - AI驱动的光照优化
     */
    bool InitializeRTXGI(void* config);

    /**
     * @brief 更新RTXGI系统
     * @param deltaTime 帧时间
     *
     * 更新DDGI体积、探头数据等
     */
    void UpdateRTXGI(float deltaTime);

    /**
     * @brief 应用RTXGI全局照明
     * @param context 渲染命令上下文
     *
     * 在光照通道中应用RTXGI计算的全局照明
     */
    void ApplyRTXGIIllumination(RenderCommandContext* context);

    /**
     * @brief 获取RTXGI渲染目标
     * @return RTXGI DDGI渲染目标数组
     *
     * 返回DDGI纹理供着色器使用：
     * - Irradiance纹理 (漫反射照明)
     * - Distance纹理 (距离信息)
     * - Offsets纹理 (探头偏移)
     */
    void** GetRTXGITargets();

    /**
     * @brief 设置RTXGI质量级别
     * @param quality 质量级别 (0=Off, 1=Low, 2=Medium, 3=High, 4=Ultra)
     */
    void SetRTXGIQuality(int quality);

    /**
     * @brief 检查RTXGI是否可用
     * @return RTXGI支持状态
     */
    bool IsRTXGIAvailable() const;

    /**
     * @brief 启用/禁用RTXGI
     * @param enabled 是否启用
     */
    void SetRTXGIEnabled(bool enabled);

    /**
     * @brief RTXGI调试接口
     * @param debugMode 调试模式
     *
     * 支持的调试模式：
     * - 0: 关闭
     * - 1: 显示DDGI探头
     * - 2: 显示辐照度纹理
     * - 3: 显示距离纹理
     * - 4: 显示探头更新区域
     */
    void SetRTXGIDebugMode(int debugMode);

    // ========== RTXGI集成接口结束 ==========

private:
    // 渲染管线引用
    ScriptableRenderPipeline* m_renderPipe;

    // 渲染通道
    std::shared_ptr<GBuffer> m_gbuffer;
    std::shared_ptr<GeometryPass> m_geometryPass;
    std::shared_ptr<SkyboxRenderPass> m_skyboxRenderPass;
    std::shared_ptr<LightingPass> m_lightingPass;
    std::shared_ptr<TransparentPass> m_transparentPass;
    std::shared_ptr<CompositionPass> m_compositionPass;

    // 相机引用
    ICamera* m_camera;

    // 光源列表
    std::vector<Light> m_lights;
    DirectX::XMFLOAT3 m_ambientLight = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);

    // 渲染统计
    RenderStats m_stats;

    // 初始化各种渲染通道
    bool InitializeRenderPasses();

    // 创建渲染目标
    bool CreateRenderTargets();

    // 清理资源
    void Cleanup();
};

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine