#pragma once

#include "../pipelines/forward/ForwardRenderPassBase.h"
#include <memory>

namespace Prisma::Graphic {

class ICommandBuffer;
class IRenderDevice;
class OrthographicCamera;

/**
 * @brief 2D UI 渲染通道
 * 
 * 独立于场景渲染，使用屏幕空间坐标系。
 * 通常在所有场景渲染和后处理完成后执行。
 */
class ENGINE_API UIPass2D : public ForwardRenderPass {
public:
    UIPass2D();
    ~UIPass2D() override;

    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    /**
     * @brief 执行 UI 渲染
     */
    void RenderUI(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height);

private:
    std::shared_ptr<OrthographicCamera> m_uiCamera;
};

} // namespace Prisma::Graphic
