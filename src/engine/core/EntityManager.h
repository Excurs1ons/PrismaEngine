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

    // ---- Double-Buffer Management ----
    // C++ 是唯一的交换权威。C# 每帧重新查询当前 Read/Write 指针。
    void SwapBuffers();
    TransformBufferSoA* GetTransformBufferRead();
    TransformBufferSoA* GetTransformBufferWrite();
    RenderBufferSoA*    GetRenderBuffer() { return &m_layoutR; }

    uint32_t GetAliveCount() const { return m_aliveCount; }
    uint32_t GetCommittedCount() const { return m_committed; }

private:
    void initLayoutPointers();
    void commitRange(uint32_t fromEntity, uint32_t toEntity);

    void* m_blockABase = nullptr;
    void* m_blockBBase = nullptr;
    void* m_blockRBase = nullptr;

    uint32_t m_aliveCount = 0;
    uint32_t m_committed = 0;

    TransformBufferSoA m_layoutA{};
    TransformBufferSoA m_layoutB{};
    RenderBufferSoA    m_layoutR{};

    int m_writeIndex = 0; // 0: Write→A, Read→B   1: Write→B, Read→A

    std::mutex m_mutex;
};

} // namespace Prisma
