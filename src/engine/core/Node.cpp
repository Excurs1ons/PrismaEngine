#include "Node.h"
#include "EntityManager.h"

namespace Prisma {

bool Node::IsValid() const {
    if (handle == 0) return false;
    uint32_t index = handle & 0xFFFF;
    uint32_t gen = handle >> 16;
    auto* rb = EntityManager::Get().GetRenderBuffer();
    return index < EntityManager::Get().GetAliveCount() && 
           rb->active[index] != 0 && 
           (rb->generation[index] & 0xFFFF) == gen;
}

void Node::Destroy() {
    EntityManager::Get().DestroyNode(handle);
}

float Node::GetX() const {
    return EntityManager::Get().GetTransformBufferRead()->posX[handle & 0xFFFF];
}

float Node::GetY() const {
    return EntityManager::Get().GetTransformBufferRead()->posY[handle & 0xFFFF];
}

float Node::GetRotation() const {
    return EntityManager::Get().GetTransformBufferRead()->rotation[handle & 0xFFFF];
}

Vector2 Node::GetScale() const {
    uint32_t idx = handle & 0xFFFF;
    auto* tb = EntityManager::Get().GetTransformBufferRead();
    return { tb->scaleX[idx], tb->scaleY[idx] };
}

void Node::SetX(float x) {
    EntityManager::Get().GetTransformBufferWrite()->posX[handle & 0xFFFF] = x;
}

void Node::SetY(float y) {
    EntityManager::Get().GetTransformBufferWrite()->posY[handle & 0xFFFF] = y;
}

void Node::SetRotation(float rot) {
    EntityManager::Get().GetTransformBufferWrite()->rotation[handle & 0xFFFF] = rot;
}

void Node::SetScale(const Vector2& scale) {
    uint32_t idx = handle & 0xFFFF;
    auto* tb = EntityManager::Get().GetTransformBufferWrite();
    tb->scaleX[idx] = scale.x;
    tb->scaleY[idx] = scale.y;
}

} // namespace Prisma
