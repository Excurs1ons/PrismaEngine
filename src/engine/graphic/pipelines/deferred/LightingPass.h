#pragma once

#include "Export.h"
#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IGBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class LightingPass : public LogicalPass {
public:
    enum class LightType : uint32_t {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    struct Light {
        LightType type = LightType::Point;
        PrismaMath::vec3 position = {0.0f, 0.0f, 0.0f};
        PrismaMath::vec3 direction = {0.0f, -1.0f, 0.0f};
        PrismaMath::vec3 color = {1.0f, 1.0f, 1.0f};
        float intensity = 1.0f;
        float range = 10.0f;
        float innerCone = 0.5f;
        float outerCone = 1.0f;
        bool castShadows = false;
        uint32_t shadowMapIndex = 0xFFFFFFFF;
        PrismaMath::mat4 shadowMatrix = PrismaMath::mat4(1.0f);
    };

    struct RenderStats {
        uint32_t lightsRendered = 0;
        uint32_t shadowCastingLights = 0;
    };

    LightingPass();
    ~LightingPass() override = default;

    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    void Execute(ICommandBuffer* cmd, IRenderDevice* device);

    void SetGBuffer(IGBuffer* gBuffer) { m_gBuffer = gBuffer; }
    IGBuffer* GetGBuffer() const { return m_gBuffer; }

    void SetAmbientLight(const PrismaMath::vec3& ambient) { m_ambientLight = ambient; }
    const PrismaMath::vec3& GetAmbientLight() const { return m_ambientLight; }

    void SetLights(const std::vector<Light>& lights) { m_lights = lights; }
    const std::vector<Light>& GetLights() const { return m_lights; }
    void AddLight(const Light& light) { m_lights.push_back(light); }
    void ClearLights() { m_lights.clear(); }

    void SetIBL(bool enable) { m_iblEnabled = enable; }
    bool GetIBL() const { return m_iblEnabled; }
    void SetIBLTextures(ITexture* irradianceMap, ITexture* prefilterMap, ITexture* brdfLUT);

    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }
    void ResetStats() { m_stats = RenderStats(); }

private:
    bool EnsureDefaultPipeline(IRenderDevice* device);

    IGBuffer* m_gBuffer;
    PrismaMath::vec3 m_ambientLight;
    std::vector<Light> m_lights;
    bool m_iblEnabled;
    ITexture* m_irradianceMap;
    ITexture* m_prefilterMap;
    ITexture* m_brdfLUT;
    RenderStats m_stats;

    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_fragmentShader;
    std::shared_ptr<IPipelineState> m_pipelineState;
};

} // namespace Prisma::Graphic
