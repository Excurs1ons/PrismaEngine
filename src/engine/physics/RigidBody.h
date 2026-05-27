#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CollisionSystem.h"
#include <memory>
#include <cstdint>

namespace Prisma {
    namespace Physics {

        // 刚体类型枚举
        enum class RigidBodyType {
            Static,     // 静态刚体（不受力影响，不可移动）
            Dynamic,    // 动态刚体（受力和力矩影响，完全模拟）
            Kinematic   // 运动学刚体（可移动但不受力影响，动画控制）
        };

        // 碰撞标志枚举
        enum class CollisionFlags : uint32_t {
            None = 0,
            Static = 1 << 0,
            Dynamic = 1 << 1,
            Kinematic = 1 << 2,
            All = Static | Dynamic | Kinematic
        };

        inline CollisionFlags operator|(CollisionFlags a, CollisionFlags b) {
            return static_cast<CollisionFlags>(
                static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
            );
        }

        inline CollisionFlags operator&(CollisionFlags a, CollisionFlags b) {
            return static_cast<CollisionFlags>(
                static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
            );
        }

        /* 运动状态 - 存储前一帧变换用于渲染插值 */
        struct MotionState {
            glm::dvec3 previousPosition{ 0.0 };
            glm::dquat previousRotation{ 1.0, 0.0, 0.0, 0.0 };
            glm::dvec3 currentPosition{ 0.0 };
            glm::dquat currentRotation{ 1.0, 0.0, 0.0, 0.0 };

            void storeCurrentState() {
                previousPosition = currentPosition;
                previousRotation = currentRotation;
            }

            void setCurrentState(const glm::dvec3& pos, const glm::dquat& rot) {
                currentPosition = pos;
                currentRotation = rot;
            }

            glm::dmat4 getInterpolatedTransform(float alpha) const {
                double a = static_cast<double>(alpha);
                glm::dvec3 pos = glm::mix(previousPosition, currentPosition, a);
                glm::dquat rot = glm::slerp(previousRotation, currentRotation, a);
                glm::dmat4 t = glm::translate(glm::dmat4(1.0), pos);
                return t * glm::mat4_cast(rot);
            }
        };

        /* 刚体类 */
        class RigidBody {
        public:
            // ========== 构造函数 ==========

            RigidBody();
            RigidBody(RigidBodyType type);
            ~RigidBody() = default;

            // ========== 禁止拷贝 ==========

            RigidBody(const RigidBody&) = delete;
            RigidBody& operator=(const RigidBody&) = delete;

            // ========== 允许移动 ==========

            RigidBody(RigidBody&&) = default;
            RigidBody& operator=(RigidBody&&) = default;

            // ========== 状态属性 ==========

            // 获取刚体类型
            RigidBodyType getType() const { return m_type; }

            // 设置刚体类型
            void setType(RigidBodyType type) { m_type = type; }

            // 获取碰撞标志
            CollisionFlags getCollisionFlags() const { return m_collisionFlags; }

            // 设置碰撞标志
            void setCollisionFlags(CollisionFlags flags) { m_collisionFlags = flags; }

            // 检查是否为静态刚体
            bool isStatic() const { return m_type == RigidBodyType::Static; }

            // 检查是否为动态刚体
            bool isDynamic() const { return m_type == RigidBodyType::Dynamic; }

            // 检查是否为运动学刚体
            bool isKinematic() const { return m_type == RigidBodyType::Kinematic; }

            // 检查是否激活（参与模拟）
            bool isActive() const { return m_isActive; }

            // 设置激活状态
            void setActive(bool active) { m_isActive = active; }

            // ========== 变换属性 ==========

            // 获取位置（世界空间）
            const glm::dvec3& getPosition() const { return m_position; }

            // 设置位置
            void setPosition(const glm::dvec3& position) {
                m_position = position;
                m_isAwake = true;
            }

            // 获取旋转（四元数，世界空间）
            const glm::dquat& getRotation() const { return m_rotation; }

            // 设置旋转
            void setRotation(const glm::dquat& rotation) {
                m_rotation = rotation;
                m_isAwake = true;
            }

            // 获取缩放
            const glm::dvec3& getScale() const { return m_scale; }

            // 设置缩放
            void setScale(const glm::dvec3& scale) { m_scale = scale; }

            // 获取变换矩阵
            glm::dmat4 getTransformMatrix() const {
                glm::dmat4 translation = glm::translate(glm::dmat4(1.0), m_position);
                glm::dmat4 rotation = glm::mat4_cast(m_rotation);
                glm::dmat4 scale = glm::scale(glm::dmat4(1.0), m_scale);
                return translation * rotation * scale;
            }

            // ========== 速度属性 ==========

            // 获取线性速度
            const glm::dvec3& getLinearVelocity() const { return m_linearVelocity; }

            // 设置线性速度
            void setLinearVelocity(const glm::dvec3& velocity) {
                if (!isStatic()) {
                    m_linearVelocity = velocity;
                    m_isAwake = true;
                }
            }

            // 获取角速度（弧度/秒）
            const glm::dvec3& getAngularVelocity() const { return m_angularVelocity; }

            // 设置角速度
            void setAngularVelocity(const glm::dvec3& velocity) {
                if (!isStatic()) {
                    m_angularVelocity = velocity;
                    m_isAwake = true;
                }
            }

            // ========== 质量属性 ==========

            // 获取质量
            double getMass() const { return m_mass; }

            // 设置质量
            void setMass(double mass);

            // 获取质量倒数（用于优化计算）
            double getInverseMass() const { return m_inverseMass; }

            // 获取无限质量标志
            bool hasInfiniteMass() const { return m_inverseMass == 0.0; }

            // ========== 惯性张量 ==========

            // 获取惯性张量
            const glm::dmat3& getInertiaTensor() const { return m_inertiaTensor; }

            // 设置惯性张量
            void setInertiaTensor(const glm::dmat3& tensor);

            // 获取惯性张量倒数（世界空间）
            const glm::dmat3& getInverseInertiaTensorWorld() const {
                return m_inverseInertiaTensorWorld;
            }

            // 计算并更新世界空间惯性张量倒数
            void updateInertiaTensorWorld();

            // ========== 阻尼属性 ==========

            // 获取线性阻尼
            double getLinearDamping() const { return m_linearDamping; }

            // 设置线性阻尼
            void setLinearDamping(double damping) { m_linearDamping = damping; }

            // 获取角阻尼
            double getAngularDamping() const { return m_angularDamping; }

            // 设置角阻尼
            void setAngularDamping(double damping) { m_angularDamping = damping; }

            // ========== 睡眠/唤醒 ==========

            // 检查是否睡眠（静止）
            bool isAwake() const { return m_isAwake; }

            // 设置唤醒状态
            void setAwake(bool awake);

            // 唤醒刚体
            void wakeUp() { setAwake(true); }

            // 让刚体进入睡眠
            void sleep() { setAwake(false); }

            // 获取睡眠时间（秒）
            double getSleepTime() const { return m_sleepTime; }

            // ========== 力和力矩累加 ==========

            /* 应用力（世界空间） */
            void applyForce(const glm::dvec3& force);

            /**
             * @brief 在指定点应用力（世界空间）
             */
            void applyForceAtPoint(const glm::dvec3& force, const glm::dvec3& point);

            /* 应用力矩（世界空间） */
            void applyTorque(const glm::dvec3& torque);

            /* 应用冲量（瞬间改变速度） */
            void applyImpulse(const glm::dvec3& impulse);

            /**
             * @brief 在指定点应用冲量（世界空间）
             */
            void applyImpulseAtPoint(const glm::dvec3& impulse, const glm::dvec3& point);

            // 清除所有累加的力和力矩
            void clearForces();

            // 获取当前累加的力
            const glm::dvec3& getAccumulatedForce() const { return m_accumulatedForce; }

            // 获取当前累加的力矩
            const glm::dvec3& getAccumulatedTorque() const { return m_accumulatedTorque; }

            // ========== 物理更新 ==========

            /* 物理步进 */
            void integrate(double ts);

            // 计算速度阻尼
            void applyDamping(double ts);

            // ========== 用户数据 ==========

            // 设置用户数据指针
            void setUserData(void* data) { m_userData = data; }

            // 获取用户数据指针
            void* getUserData() const { return m_userData; }

            // 获取运动状态引用
            MotionState& getMotionState() { return m_motionState; }
            const MotionState& getMotionState() const { return m_motionState; }

            // 更新运动状态
            void updateMotionState() {
                m_motionState.storeCurrentState();
                m_motionState.setCurrentState(m_position, m_rotation);
            }

            // ========== 碰撞形状属性 ==========

            // 获取碰撞形状 AABB（世界空间）
            AABB getWorldAABB() const {
                glm::dvec3 halfSize = m_collisionHalfSize;
                glm::dvec3 worldMin = m_position - halfSize;
                glm::dvec3 worldMax = m_position + halfSize;
                return AABB(worldMin.x, worldMin.y, worldMin.z,
                           worldMax.x, worldMax.y, worldMax.z);
            }

            // 设置碰撞半尺寸
            void setCollisionHalfSize(const glm::dvec3& halfSize) { m_collisionHalfSize = halfSize; }

            // 获取碰撞半尺寸
            const glm::dvec3& getCollisionHalfSize() const { return m_collisionHalfSize; }

            // ========== CCD (连续碰撞检测) ==========

            // 启用或禁用 CCD
            void setCCDEnabled(bool enabled) { m_ccdEnabled = enabled; }

            // 检查 CCD 是否启用
            bool isCCDEnabled() const { return m_ccdEnabled; }

            // 设置 CCD 速度阈值（超过此速度时触发 CCD）
            void setCcdMotionThreshold(double threshold) { m_ccdMotionThreshold = threshold; }

            // 获取 CCD 速度阈值
            double getCcdMotionThreshold() const { return m_ccdMotionThreshold; }

        private:
            friend class PhysicsSystem;
            friend class ConstraintSolver;
            friend class CCDSolver;

            // ========== 类型和标志 ==========
            RigidBodyType m_type;              // 刚体类型
            CollisionFlags m_collisionFlags;     // 碰撞标志
            bool m_isActive;                  // 是否激活
            bool m_isAwake;                  // 是否唤醒（参与模拟）
            double m_sleepTime;                // 睡眠时间

            // ========== 变换属性 ==========
            glm::dvec3 m_position;           // 位置（世界空间）
            glm::dquat m_rotation;           // 旋转（世界空间四元数）
            glm::dvec3 m_scale;             // 缩放

            // ========== 速度属性 ==========
            glm::dvec3 m_linearVelocity;     // 线性速度
            glm::dvec3 m_angularVelocity;    // 角速度（弧度/秒）

            // ========== 质量属性 ==========
            double m_mass;                    // 质量
            double m_inverseMass;              // 质量倒数（用于优化）
            glm::dmat3 m_inertiaTensor;       // 惯性张量（物体空间）
            glm::dmat3 m_inverseInertiaTensorWorld; // 惯性张量倒数（世界空间）

            // ========== 阻尼属性 ==========
            double m_linearDamping;           // 线性阻尼（0-1）
            double m_angularDamping;          // 角阻尼（0-1）

            // ========== 力和力矩累加 ==========
            glm::dvec3 m_accumulatedForce;   // 累加的力
            glm::dvec3 m_accumulatedTorque; // 累加的力矩

            // ========== 碰撞形状 ==========
            glm::dvec3 m_collisionHalfSize;   // 碰撞半尺寸（用于 AABB 计算）
            MotionState m_motionState;         // 运动状态（渲染插值）

            // ========== CCD ==========
            bool m_ccdEnabled = false;         // 是否启用连续碰撞检测
            double m_ccdMotionThreshold = 5.0; // CCD 速度阈值

            // ========== 用户数据 ==========
            void* m_userData;                 // 用户数据指针
        };

    } // namespace Physics
} // namespace Prisma
