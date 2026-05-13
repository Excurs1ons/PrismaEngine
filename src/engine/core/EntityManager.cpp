#include "EntityManager.h"
#include "platform/Platform.h"
#include <cstring>
#include <algorithm>

namespace Prisma {

static constexpr size_t kFieldStride = kMaxVirtualEntities * sizeof(float);
static EntityManager* s_instance = nullptr;

EntityManager& EntityManager::Get() {
    return *s_instance;
}

EntityManager::EntityManager() {
    s_instance = this;

    size_t blockSizeA = 5 * kFieldStride;
    size_t blockSizeR = 8 * kFieldStride;

    m_blockABase = Platform::ReserveVirtualMemory(blockSizeA);
    m_blockBBase = Platform::ReserveVirtualMemory(blockSizeA);
    m_blockRBase = Platform::ReserveVirtualMemory(blockSizeR);

    initLayoutPointers();
}

EntityManager::~EntityManager() {
    if (m_blockABase) Platform::ReleaseVirtualMemory(m_blockABase, 5 * kFieldStride);
    if (m_blockBBase) Platform::ReleaseVirtualMemory(m_blockBBase, 5 * kFieldStride);
    if (m_blockRBase) Platform::ReleaseVirtualMemory(m_blockRBase, 8 * kFieldStride);
    s_instance = nullptr;
}

void EntityManager::initLayoutPointers() {
    auto initTransform = [](TransformBufferSoA& layout, void* base) {
        auto* bytes = (uint8_t*)base;
        layout.posX     = (float*)(bytes + 0 * kFieldStride);
        layout.posY     = (float*)(bytes + 1 * kFieldStride);
        layout.rotation = (float*)(bytes + 2 * kFieldStride);
        layout.scaleX   = (float*)(bytes + 3 * kFieldStride);
        layout.scaleY   = (float*)(bytes + 4 * kFieldStride);
    };

    auto initRender = [](RenderBufferSoA& layout, void* base) {
        auto* bytes = (uint8_t*)base;
        layout.active     = (uint32_t*)(bytes + 0 * kFieldStride);
        layout.generation = (uint32_t*)(bytes + 1 * kFieldStride);
        layout.colorR     = (float*)(bytes + 2 * kFieldStride);
        layout.colorG     = (float*)(bytes + 3 * kFieldStride);
        layout.colorB     = (float*)(bytes + 4 * kFieldStride);
        layout.colorA     = (float*)(bytes + 5 * kFieldStride);
        layout.sizeW      = (float*)(bytes + 6 * kFieldStride);
        layout.sizeH      = (float*)(bytes + 7 * kFieldStride);
    };

    initTransform(m_layoutA, m_blockABase);
    initTransform(m_layoutB, m_blockBBase);
    initRender(m_layoutR, m_blockRBase);
}

void EntityManager::commitRange(uint32_t fromEntity, uint32_t toEntity) {
    if (toEntity <= fromEntity) return;

    size_t start = (fromEntity * sizeof(float) / 4096) * 4096;
    size_t end   = ((toEntity   * sizeof(float) + 4095) / 4096) * 4096;
    size_t size  = end - start;
    if (size == 0) return;

    auto commitField = [&](void* base, uint32_t fieldCount) {
        for (uint32_t f = 0; f < fieldCount; f++) {
            void* addr = (uint8_t*)base + f * kFieldStride + start;
            Platform::CommitVirtualMemory(addr, size);
        }
    };

    commitField(m_blockABase, 5);
    commitField(m_blockBBase, 5);
    commitField(m_blockRBase, 8);
}

Node EntityManager::CreateNode() {
    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t index = 0xFFFFFFFF;
    for (uint32_t i = 0; i < m_aliveCount; ++i) {
        if (m_layoutR.active[i] == 0) {
            index = i;
            break;
        }
    }

    if (index == 0xFFFFFFFF) {
        index = m_aliveCount++;
        if (index >= m_committed) {
            uint32_t newCommit = ((index + 1 + kCommitStep - 1) / kCommitStep) * kCommitStep;
            if (newCommit > kMaxVirtualEntities) newCommit = kMaxVirtualEntities;
            commitRange(m_committed, newCommit);
            m_committed = newCommit;
        }
    }

    m_layoutR.active[index] = 1;
    if (m_layoutR.generation[index] == 0) m_layoutR.generation[index] = 1;

    // Reset transform (双缓冲：两个 layout 都要初始化)
    m_layoutA.posX[index] = m_layoutB.posX[index] = 0.0f;
    m_layoutA.posY[index] = m_layoutB.posY[index] = 0.0f;
    m_layoutA.rotation[index] = m_layoutB.rotation[index] = 0.0f;
    m_layoutA.scaleX[index] = m_layoutB.scaleX[index] = 1.0f;
    m_layoutA.scaleY[index] = m_layoutB.scaleY[index] = 1.0f;

    // Reset render
    m_layoutR.colorR[index] = m_layoutR.colorG[index] = m_layoutR.colorB[index] = m_layoutR.colorA[index] = 1.0f;
    m_layoutR.sizeW[index] = m_layoutR.sizeH[index] = 100.0f;

    uint32_t handle = (index & 0xFFFF) | ((m_layoutR.generation[index] & 0xFFFF) << 16);
    return Node(handle);
}

void EntityManager::DestroyNode(uint32_t handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t index = handle & 0xFFFF;
    uint32_t gen = handle >> 16;

    if (index < m_aliveCount && (m_layoutR.generation[index] & 0xFFFF) == gen) {
        m_layoutR.active[index] = 0;
        m_layoutR.generation[index]++;
        if ((m_layoutR.generation[index] & 0xFFFF) == 0) m_layoutR.generation[index] = 1;
    }
}

void EntityManager::SwapBuffers() {
    m_writeIndex = 1 - m_writeIndex;
}

TransformBufferSoA* EntityManager::GetTransformBufferRead() {
    return (m_writeIndex == 1) ? &m_layoutA : &m_layoutB;
}

TransformBufferSoA* EntityManager::GetTransformBufferWrite() {
    return (m_writeIndex == 0) ? &m_layoutA : &m_layoutB;
}

} // namespace Prisma
