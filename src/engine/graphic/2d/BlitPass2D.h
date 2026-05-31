#pragma once

#include "Pass2D.h"
#include "graphic/interfaces/RenderTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class IPipelineState;
class IShader;
class ICommandBuffer;
class IRenderDevice;
class IRenderResourceManager;
class IDescriptorSet;
class IDescriptorSetLayout;
class IBuffer;
class ITexture;
struct RenderCommand;

/**
 * @brief 2D Blit Pass — Vertex2D (32B) 专用，替代 OpaquePass 在 Pipeline2D 中的角色。
 * 
 * 类型安全：仅接受 Vertex2D 顶点格式。
 * 自管 descriptor set，不依赖 Material::Bind 的布局兼容性。
 */
class ENGINE_API BlitPass2D : public Pass2D {
public:
    BlitPass2D();
    ~BlitPass2D() override;

    bool Initialize(IRenderDevice* device, IRenderResourceManager* rm, TextureFormat rtFormat = TextureFormat::RGBA8_UNorm);

    /**
     * @brief 绘制 Renderer2D 提交的合批 quad 命令队列
     * @param mvp 正交投影 MVP 矩阵
     */
    void Draw(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands,
              const PrismaMath::mat4& mvp);

    // Pass2D overrides
    void Update(Timestep ts) override { UpdateTime(ts); }
    void Execute(const PassExecutionContext& context) override {}

private:
    bool createPSO(IRenderDevice* device, IRenderResourceManager* rm, TextureFormat rtFormat);
    bool createDescriptorSet(IRenderDevice* device, IRenderResourceManager* rm);

    std::shared_ptr<IPipelineState> m_pso;
    std::shared_ptr<IShader> m_vertShader;
    std::shared_ptr<IShader> m_fragShader;

    // 自管 descriptor set (UnlitSprite.frag: set=0 binding=0 UBO, binding=1 sampler2D)
    std::shared_ptr<IBuffer> m_materialUBO;
    std::shared_ptr<IDescriptorSetLayout> m_dsLayout;
    std::shared_ptr<IDescriptorSet> m_ds;
    std::shared_ptr<ITexture> m_defaultTexture;
};

} // namespace Prisma::Graphic
