#pragma once

#include "graphic/RenderPass.h"
#include <memory>
#include <vector>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

// 不透明物体渲染通道 - 主要的前向渲染Pass
class OpaquePass : public RenderPass
{
public:
    OpaquePass();
    ~OpaquePass();

    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;

    // 设置渲染目标
    void SetRenderTarget(void* renderTarget) override;

    // 清屏操作
    void ClearRenderTarget(float r, float g, float b, float a) override;

    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;

    // 设置深度缓冲区
    void SetDepthBuffer(void* depthBuffer);

    // 设置视图矩阵
    void SetViewMatrix(const Prisma::Matrix4x4& view);

    // 设置投影矩阵
    void SetProjectionMatrix(const Prisma::Matrix4x4& projection);

    // 设置光源信息
    struct Light {
        Prisma::Vector3 position;
        Prisma::Color color;
        float intensity;
        Prisma::Vector3 direction;  // 用于方向光
        int type;  // 0=directional, 1=point, 2=spot
    };
    void SetLights(const std::vector<Light>& lights);

    // 设置环境光
    void SetAmbientLight(const Prisma::Color& color);

private:
    // 深度缓冲区
    void* m_depthBuffer = nullptr;

    // 相机矩阵
    Prisma::Matrix4x4 m_view = Prisma::Matrix4x4();
    Prisma::Matrix4x4 m_projection = Prisma::Matrix4x4();
    Prisma::Matrix4x4 m_viewProjection = Prisma::Matrix4x4();

    // 光照数据
    std::vector<Light> m_lights;
    Prisma::Vector3 m_ambientLight = Prisma::Vector3(0.1f, 0.1f, 0.1f);

    // 渲染目标
    void* m_renderTarget = nullptr;

    // 视口尺寸
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // 渲染统计
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t objects = 0;
    };
    RenderStats m_stats;
};

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine