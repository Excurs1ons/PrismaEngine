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
struct RenderCommand;

/**
 * @brief 2D Blit Pass — Vertex2D (32B) 专用，替代 OpaquePass 在 Pipeline2D 中的角色。
 * 
 * 类型安全：仅接受 Vertex2D 顶点格式。
 * 不依赖 ForwardRenderPass 的 view/projection（那是 3D 概念）。
 */
class ENGINE_API BlitPass2D : public Pass2D {
public:
    BlitPass2D();
    ~BlitPass2D() override;

    bool Initialize(IRenderDevice* device, IRenderResourceManager* rm);

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
    bool createPSO(IRenderDevice* device, IRenderResourceManager* rm);

    std::shared_ptr<IPipelineState> m_pso;
    std::shared_ptr<IShader> m_vertShader;
    std::shared_ptr<IShader> m_fragShader;
};

} // namespace Prisma::Graphic
