#include "Renderer2D.h"
#include "core/EntityManager.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Prisma::Graphic {

void Renderer2D::DrawNodesSoA() {
    auto& em = EntityManager::Get();
    auto* rb = em.GetRenderData();
    auto* tb = em.GetTransformRead();
    uint32_t count = em.GetAliveCount();

    for (uint32_t i = 0; i < count; ++i) {
        if (!rb->active[i]) continue;

        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(tb->posX[i] + rb->sizeW[i] * 0.5f, tb->posY[i] + rb->sizeH[i] * 0.5f, 0.0f));
        if (std::abs(tb->rotation[i]) > 0.001f)
            t = glm::rotate(t, tb->rotation[i], glm::vec3(0, 0, 1));
        t = glm::scale(t, glm::vec3(rb->sizeW[i], rb->sizeH[i], 1.0f));

        Graphics2D::DrawQuad(t, {rb->colorR[i], rb->colorG[i], rb->colorB[i], rb->colorA[i]}, 0);
    }
}

void Renderer2D::DrawNode(Node node, int layer, const Prisma::Color& tint) {
    if (!node.IsValid()) return;
    uint32_t i = node.GetIndex();
    auto& em = EntityManager::Get();
    auto* rb = em.GetRenderData();
    auto* tb = em.GetTransformRead();

    Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(tb->posX[i] + rb->sizeW[i] * 0.5f, tb->posY[i] + rb->sizeH[i] * 0.5f, 0.0f));
    if (std::abs(tb->rotation[i]) > 0.001f)
        t = glm::rotate(t, tb->rotation[i], glm::vec3(0, 0, 1));
    t = glm::scale(t, glm::vec3(rb->sizeW[i], rb->sizeH[i], 1.0f));

    Prisma::Color color = { rb->colorR[i] * tint.r, rb->colorG[i] * tint.g, rb->colorB[i] * tint.b, rb->colorA[i] * tint.a };
    Graphics2D::DrawQuad(t, color, layer);
}

void Renderer2D::DrawLine(const Vector2& start, const Vector2& end, const Prisma::Color& color, float thickness, int layer) {
    Vector2 dir = end - start;
    float length = glm::length(dir);
    if (length < 0.0001f) return;

    Vector2 center = start + dir * 0.5f;
    float angle = std::atan2(dir.y, dir.x);

    Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(length, thickness, 1.0f));
    Graphics2D::DrawQuad(t, color, layer);
}

void Renderer2D::DrawRect(const Vector2& pos, const Vector2& size, const Prisma::Color& color, float thickness, int layer) {
    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;
    DrawLine({pos.x - hw, pos.y + hh}, {pos.x + hw, pos.y + hh}, color, thickness, layer);
    DrawLine({pos.x - hw, pos.y - hh}, {pos.x + hw, pos.y - hh}, color, thickness, layer);
    DrawLine({pos.x - hw, pos.y - hh}, {pos.x - hw, pos.y + hh}, color, thickness, layer);
    DrawLine({pos.x + hw, pos.y - hh}, {pos.x + hw, pos.y + hh}, color, thickness, layer);
}

} // namespace Prisma::Graphic
