#pragma once
#include "ISubSystem.h"
#include "WorkerThread.h"
#include "core/Node.h"
#include "physics/CollisionSystem.h"
#include "physics/RigidBody.h"
#include "physics/Constraint.h"
#include "physics/ConstraintSolver.h"
#include "physics/TriggerManager.h"
#include "physics/CCDSolver.h"
#include "physics/SweepAndPrune.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Prisma {

namespace Physics {

/// 射线检测结果
struct RaycastResult {
    bool hit = false;               ///< 是否命中
    glm::dvec3 point{ 0.0 };       ///< 碰撞点（世界空间）
    glm::dvec3 normal{ 0.0, 1.0, 0.0 }; ///< 碰撞法线
    double distance = 0.0;          ///< 从射线起点到碰撞点的距离
    RigidBody* body = nullptr;      ///< 命中的刚体
};

} // namespace Physics



class ENGINE_API PhysicsSystem : public ISubSystem {
public:
    // ========== 子系统接口 ==========
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "PhysicsSystem"; }

    PhysicsSystem()           = default;
    ~PhysicsSystem() override = default;

    // ========== 刚体管理 ==========

    // 创建一个刚体并添加到系统
    Physics::RigidBody* createRigidBody(Physics::RigidBodyType type = Physics::RigidBodyType::Dynamic);

    // 移除刚体（通过指针）
    void destroyRigidBody(Physics::RigidBody* body);

    // 获取所有刚体
    const std::vector<std::unique_ptr<Physics::RigidBody>>& getBodies() const { return m_bodies; }

    // ========== 约束管理 ==========

    // 添加约束
    void addConstraint(Physics::IConstraint* constraint);

    // 移除约束
    void removeConstraint(Physics::IConstraint* constraint);

    // 获取所有约束
    const std::vector<Physics::IConstraint*>& getConstraints() const { return m_constraints; }

    // ========== 触发管理 ==========

    // 添加触发体积，返回 ID
    uint32_t addTrigger(const Physics::TriggerVolume& trigger);

    // 移除触发体积
    void removeTrigger(uint32_t triggerId);

    // 获取触发管理器
    Physics::TriggerManager& getTriggerManager() { return m_triggerManager; }
    const Physics::TriggerManager& getTriggerManager() const { return m_triggerManager; }

    // ========== 场景同步 ==========

    // 从场景中的 RigidBodyComponent + BoxColliderComponent 创建底层 RigidBody
    // 应在场景加载后调用一次
    void SyncFromScene();

    // 每帧同步底层 RigidBody 位置到场景 Node
    void SyncBodiesToNodes();

    // ========== 配置 ==========

    // 设置重力
    void setGravity(const glm::dvec3& gravity) { m_gravity = gravity; }
    const glm::dvec3& getGravity() const { return m_gravity; }

    // 设置求解器迭代次数
    void setSolverIterations(int iterations) { m_solver.m_iterations = std::max(1, iterations); }
    int getSolverIterations() const { return m_solver.m_iterations; }

    // CCD 启用/禁用
    void setCCDEnabled(bool enabled) { m_ccdEnabled = enabled; }
    bool isCCDEnabled() const { return m_ccdEnabled; }

    // ========== 射线检测 ==========

    /**
     * @brief 对场景中所有刚体执行射线检测
     * @param origin 射线起点
     * @param direction 射线方向（不需要归一化）
     * @param maxDistance 最大检测距离
     * @return 最近的命中结果（hit=false 表示无碰撞）
     */
    Physics::RaycastResult raycast(const glm::dvec3& origin, const glm::dvec3& direction, double maxDistance);

private:
    // ========== 内部物理步 ==========

    // 应用力（重力 + 用户力）
    void stepApplyForces(double dt);

    // 应用阻尼
    void stepApplyDamping(double dt);

    // 积分速度 (semi-implicit Euler velocity step)
    void stepIntegrateVelocity(double dt);

    // 连续碰撞检测
    void stepCCD(double dt);

    // 碰撞检测并生成接触对
    void stepCollide();

    // 求解约束
    void stepSolveConstraints(double dt);

    // 积分位置
    void stepIntegratePosition(double dt);

    // 更新触发体积
    void stepTriggers(double dt);

    // ========== 数据成员 ==========

    // 刚体容器
    std::vector<std::unique_ptr<Physics::RigidBody>> m_bodies;

    // 约束容器（原始指针，由用户管理生命周期或通过 unique_ptr 持有）
    std::vector<Physics::IConstraint*> m_constraints;

    // 需要销毁的约束列表
    std::vector<Physics::IConstraint*> m_pendingDestroyConstraints;

    // 约束求解器
    Physics::ConstraintSolver m_solver;

    // 连续碰撞检测求解器
    Physics::CCDSolver m_ccdSolver;

    // Sweep and Prune 宽相碰撞检测（按 X 轴排序）
    Physics::SweepAndPrune<0> m_sap;

    // 触发管理器
    Physics::TriggerManager m_triggerManager;

    // 物理配置
    glm::dvec3 m_gravity{ 0.0, -9.81, 0.0 };
    bool m_ccdEnabled = true;
    double m_ccdThreshold = 5.0; // 速度超过此值触发 CCD

    // 工作线程
    WorkerThread m_workerThread;

    // 场景 Node 映射（RigidBody → Node，用于每帧位置回写）
    std::unordered_map<Physics::RigidBody*, Node> m_nodeMap;
    bool m_hasSyncedFromScene = false;
};

}  // namespace Prisma
