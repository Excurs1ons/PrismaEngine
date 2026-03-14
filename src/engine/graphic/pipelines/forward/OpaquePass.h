#pragma once

#include "interfaces/RenderTypes.h"
#include "graphic/Renderer.h"
#include "ForwardRenderPassBase.h"
#include <vector>

namespace Prisma::Graphic {

class ICommandBuffer;

/**
 * @brief 不透明渲染通道 (Opaque Pass)
 * 没有任何单例，由 ForwardPipeline 调用。
 */
class OpaquePass : public ForwardRenderPass {
public:
    OpaquePass();
    ~OpaquePass() override = default;

    // IPass 接口实现
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    // 旧版执行接口支持 (向后兼容)
    void Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands);

    // 数据设置
    void SetLights(const std::vector<Light>& lights);

private:
    std::vector<Light> m_Lights;
};

} // namespace Prisma::Graphic
