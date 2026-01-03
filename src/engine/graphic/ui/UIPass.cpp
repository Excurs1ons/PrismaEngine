//#include "UIPass.h"
//#include "FontAtlas.h"
//#include "Mesh.h"
//#include "math/MathTypes.h"
//
//// 使用 GLM 的正交投影函数
//namespace PrismaEngine::UIHelpers {
//    inline PrismaMath::mat4 OrthographicLH(float viewWidth, float viewHeight, float nearZ, float farZ) {
//        return glm::orthoLH(0.0f, viewWidth, 0.0f, viewHeight, nearZ, farZ);
//    }
//}
//
//namespace PrismaEngine {
//
//UIPass::UIPass()
//    : Graphic::LogicalPass("UIPass") {
//    // UI Pass 在所有 3D 渲染之后执行，使用较高的优先级值
//    m_priority = 1000;
//}
//
//    void UIPass::initialize(VkDevice device, VkRenderPass renderPass) {
//
//    }
//
//    void UIPass::cleanup(VkDevice device) {
//
//    }
//
//    void UIPass::Execute(const Graphic::PassExecutionContext& context) {
//    if (!context.deviceContext) {
//        return;
//    }
//
//    // 如果没有渲染内容，直接返回
//    if (m_renderQueue.empty()) {
//        return;
//    }
//
//    // 设置视口
//    context.deviceContext->SetViewport(0.0f, 0.0f,
//        static_cast<float>(context.sceneData->viewport.width),
//        static_cast<float>(context.sceneData->viewport.height));
//
//    // 创建正交投影矩阵 (左上角为原点)
//    PrismaMath::mat4 projectionMatrix = UIHelpers::OrthographicLH(
//        static_cast<float>(context.sceneData->viewport.width),
//        static_cast<float>(context.sceneData->viewport.height),
//        -1.0f,  // near
//        1.0f    // far
//    );
//
//    // 设置投影矩阵到常量缓冲区
//    context.deviceContext->SetConstantData(0, &projectionMatrix, sizeof(PrismaMath::mat4));
//
//    // 渲染所有项目
//    for (const auto& item : m_renderQueue) {
//        switch (item.type) {
//            case UIRenderItem::Type::Text: {
//                TextRendererComponent* textComp = item.textComponent;
//                if (!textComp || !textComp->GetFontAtlas()) {
//                    continue;
//                }
//
//                // 如果字体图集仍未上传，跳过
//                if (!textComp->GetFontAtlas()->IsUploaded()) {
//                    continue;
//                }
//
//                // 获取顶点和索引数据
//                const auto& vertices = textComp->GetVertices();
//                const auto& indices = textComp->GetIndices();
//
//                if (vertices.empty() || indices.empty()) {
//                    continue;
//                }
//
//                // 设置顶点缓冲区
//                context.deviceContext->SetVertexData(
//                    vertices.data(),
//                    static_cast<uint32_t>(vertices.size() * sizeof(Vertex)),
//                    static_cast<uint32_t>(sizeof(Vertex))
//                );
//
//                // 设置索引缓冲区
//                context.deviceContext->SetIndexData(
//                    indices.data(),
//                    static_cast<uint32_t>(indices.size() * sizeof(uint32_t)),
//                    false  // 使用 32 位索引
//                );
//
//                // 设置字体纹理
//                auto* fontTexture = textComp->GetFontAtlas()->GetTexture();
//                if (fontTexture) {
//                    context.deviceContext->SetTexture(fontTexture, 0);
//                }
//
//                // 设置文本颜色
//                const PrismaMath::vec4& color = textComp->GetColor();
//                context.deviceContext->SetConstantData(1, &color.x, sizeof(PrismaMath::vec4));
//
//                // 绘制
//                context.deviceContext->DrawIndexed(static_cast<uint32_t>(indices.size()));
//                break;
//            }
//            case UIRenderItem::Type::Component: {
//                if (item.uiComponent && item.uiComponent->IsVisible()) {
//                    RenderUIComponent(context, item.uiComponent);
//                }
//                break;
//            }
//        }
//    }
//}
//
//void UIPass::AddText(TextRendererComponent* text, const PrismaMath::mat4& transform) {
//    if (text) {
//        UIRenderItem item;
//        item.type = UIRenderItem::Type::Text;
//        item.textComponent = text;
//        item.transform = transform;
//        m_renderQueue.push_back(item);
//    }
//}
//
//void UIPass::AddUIComponent(UIComponent* component) {
//    if (component) {
//        UIRenderItem item;
//        item.type = UIRenderItem::Type::Component;
//        item.uiComponent = component;
//        m_renderQueue.push_back(item);
//    }
//}
//
//void UIPass::RenderUIComponent(const Graphic::PassExecutionContext& context, UIComponent* component) {
//    // 简单的矩形渲染（用于 Button 等组件）
//    // 使用屏幕坐标（考虑锚点）
//    const auto& pos = component->GetScreenPosition();
//    const auto& size = component->GetSize();
//    const auto& color = component->GetColor();
//
//    struct UIVertex {
//        PrismaMath::vec3 position;
//        PrismaMath::vec4 color;
//    };
//
//    UIVertex vertices[4] = {
//        {{pos.x, pos.y, 0.0f}, color},
//        {{pos.x + size.x, pos.y, 0.0f}, color},
//        {{pos.x + size.x, pos.y + size.y, 0.0f}, color},
//        {{pos.x, pos.y + size.y, 0.0f}, color}
//    };
//
//    uint32_t indices[6] = {0, 1, 2, 0, 2, 3};
//
//    // 设置顶点数据
//    context.deviceContext->SetVertexData(
//        vertices,
//        static_cast<uint32_t>(sizeof(vertices)),
//        static_cast<uint32_t>(sizeof(UIVertex))
//    );
//
//    // 设置索引数据
//    context.deviceContext->SetIndexData(
//        indices,
//        static_cast<uint32_t>(sizeof(indices)),
//        false
//    );
//
//    // 设置颜色
//    context.deviceContext->SetConstantData(1, &color.x, sizeof(PrismaMath::vec4));
//
//    // 绘制
//    context.deviceContext->DrawIndexed(6);
//}
//
//} // namespace PrismaEngine
