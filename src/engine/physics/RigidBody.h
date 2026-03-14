#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <cstdint>

namespace Prisma {
    namespace Physics {

        /**
         * @brief 刚体类型枚举
         */
        enum class RigidBodyType {
            Static,     // 静态刚体（不受力影响，不可移动）
            Dynamic,    // 动态刚体（受力和力矩影响，完全模拟）
            Kinematic   // 运动学刚体（可移动但不受力影响，动画控制）
        };

        /**
         * @brief 碰撞标志枚举
         */
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

        /**
         * @brief 刚体类
         *
         * 实现基本的刚体动力学模拟
         */
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

            /**
             * @brief 获取刚体类型
             */
            RigidBodyType getType() const { return m_type; }

            /**
             * @brief 设置刚体类型
             */
            void setType(RigidBodyType type) { m_type = type; }

            /**
             * @brief 获取碰撞标志
             */
            CollisionFlags getCollisionFlags() const { return m_collisionFlags; }

            /**
             * @brief 设置碰撞标志
             */
            void setCollisionFlags(CollisionFlags flags) { m_collisionFlags = flags; }

            /**
             * @brief 检查是否为静态刚体
             */
            bool isStatic() const { return m_type == RigidBodyType::Static; }

            /**
             * @brief 检查是否为动态刚体
             */
            bool isDynamic() const { return m_type == RigidBodyType::Dynamic; }

            /**
             * @brief 检查是否为运动学刚体
             */
            bool isKinematic() const { return m_type == RigidBodyType::Kinematic; }

            /**
             * @brief 检查是否激活（参与模拟）
             */
            bool isActive() const { return m_isActive; }

            /**
             * @brief 设置激活状态
             */
            void setActive(bool active) { m_isActive = active; }

            // ========== 变换属性 ==========

            /**
             * @brief 获取位置（世界空间）
             */
            const glm::dvec3& getPosition() const { return m_position; }

            /**
             * @brief 设置位置
             */
            void setPosition(const glm::dvec3& position) {
                m_position = position;
                m_isAwake = true;
            }

            /**
             * @brief 获取旋转（四元数，世界空间）
             */
            const glm::dquat& getRotation() const { return m_rotation; }

            /**
             * @brief 设置旋转
             */
            void setRotation(const glm::dquat& rotation) {
                m_rotation = rotation;
                m_isAwake = true;
            }

            /**
             * @brief 获取缩放
             */
            const glm::dvec3& getScale() const { return m_scale; }

            /**
             * @brief 设置缩放
             */
            void setScale(const glm::dvec3& scale) { m_scale = scale; }

            /**
             * @brief 获取变换矩阵
             */
            glm::dmat4 getTransformMatrix() const {
                glm::dmat4 translation = glm::translate(glm::dmat4(1.0), m_position);
                glm::dmat4 rotation = glm::mat4_cast(m_rotation);
                glm::dmat4 scale = glm::scale(glm::dmat4(1.0), m_scale);
                return translation * rotation * scale;
            }

            // ========== 速度属性 ==========

            /**
             * @brief 获取线性速度
             */
            const glm::dvec3& getLinearVelocity() const { return m_linearVelocity; }

            /**
             * @brief 设置线性速度
             */
            void setLinearVelocity(const glm::dvec3& velocity) {
                if (!isStatic()) {
                    m_linearVelocity = velocity;
                    m_isAwake = true;
                }
            }

            /**
             * @brief 获取角速度（弧度/秒）
             */
            const glm::dvec3& getAngularVelocity() const { return m_angularVelocity; }

            /**
             * @brief 设置角速度
             */
            void setAngularVelocity(const glm::dvec3& velocity) {
                if (!isStatic()) {
                    m_angularVelocity = velocity;
                    m_isAwake = true;
                }
            }

            // ========== 质量属性 ==========

            /**
             * @brief 获取质量
             */
            double getMass() const { return m_mass; }

            /**
             * @brief 设置质量
             */
            void setMass(double mass);

            /**
             * @brief 获取质量倒数（用于优化计算）
             */
            double getInverseMass() const { return m_inverseMass; }

            /**
             * @brief 获取无限质量标志
             */
            bool hasInfiniteMass() const { return m_inverseMass == 0.0; }

            // ========== 惯性张量 ==========

            /**
             * @brief 获取惯性张量
             */
            const glm::dmat3& getInertiaTensor() const { return m_inertiaTensor; }

            /**
             * @brief 设置惯性张量
             */
            void setInertiaTensor(const glm::dmat3& tensor);

            /**
             * @brief 获取惯性张量倒数（世界空间）
             */
            const glm::dmat3& getInverseInertiaTensorWorld() const {
                return m_inverseInertiaTensorWorld;
            }

            /**
             * @brief 计算并更新世界空间惯性张量倒数
             */
            void updateInertiaTensorWorld();

            // ========== 阻尼属性 ==========

            /**
             * @brief 获取线性阻尼
             */
            double getLinearDamping() const { return m_linearDamping; }

            /**
             * @brief 设置线性阻尼
             */
            void setLinearDamping(double damping) { m_linearDamping = damping; }

            /**
             * @brief 获取角阻尼
             */
            double getAngularDamping() const { return m_angularDamping; }

            /**
             * @brief 设置角阻尼
             */
            void setAngularDamping(double damping) { m_angularDamping = damping; }

            // ========== 睡眠/唤醒 ==========

            /**
             * @brief 检查是否睡眠（静止）
             */
            bool isAwake() const { return m_isAwake; }

            /**
             * @brief 设置唤醒状态
             */
            void setAwake(bool awake);

            /**
             * @brief 唤醒刚体
             */
            void wakeUp() { setAwake(true); }

            /**
             * @brief 让刚体进入睡眠
             */
            void sleep() { setAwake(false); }

            /**
             * @brief 获取睡眠时间（秒）
             */
            double getSleepTime() const { return m_sleepTime; }

            // ========== 力和力矩累加 ==========

            /**
             * @brief 应用力（世界空间）
             * @param force 力向量
             */
            void applyForce(const glm::dvec3& force);

            /**
             * @brief 在指定点应用力（世界空间）
             * @param force 力向量
             * @param point 世界空间点
             */
            void applyForceAtPoint(const glm::dvec3& force, const glm::dvec3& point);

            /**
             * @brief 应用力矩（世界空间）
             * @param torque 力矩向量
             */
            void applyTorque(const glm::dvec3& torque);

            /**
             * @brief 应用冲量（瞬间改变速度）
             * @param impulse 冲量向量
             */
            void applyImpulse(const glm::dvec3& impulse);

            /**
             * @brief 在指定点应用冲量（世界空间）
             * @param impulse 冲量向量
             * @param point 世界空间点
             */
            void applyImpulseAtPoint(const glm::dvec3& impulse, const glm::dvec3& point);

            /**
             * @brief 清除所有累加的力和力矩
             */
            void clearForces();

            /**
             * @brief 获取当前累加的力
             */
            const glm::dvec3& getAccumulatedForce() const { return m_accumulatedForce; }

            /**
             * @brief 获取当前累加的力矩
             */
            const glm::dvec3& getAccumulatedTorque() const { return m_accumulatedTorque; }

            // ========== 物理更新 ==========

            /**
             * @brief 物理步进
             * @param ts 时间步长（秒）
             */
            void integrate(double ts);

            /**
             * @brief 计算速度阻尼
             */
            void applyDamping(double ts);

            // ========== 用户数据 ==========

            /**
             * @brief 设置用户数据指针
             */
            void setUserData(void* data) { m_userData = data; }

            /**
             * @brief 获取用户数据指针
             */
            void* getUserData() const { return m_userData; }

        private:
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

            // ========== 用户数据 ==========
            void* m_userData;                 // 用户数据指针
        };

    } // namespace Physics
} // namespace Prisma
