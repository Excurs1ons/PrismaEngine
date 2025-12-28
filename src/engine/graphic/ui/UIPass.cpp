#include "UIPass.h"
#include "FontAtlas.h"
#include "Mesh.h"
#include "math/Math.h"

namespace PrismaEngine {

UIPass::UIPass()
    : Graphic::LogicalPass("UIPass") {
    // UI Pass 在所有 3D 渲染之后执行，使用较高的优先级值
    m_priority = 1000;
}

void UIPass::Execute(const Graphic::PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    // 如果没有渲染内容，直接返回
    if (m_renderQueue.empty()) {
        return;
    }

    // 设置视口
    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    // 创建正交投影矩阵 (左上角为原点)
    PrismaMath::mat4 projectionMatrix = Prisma::OrthographicLH(
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height),
        -1.0f,  // near
        1.0f    // far
    );

    // 设置投影矩阵到常量缓冲区
    context.deviceContext->SetConstantData(0, &projectionMatrix, sizeof(PrismaMath::mat4));

    // 渲染所有文本
    for (const auto& item : m_renderQueue) {
        TextRendererComponent* textComp = item.textComponent;
        if (!textComp || !textComp->GetFontAtlas()) {
            continue;
        }

        // 确保字体图集已上传到 GPU
        // 注意：这里需要通过 IRenderDevice 上传，后续实现
        // if (!textComp->GetFontAtlas()->IsUploaded()) {
        //     textComp->GetFontAtlas()->UploadToGPU(device);
        // }

        // 如果字体图集仍未上传，跳过
        if (!textComp->GetFontAtlas()->IsUploaded()) {
            continue;
        }

        // 获取顶点和索引数据
        const auto& vertices = textComp->GetVertices();
        const auto& indices = textComp->GetIndices();

        if (vertices.empty() || indices.empty()) {
            continue;
        }

        // 设置顶点缓冲区
        context.deviceContext->SetVertexData(
            vertices.data(),
            static_cast<uint32_t>(vertices.size() * sizeof(Vertex)),
            static_cast<uint32_t>(sizeof(Vertex))
        );

        // 设置索引缓冲区
        context.deviceContext->SetIndexData(
            indices.data(),
            static_cast<uint32_t>(indices.size() * sizeof(uint32_t)),
            false  // 使用 32 位索引
        );

        // 设置字体纹理
        auto* fontTexture = textComp->GetFontAtlas()->GetTexture();
        if (fontTexture) {
            context.deviceContext->SetTexture(fontTexture, 0);
        }

        // 设置文本颜色
        const PrismaMath::vec4& color = textComp->GetColor();
        context.deviceContext->SetConstantData(1, &color.x, sizeof(PrismaMath::vec4));

        // 绘制
        context.deviceContext->DrawIndexed(static_cast<uint32_t>(indices.size()));
    }
}

void UIPass::AddText(TextRendererComponent* text, const PrismaMath::mat4& transform) {
    if (text) {
        UIRenderItem item;
        item.textComponent = text;
        item.transform = transform;
        m_renderQueue.push_back(item);
    }
}

} // namespace PrismaEngine
