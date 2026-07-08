#pragma once

#include "forward/ForwardRenderPassBase.h"
#include "graphic/Mesh.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ITexture.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class IRenderDevice;

// 程序化物理天空 Pass (Rayleigh + Mie 单次散射)
// 独立于 SkyboxPass,使用自有 PSO + shader,不依赖外部 cubemap 绑定。
class ProceduralSkyPass : public ForwardRenderPass {
public:
    ProceduralSkyPass();
    ~ProceduralSkyPass() override = default;

    // IPass 接口
    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    // 初始化 PSO + shader(需 device,在管线 Initialize 阶段调用)
    bool Initialize(IRenderDevice* device);
    void Shutdown();

    // 设置太阳参数(从 RenderContext.lights 提取)
    void SetSunDirection(const PrismaMath::vec3& dir) { m_sunDirection = dir; }
    void SetSunColor(const PrismaMath::vec3& color) { m_sunColor = color; }
    void SetSunIntensity(float intensity) { m_sunIntensity = intensity; }

private:
    void InitializeSkyboxMesh();

    // PSO + shader
    IRenderDevice* m_device = nullptr;
    std::shared_ptr<IShader> m_vertexShader;
    std::shared_ptr<IShader> m_fragmentShader;
    std::shared_ptr<IPipelineState> m_pso;
    bool m_psoReady = false;

    // 立方体 mesh (复制自 SkyboxPass)
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    bool m_meshInitialized = false;

    // 太阳参数
    PrismaMath::vec3 m_sunDirection = {0.0f, -1.0f, 0.0f};
    PrismaMath::vec3 m_sunColor = {1.0f, 1.0f, 1.0f};
    float m_sunIntensity = 1.0f;

    // UBO 数据 (viewProj + sun params,与 shader SkyUBO 内存布局一致)
    struct SkyUBO {
        PrismaMath::mat4 viewProj;
        PrismaMath::vec4 sunDirIntensity;  // xyz=dir, w=intensity
        PrismaMath::vec4 sunColor;         // rgb=color, a=pad
    };
};

} // namespace Prisma::Graphic
