#include "Node.h"
#include "EntityManager.h"

namespace Prisma {

bool Node::IsValid() const {
    if (handle == 0) return false;
    uint32_t index = handle & 0xFFFF;
    uint32_t gen = handle >> 16;
    auto* rb = EntityManager::Get().GetRenderData();
    return index < EntityManager::Get().GetAliveCount() && 
           rb->active[index] != 0 && 
           (rb->generation[index] & 0xFFFF) == gen;
}

void Node::Destroy() {
    EntityManager::Get().DestroyNode(handle);
}

float Node::GetX() const {
    return EntityManager::Get().GetTransformRead()->posX[handle & 0xFFFF];
}
float Node::GetY() const {
    return EntityManager::Get().GetTransformRead()->posY[handle & 0xFFFF];
}
float Node::GetRotation() const {
    return EntityManager::Get().GetTransformRead()->rotation[handle & 0xFFFF];
}
Vector2 Node::GetScale() const {
    uint32_t idx = handle & 0xFFFF;
    auto* tb = EntityManager::Get().GetTransformRead();
    return { tb->scaleX[idx], tb->scaleY[idx] };
}

void Node::SetX(float x) {
    EntityManager::Get().GetTransformWrite()->posX[handle & 0xFFFF] = x;
}

void Node::SetY(float y) {
    EntityManager::Get().GetTransformWrite()->posY[handle & 0xFFFF] = y;
}

void Node::SetRotation(float rot) {
    EntityManager::Get().GetTransformWrite()->rotation[handle & 0xFFFF] = rot;
}

void Node::SetScale(const Vector2& scale) {
    uint32_t idx = handle & 0xFFFF;
    auto* tb = EntityManager::Get().GetTransformWrite();
    tb->scaleX[idx] = scale.x;
    tb->scaleY[idx] = scale.y;
}

} // namespace Prisma
