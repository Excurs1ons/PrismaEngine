#pragma once

#include "Pass2D.h"
#include "ShadowCaster2D.h"
#include "interfaces/ICommandBuffer.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ITexture;
class IShader;
class IPipelineState;
class ITextureRenderTarget;
class IRenderDevice;
class IBuffer;

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

    // ========== 阴影投射体注册 ==========

    static uint32_t RegisterCaster(const ShadowCaster2D& caster);
    static void RemoveCaster(uint32_t handle);
    static void ClearCasters();
    static const std::vector<ShadowCaster2D>& GetCasters();

private:
    void EnsureResources(uint32_t width, uint32_t height, IRenderDevice* device);
    void UpdateViewProjection() { m_viewProjection = m_projection * m_view; }
    void EnsureShadowResources(IRenderDevice* device);
    void RenderShadowGeometry(ICommandBuffer* cmd, IRenderDevice* device,
                              const Matrix4& vp, const Vector2& lightPos,
                              float lightRadius, float shadowIntensity);

    std::shared_ptr<ITexture> m_lightTexture;
    std::shared_ptr<ITextureRenderTarget> m_lightRT;
    
    std::shared_ptr<IShader> m_pointLightVertexShader;
    std::shared_ptr<IShader> m_pointLightPixelShader;
    std::shared_ptr<IPipelineState> m_pointLightPSO;

    // 阴影渲染资源
    std::shared_ptr<IShader> m_shadowVertShader;
    std::shared_ptr<IShader> m_shadowFragShader;
    std::shared_ptr<IPipelineState> m_shadowPSO;
    std::shared_ptr<IBuffer> m_shadowVB;
    uint32_t m_shadowVBCapacity = 0;

    PrismaMath::mat4 m_view = PrismaMath::mat4(1.0f);
    PrismaMath::mat4 m_projection = PrismaMath::mat4(1.0f);
    PrismaMath::mat4 m_viewProjection = PrismaMath::mat4(1.0f);

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    static constexpr uint32_t MAX_SHADOW_QUADS = 4096;
    static constexpr uint32_t MAX_SHADOW_VERTS = MAX_SHADOW_QUADS * 4;

    static std::vector<ShadowCaster2D> s_shadowCasters;
    static std::vector<bool> s_casterActive;
};

} // namespace Prisma::Graphic
