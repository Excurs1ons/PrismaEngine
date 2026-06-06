#pragma once

#include "Node.h"
#include <vector>
#include <mutex>

namespace Prisma {

class ENGINE_API EntityManager {
public:
    EntityManager();
    ~EntityManager();

    static EntityManager& Get();

    Node CreateNode();
    void DestroyNode(uint32_t handle);

    TransformDataLayout* GetTransform() { return &m_layout; }
    TransformDataLayout* GetTransformRead() { return &m_layout; }
    TransformDataLayout* GetTransformWrite() { return &m_layout; }
    RenderDataLayout*    GetRenderData() { return &m_layoutR; }

    uint32_t GetAliveCount() const { return m_aliveCount; }
    uint32_t GetCommittedCount() const { return m_committed; }

private:
    void initLayoutPointers();
    void commitRange(uint32_t fromEntity, uint32_t toEntity);

    void* m_blockBase = nullptr;
    void* m_blockRBase = nullptr;

    uint32_t m_aliveCount = 0;
    uint32_t m_committed = 0;

    TransformDataLayout m_layout{};
    RenderDataLayout    m_layoutR{};

    std::mutex m_mutex;
};

} // namespace Prisma
