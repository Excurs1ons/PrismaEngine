#pragma once

#include "graphic/RenderPass.h"
#include "GBuffer.h"
#include <DirectXMath.h>
#include <memory>
#include <vector>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

// 光源类型
enum class LightType : uint32_t {
    Directional = 0,
    Point = 1,
    Spot = 2
};

// 光源数据结构
struct Light {
    LightType type;
    DirectX::XMFLOAT3 position;      // 点光源/聚光灯位置
    DirectX::XMFLOAT3 direction;     // 方向光/聚光灯方向
    DirectX::XMFLOAT3 color;
    float intensity;

    // 点光源/聚光灯参数
    float range;
    float innerCone;     // 聚光灯内锥角（弧度）
    float outerCone;     // 聚光灯外锥角（弧度）

    // 阴影参数
    bool castShadows;
    uint32_t shadowMapIndex;
    DirectX::XMMATRIX shadowMatrix;

    Light() {
        type = LightType::Point;
        position = DirectX::XMFLOAT3(0, 0, 0);
        direction = DirectX::XMFLOAT3(0, -1, 0);
        color = DirectX::XMFLOAT3(1, 1, 1);
        intensity = 1.0f;
        range = 10.0f;
        innerCone = 0.5f;
        outerCone = 1.0f;
        castShadows = false;
        shadowMapIndex = 0xFFFFFFFF;
        shadowMatrix = DirectX::XMMatrixIdentity();
    }
};

// 光照通道 - 使用G-Buffer计算场景光照
class LightingPass : public RenderPass
{
public:
    LightingPass();
    ~LightingPass();

    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;

    // 设置渲染目标（最终的场景纹理）
    void SetRenderTarget(void* renderTarget) override;

    // 清屏操作
    void ClearRenderTarget(float r, float g, float b, float a) override;

    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;

    // 设置G-Buffer
    void SetGBuffer(std::shared_ptr<GBuffer> gbuffer);

    // 设置环境光
    void SetAmbientLight(const DirectX::XMFLOAT3& ambient);

    // 设置光源
    void SetLights(const std::vector<Light>& lights);

    // 添加光源
    void AddLight(const Light& light);

    // 清除所有光源
    void ClearLights();

    // 启用/禁用IBL（基于图像的照明）
    void SetIBL(bool enable);

    // 设置IBL纹理
    void SetIBLTextures(void* irradianceMap, void* prefilterMap, void* brdfLUT);

private:
    // G-Buffer引用
    std::shared_ptr<GBuffer> m_gbuffer;

    // 渲染目标
    void* m_renderTarget = nullptr;

    // 视口尺寸
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // 环境光
    DirectX::XMFLOAT3 m_ambientLight = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);

    // 光源列表
    std::vector<Light> m_lights;

    // IBL设置
    bool m_iblEnabled = true;
    void* m_irradianceMap = nullptr;
    void* m_prefilterMap = nullptr;
    void* m_brdfLUT = nullptr;

    // 渲染统计
    struct LightingPassStats {
        uint32_t lightsRendered = 0;
        uint32_t shadowCastingLights = 0;
    } m_stats;

    // 渲染全屏四边形
    void RenderFullScreenQuad(RenderCommandContext* context);

    // 应用方向光
    void ApplyDirectionalLight(RenderCommandContext* context, const Light& light);

    // 应用点光源
    void ApplyPointLight(RenderCommandContext* context, const Light& light);

    // 应用聚光灯
    void ApplySpotLight(RenderCommandContext* context, const Light& light);
};

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine