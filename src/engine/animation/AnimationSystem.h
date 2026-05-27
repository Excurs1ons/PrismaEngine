#pragma once

#include "Export.h"
#include "ISubSystem.h"
#include "Skeleton.h"
#include "AnimationClip.h"
#include "AnimStateMachine.h"
#include "AnimComponent.h"
#include "SkinningRenderer.h"
#include "core/Timestep.h"

#include <vector>
#include <memory>
#include <unordered_set>

namespace Prisma {
namespace Animation {

class ENGINE_API AnimationSystem : public ISubSystem {
public:
    AnimationSystem() = default;
    ~AnimationSystem() override = default;

    AnimationSystem(const AnimationSystem&) = delete;
    AnimationSystem& operator=(const AnimationSystem&) = delete;

    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "AnimationSystem"; }

    void RegisterComponent(AnimComponent* component);
    void UnregisterComponent(AnimComponent* component);

    size_t GetActiveComponentCount() const { return m_components.size(); }

private:
    void UpdateSkeletonPose(AnimComponent& component, double deltaTime);
    void UpdateSkinningMatrices(AnimComponent& component);

    std::vector<AnimComponent*> m_components;
    std::vector<glm::dmat4> m_skinningMatrices;
};

inline int AnimationSystem::Initialize() {
    m_components.clear();
    m_skinningMatrices.clear();
    return 0;
}

inline void AnimationSystem::Shutdown() {
    m_components.clear();
    m_skinningMatrices.clear();
}

inline void AnimationSystem::RegisterComponent(AnimComponent* component) {
    if (component) {
        m_components.push_back(component);
    }
}

inline void AnimationSystem::UnregisterComponent(AnimComponent* component) {
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        if (*it == component) {
            m_components.erase(it);
            return;
        }
    }
}

inline void AnimationSystem::Update(Timestep ts) {
    double deltaTime = static_cast<double>(ts);

    for (auto* component : m_components) {
        if (!component || !component->IsEnabled()) continue;
        UpdateSkeletonPose(*component, deltaTime);
    }
}

inline void AnimationSystem::UpdateSkeletonPose(
    AnimComponent& component, double deltaTime)
{
    if (!component.skeleton || !component.stateMachine) return;

    size_t boneCount = component.skeleton->GetBoneCount();
    if (component.currentPose.size() != boneCount) {
        component.currentPose.resize(boneCount);
    }

    component.stateMachine->Update(deltaTime, component.currentPose);

    for (size_t i = 0; i < boneCount; ++i) {
        auto& bone = component.skeleton->GetBone(static_cast<uint32_t>(i));
        bone.localTransform = component.currentPose[i].ToMatrix();
    }

    UpdateSkinningMatrices(component);
}

inline void AnimationSystem::UpdateSkinningMatrices(
    AnimComponent& component)
{
    if (!component.skeleton) return;
    size_t boneCount = component.skeleton->GetBoneCount();
    if (boneCount == 0) return;

    m_skinningMatrices.resize(boneCount);

    auto finalMatrices = component.skeleton->ComputeFinalBoneMatrices();
    auto skinningMatrices = component.skeleton->ComputeSkinningMatrices();

    component.skinningRenderer.UpdateBoneTransforms(
        skinningMatrices.data(),
        static_cast<uint32_t>(skinningMatrices.size()));
}

} // namespace Animation
} // namespace Prisma
