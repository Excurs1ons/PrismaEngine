#pragma once

#include "../pipelines/forward/ForwardRenderPassBase.h"
#include "interfaces/ICommandBuffer.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ITexture;
class IShader;
class IPipelineState;
class ITextureRenderTarget;
class IRenderDevice;

/* 2D 光照渲染通道 */
class Light2DPass : public ForwardRenderPass {
public:
    Light2DPass();
    ~Light2DPass() override;

    void Execute(const PassExecutionContext& context) override;
    
    // ICommandBuffer 驱动的执行
    void ExecuteLight(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height);
    
    void Update(Prisma::Timestep ts) override;

    std::shared_ptr<ITexture> GetLightTexture() const { return m_lightTexture; }

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);

    std::shared_ptr<ITexture> m_lightTexture;
    std::shared_ptr<ITextureRenderTarget> m_lightRT;
    
    std::shared_ptr<IShader> m_pointLightVertexShader;
    std::shared_ptr<IShader> m_pointLightPixelShader;
    std::shared_ptr<IPipelineState> m_pointLightPSO;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
