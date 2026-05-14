#include "SpriteRendererComponent.h"
#include "EntityManager.h"

namespace Prisma {
namespace Core {

void SpriteRendererComponent::WriteToSoA(uint32_t entityIndex) const {
    auto& em = EntityManager::Get();
    auto* rb = em.GetRenderData();
    rb->colorR[entityIndex] = color.r;
    rb->colorG[entityIndex] = color.g;
    rb->colorB[entityIndex] = color.b;
    rb->colorA[entityIndex] = color.a;
    rb->sizeW[entityIndex] = size.x;
    rb->sizeH[entityIndex] = size.y;
}

} // namespace Core
} // namespace Prisma
