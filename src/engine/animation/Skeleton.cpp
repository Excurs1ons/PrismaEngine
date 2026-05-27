#include "animation/Skeleton.h"
#include <algorithm>
#include <stdexcept>

namespace Prisma {
namespace Animation {

uint32_t Skeleton::AddBone(const std::string& name, int32_t parentIndex) {
    uint32_t index = static_cast<uint32_t>(m_bones.size());
    m_bones.emplace_back(name, index, parentIndex);
    m_nameToIndex[name] = index;
    return index;
}

int32_t Skeleton::GetBoneIndex(const std::string& name) const {
    auto it = m_nameToIndex.find(name);
    if (it != m_nameToIndex.end()) {
        return static_cast<int32_t>(it->second);
    }
    return -1;
}

Bone& Skeleton::GetBone(uint32_t index) {
    if (index >= m_bones.size()) {
        throw std::out_of_range("Bone index out of range: " + std::to_string(index));
    }
    return m_bones[index];
}

const Bone& Skeleton::GetBone(uint32_t index) const {
    if (index >= m_bones.size()) {
        throw std::out_of_range("Bone index out of range: " + std::to_string(index));
    }
    return m_bones[index];
}

void Skeleton::SetInverseBindMatrix(uint32_t index, const glm::dmat4& invBind) {
    if (index >= m_bones.size()) {
        throw std::out_of_range("Bone index out of range: " + std::to_string(index));
    }
    m_bones[index].inverseBindMatrix = invBind;
}

std::vector<glm::dmat4> Skeleton::ComputeFinalBoneMatrices() const {
    std::vector<glm::dmat4> finalMatrices;
    finalMatrices.resize(m_bones.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_bones.size()); ++i) {
        const Bone& bone = m_bones[i];

        // 从当前骨骼的局部变换开始
        glm::dmat4 finalMatrix = bone.localTransform;

        // 累积父级变换（从父到根）
        int32_t parentIdx = bone.parentIndex;
        while (parentIdx >= 0 && parentIdx < static_cast<int32_t>(m_bones.size())) {
            finalMatrix = m_bones[static_cast<uint32_t>(parentIdx)].localTransform * finalMatrix;
            parentIdx = m_bones[static_cast<uint32_t>(parentIdx)].parentIndex;
        }

        finalMatrices[i] = finalMatrix;
    }

    return finalMatrices;
}

std::vector<glm::dmat4> Skeleton::ComputeSkinningMatrices() const {
    std::vector<glm::dmat4> finalMatrices = ComputeFinalBoneMatrices();
    std::vector<glm::dmat4> skinningMatrices;
    skinningMatrices.resize(finalMatrices.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(finalMatrices.size()); ++i) {
        skinningMatrices[i] = finalMatrices[i] * m_bones[i].inverseBindMatrix;
    }

    return skinningMatrices;
}

void Skeleton::ResetPose() {
    for (auto& bone : m_bones) {
        bone.localTransform = glm::dmat4(1.0);
    }
}

} // namespace Animation
} // namespace Prisma
