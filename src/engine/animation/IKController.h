#pragma once

#include "Export.h"
#include "Skeleton.h"
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace Prisma {
namespace Animation {

struct ENGINE_API IKBoneChain {
    std::vector<uint32_t> boneIndices;
    glm::dvec3 targetPosition{0.0};
    uint32_t   maxIterations = 20;
    double     tolerance = 0.01;
};

class ENGINE_API FABRIKSolver {
public:
    FABRIKSolver() = default;

    void SetChain(const IKBoneChain& chain) { m_chain = chain; }
    const IKBoneChain& GetChain() const { return m_chain; }

    bool Solve(Skeleton& skeleton);

    bool HasConverged() const { return m_converged; }
    double GetError() const { return m_error; }

private:
    glm::dvec3 GetBoneWorldPosition(const Skeleton& skeleton, uint32_t boneIndex) const;
    void SetBoneWorldRotation(Skeleton& skeleton, uint32_t boneIndex,
                              const glm::dvec3& fromDir,
                              const glm::dvec3& toDir);

    IKBoneChain m_chain;
    bool   m_converged = false;
    double m_error = 0.0;
};

inline glm::dvec3 FABRIKSolver::GetBoneWorldPosition(
    const Skeleton& skeleton, uint32_t boneIndex) const
{
    const auto& bones = skeleton.GetBones();
    if (boneIndex >= bones.size()) return glm::dvec3(0.0);

    glm::dmat4 worldMatrix = bones[boneIndex].localTransform;
    int32_t parentIdx = bones[boneIndex].parentIndex;
    while (parentIdx >= 0 && static_cast<size_t>(parentIdx) < bones.size()) {
        worldMatrix = bones[static_cast<size_t>(parentIdx)].localTransform * worldMatrix;
        parentIdx = bones[static_cast<size_t>(parentIdx)].parentIndex;
    }
    return glm::dvec3(worldMatrix[3]);
}

inline void FABRIKSolver::SetBoneWorldRotation(
    Skeleton& skeleton, uint32_t boneIndex,
    const glm::dvec3& fromDir, const glm::dvec3& toDir)
{
    if (boneIndex >= skeleton.GetBoneCount()) return;
    glm::dvec3 f = glm::normalize(fromDir);
    glm::dvec3 t = glm::normalize(toDir);

    double dot = glm::clamp(glm::dot(f, t), -1.0, 1.0);
    if (glm::abs(dot - 1.0) < 1e-8) return;
    if (glm::abs(dot + 1.0) > 1.0 - 1e-8) {
        glm::dquat rotation = glm::angleAxis(glm::pi<double>(),
            glm::normalize(glm::cross(f, glm::dvec3(0.0, 1.0, 0.0))));
        if (glm::length(glm::cross(f, glm::dvec3(0.0, 1.0, 0.0))) < 1e-6) {
            rotation = glm::angleAxis(glm::pi<double>(),
                glm::normalize(glm::cross(f, glm::dvec3(1.0, 0.0, 0.0))));
        }
        skeleton.GetBone(boneIndex).localTransform =
            glm::toMat4(rotation) * skeleton.GetBone(boneIndex).localTransform;
        return;
    }

    glm::dquat rotation = glm::rotation(f, t);
    skeleton.GetBone(boneIndex).localTransform =
        glm::toMat4(rotation) * skeleton.GetBone(boneIndex).localTransform;
}

inline bool FABRIKSolver::Solve(Skeleton& skeleton) {
    m_converged = false;
    m_error = 0.0;

    if (m_chain.boneIndices.size() < 2) return false;

    size_t chainLen = m_chain.boneIndices.size();
    std::vector<glm::dvec3> positions(chainLen);

    for (size_t i = 0; i < chainLen; ++i) {
        positions[i] = GetBoneWorldPosition(skeleton, m_chain.boneIndices[i]);
    }

    std::vector<double> boneLengths(chainLen - 1);
    for (size_t i = 0; i < chainLen - 1; ++i) {
        boneLengths[i] = glm::distance(positions[i], positions[i + 1]);
    }

    double totalLength = 0.0;
    for (const auto& len : boneLengths) totalLength += len;

    glm::dvec3 rootPos = positions[0];
    double targetDist = glm::distance(rootPos, m_chain.targetPosition);

    bool reached = false;
    for (uint32_t iter = 0; iter < m_chain.maxIterations; ++iter) {
        if (targetDist > totalLength) {
            for (size_t i = 0; i < chainLen - 1; ++i) {
                glm::dvec3 dir = glm::normalize(m_chain.targetPosition - positions[i]);
                positions[i + 1] = positions[i] + dir * boneLengths[i];
            }
            reached = true;
            break;
        }

        positions[chainLen - 1] = m_chain.targetPosition;
        for (size_t i = chainLen - 1; i > 0; --i) {
            glm::dvec3 dir = glm::normalize(positions[i - 1] - positions[i]);
            positions[i - 1] = positions[i] + dir * boneLengths[i - 1];
        }

        positions[0] = rootPos;
        for (size_t i = 0; i < chainLen - 1; ++i) {
            glm::dvec3 dir = glm::normalize(positions[i + 1] - positions[i]);
            positions[i + 1] = positions[i] + dir * boneLengths[i];
        }

        m_error = glm::distance(positions[chainLen - 1], m_chain.targetPosition);
        if (m_error < m_chain.tolerance) {
            reached = true;
            break;
        }
    }

    for (size_t i = 0; i < chainLen - 1; ++i) {
        glm::dvec3 fromDir = glm::normalize(GetBoneWorldPosition(skeleton, m_chain.boneIndices[i + 1])
                                           - GetBoneWorldPosition(skeleton, m_chain.boneIndices[i]));
        glm::dvec3 toDir   = glm::normalize(positions[i + 1] - positions[i]);
        SetBoneWorldRotation(skeleton, m_chain.boneIndices[i], fromDir, toDir);
    }

    m_converged = reached;
    return m_converged;
}

} // namespace Animation
} // namespace Prisma
