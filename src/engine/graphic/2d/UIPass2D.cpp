#include "UIPass2D.h"
#include "graphic/Renderer2D.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"

namespace Prisma::Graphic {

UIPass2D::UIPass2D()
    : ForwardRenderPass("UIPass2D") {
    m_priority = 300; // 最后渲染
    m_uiCamera = std::make_shared<OrthographicCamera>(0, 1920, 1080, 0); // 修正：bottom=1080, top=0 -> Y向下
}

UIPass2D::~UIPass2D() {}

void UIPass2D::Update(Prisma::Timestep ts) {
    UpdateTime(ts);
}

void UIPass2D::Execute([[maybe_unused]] const PassExecutionContext& context) {
    // 无需在此处执行逻辑：该 Pass 的渲染由 Pipeline 通过 RenderUI() 显式调用
}

void UIPass2D::RenderUI(ICommandBuffer* cmd, IRenderDevice* device, uint32_t width, uint32_t height) {
    if (!cmd || !device) return;

    // 更新 UI 相机矩阵以匹配当前视口 (Top-Down)
    m_uiCamera->SetProjection(0, (float)width, (float)height, 0); 
    
    // 开始 UI 绘制模式 (独立于场景的合批)
    Renderer2D::BeginUI();
    
    // 这里可以提交 UI 渲染命令
    // 例如: Renderer2D::DrawQuad(...)
    
    Renderer2D::EndUI();
}

} // namespace Prisma::Graphic
