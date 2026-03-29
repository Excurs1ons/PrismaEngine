#include "Renderer2D.h"
#include "OrthographicCamera.h"
#include "interfaces/ITexture.h"
#include <algorithm>

namespace Prisma {
namespace Graphic {

struct QuadVertex {
    Vector2 Position;
    Vector2 TexCoord;
    Color TintColor;
};

struct Renderer2DStorage {
    static const uint32_t MaxQuads = 10000;
    static const uint32_t MaxVertices = MaxQuads * 4;
    static const uint32_t MaxIndices = MaxQuads * 6;

    uint32_t QuadIndexCount = 0;
    std::vector<QuadVertex> QuadVertices;
    std::shared_ptr<ITexture> CurrentTexture;

    Renderer2D::Statistics Stats;
};

static Renderer2DStorage* s_Data = nullptr;

void Renderer2D::Initialize() {
    s_Data = new Renderer2DStorage();
    s_Data->QuadVertices.reserve(Renderer2DStorage::MaxVertices);
}

void Renderer2D::Shutdown() {
    delete s_Data;
    s_Data = nullptr;
}

void Renderer2D::BeginScene(const OrthographicCamera& /*camera*/) {
    StartBatch();
}

void Renderer2D::EndScene() {
    Flush();
}

void Renderer2D::Flush() {
    if (s_Data->QuadIndexCount == 0) {
        return;
    }

    // 实际的渲染会由具体的后端系统执行
    // 这里暂时只做统计
    s_Data->Stats.DrawCalls++;
}

void Renderer2D::StartBatch() {
    s_Data->QuadIndexCount = 0;
    s_Data->QuadVertices.clear();
    s_Data->CurrentTexture = nullptr;
}

void Renderer2D::NextBatch() {
    Flush();
    StartBatch();
}

void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Prisma::Color& color) {
    DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f)), color);
}

void Renderer2D::DrawQuad(const Matrix4& /*transform*/, const Prisma::Color& /*color*/) {
    // 提交矩形绘制指令
    s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Prisma::Color& tintColor) {
    DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f)), texture, tintColor);
}

void Renderer2D::DrawQuad(const Matrix4& /*transform*/, const std::shared_ptr<ITexture>& /*texture*/, const Prisma::Color& /*tintColor*/) {
    s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], const Prisma::Color& tintColor) {
    DrawQuad(glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f)), texture, uv, tintColor);
}

void Renderer2D::DrawQuad(const Matrix4& /*transform*/, const std::shared_ptr<ITexture>& /*texture*/, const Vector2 /*uv*/[4], const Prisma::Color& /*tintColor*/) {
    s_Data->Stats.QuadCount++;
}

void Renderer2D::ResetStats() {
    s_Data->Stats = Statistics();
}

Renderer2D::Statistics Renderer2D::GetStats() {
    return s_Data->Stats;
}

} // namespace Graphic
} // namespace Prisma
