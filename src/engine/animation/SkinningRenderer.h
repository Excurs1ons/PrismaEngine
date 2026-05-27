#pragma once

#include "Export.h"
#include <vector>
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Prisma {
namespace Animation {

static constexpr uint32_t MAX_BONES = 64;

struct ENGINE_API SkinnedVertex {
    glm::dvec3 position{0.0};
    glm::dvec3 normal{0.0, 1.0, 0.0};
    glm::dvec2 uv{0.0};
    int32_t    boneIndices[4] = {0, 0, 0, 0};
    float      boneWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct SkinnedMeshSection {
    uint32_t vertexOffset = 0;
    uint32_t vertexCount  = 0;
    uint32_t indexOffset  = 0;
    uint32_t indexCount   = 0;
    uint32_t materialIndex = 0;
};

struct ENGINE_API SkinnedMesh {
    std::vector<SkinnedVertex> vertices;
    std::vector<uint32_t>      indices;
    std::vector<SkinnedMeshSection> sections;

    bool IsValid() const { return !vertices.empty() && !indices.empty(); }
};

struct BoneUBO {
    glm::mat4 boneMatrices[MAX_BONES];
};

class ENGINE_API SkinningRenderer {
public:
    SkinningRenderer() = default;
    ~SkinningRenderer() = default;

    SkinningRenderer(const SkinningRenderer&) = delete;
    SkinningRenderer& operator=(const SkinningRenderer&) = delete;
    SkinningRenderer(SkinningRenderer&&) noexcept = default;
    SkinningRenderer& operator=(SkinningRenderer&&) noexcept = default;

    void Initialize();
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

    void UpdateBoneTransforms(const glm::dmat4* skinningMatrices, uint32_t boneCount);

    const BoneUBO& GetBoneUBO() const { return m_boneUBO; }
    const float*   GetBoneData() const { return &m_boneUBO.boneMatrices[0][0][0]; }

    uint32_t GetActiveBoneCount() const { return m_activeBoneCount; }

private:
    BoneUBO  m_boneUBO{};
    uint32_t m_activeBoneCount = 0;
    bool     m_initialized = false;
};

inline void SkinningRenderer::Initialize() {
    for (uint32_t i = 0; i < MAX_BONES; ++i) {
        m_boneUBO.boneMatrices[i] = glm::mat4(1.0f);
    }
    m_activeBoneCount = 0;
    m_initialized = true;
}

inline void SkinningRenderer::Shutdown() {
    m_initialized = false;
    m_activeBoneCount = 0;
}

inline void SkinningRenderer::UpdateBoneTransforms(
    const glm::dmat4* skinningMatrices, uint32_t boneCount)
{
    if (!m_initialized) return;
    m_activeBoneCount = std::min(boneCount, MAX_BONES);
    for (uint32_t i = 0; i < m_activeBoneCount; ++i) {
        m_boneUBO.boneMatrices[i] = glm::mat4(skinningMatrices[i]);
    }
    for (uint32_t i = m_activeBoneCount; i < MAX_BONES; ++i) {
        m_boneUBO.boneMatrices[i] = glm::mat4(1.0f);
    }
}

} // namespace Animation
} // namespace Prisma
