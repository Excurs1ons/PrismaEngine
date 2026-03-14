#pragma once

#include "interfaces/RenderTypes.h"
#include "graphic/Renderer.h"
#include <vector>

namespace Prisma::Graphic {

class ICommandBuffer;

/**
 * @brief 不透明渲染通道 (Opaque Pass)
 * 没有任何单例，由 ForwardPipeline 调用。
 */
class OpaquePass {
public:
    OpaquePass();
    ~OpaquePass() = default;

    // 执行渲染逻辑
    void Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands);

    // 数据设置
    void SetCameraData(const PrismaMath::mat4& view, const PrismaMath::mat4& proj);
    void SetLights(const std::vector<Light>& lights);

private:
    PrismaMath::mat4 m_ViewMatrix;
    PrismaMath::mat4 m_ProjMatrix;
    std::vector<Light> m_Lights;
};

} // namespace Prisma::Graphic
