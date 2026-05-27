#pragma once

#include "Export.h"
#include "Component.h"
#include "Skeleton.h"
#include "AnimationClip.h"
#include "AnimStateMachine.h"
#include "SkinningRenderer.h"

class FABRIKSolver;

#include <memory>
#include <string>

namespace Prisma {
namespace Animation {

class ENGINE_API AnimComponent : public Prisma::Component {
public:
    AnimComponent() = default;
    ~AnimComponent() override = default;

    AnimComponent(const AnimComponent&) = delete;
    AnimComponent& operator=(const AnimComponent&) = delete;

    ComponentId GetComponentId() const override {
        return GetComponentTypeId<AnimComponent>();
    }

    const char* GetComponentTypeName() const override {
        return "AnimComponent";
    }

    std::unique_ptr<Skeleton> skeleton;
    std::unique_ptr<AnimStateMachine> stateMachine;
    SkinningRenderer skinningRenderer;

    std::vector<BoneTransform> currentPose;

    bool enableIK = false;
    FABRIKSolver* ikSolver = nullptr;

    void AllocatePose() {
        if (skeleton) {
            currentPose.resize(skeleton->GetBoneCount());
        }
    }

    void SetSkeleton(std::unique_ptr<Skeleton> skel) {
        skeleton = std::move(skel);
        AllocatePose();
    }

    void SetStateMachine(std::unique_ptr<AnimStateMachine> sm) {
        stateMachine = std::move(sm);
    }
};

} // namespace Animation
} // namespace Prisma
