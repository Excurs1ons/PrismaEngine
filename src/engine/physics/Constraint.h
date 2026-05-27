#pragma once

#include "physics/RigidBody.h"
#include <glm/glm.hpp>

namespace Prisma {
namespace Physics {

class IConstraint {
public:
    virtual ~IConstraint() = default;

    // 求解约束，返回本次迭代中应用的脉冲强度
    virtual double Solve(double dt) = 0;

    // 调试绘制（预留）
    virtual void DrawDebug() {}

    // 获取约束涉及的两个刚体
    virtual RigidBody* GetBodyA() const = 0;
    virtual RigidBody* GetBodyB() const = 0;

    // 约束是否有效
    virtual bool IsValid() const { return GetBodyA() != nullptr && GetBodyB() != nullptr; }
};

} // namespace Physics
} // namespace Prisma
