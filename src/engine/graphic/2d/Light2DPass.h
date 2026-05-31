#pragma once

#include "Pass2D.h"
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
class Light2DPass : public Pass2D {
public:
    Light2DPass();
    ~Light2DPass() override;

    void Execute(const PassExecutionContext& context) override;
    
    // ICommandBuffer 驱动的执行
    void ExecuteLight(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height);
    
    void Update(Prisma::Timestep ts) override;

    std::shared_ptr<ITexture> GetLightTexture() const { return m_lightTexture; }

    void SetViewMatrix(const PrismaMath::mat4& v) { m_view = v; UpdateViewProjection(); }
    void SetProjectionMatrix(const PrismaMath::mat4& p) { m_projection = p; UpdateViewProjection(); }

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);
    void UpdateViewProjection() { m_viewProjection = m_projection * m_view; }

    std::shared_ptr<ITexture> m_lightTexture;
    std::shared_ptr<ITextureRenderTarget> m_lightRT;
    
    std::shared_ptr<IShader> m_pointLightVertexShader;
    std::shared_ptr<IShader> m_pointLightPixelShader;
    std::shared_ptr<IPipelineState> m_pointLightPSO;

    PrismaMath::mat4 m_view = PrismaMath::mat4(1.0f);
    PrismaMath::mat4 m_projection = PrismaMath::mat4(1.0f);
    PrismaMath::mat4 m_viewProjection = PrismaMath::mat4(1.0f);

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Prisma::Graphic
