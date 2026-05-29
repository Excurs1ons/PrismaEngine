#pragma once

namespace Prisma {
    namespace Physics {

        /**
         * @brief 物理材质属性
         *
         * 定义表面的摩擦和弹性特性，通过指针引用自 RigidBody
         */
        struct PhysicsMaterial {
            double restitution = 0.3;        ///< 恢复系数（弹性），0=完全非弹性，1=完全弹性
            double staticFriction = 0.5;     ///< 静摩擦系数
            double dynamicFriction = 0.3;    ///< 动摩擦系数

            PhysicsMaterial() = default;

            PhysicsMaterial(double restitution, double staticFriction, double dynamicFriction) noexcept
                : restitution(restitution)
                , staticFriction(staticFriction)
                , dynamicFriction(dynamicFriction) {}
        };

        /// 常用材质预设
        inline const PhysicsMaterial Material_Default{ 0.3, 0.5, 0.3 };
        inline const PhysicsMaterial Material_Ice{ 0.1, 0.05, 0.03 };
        inline const PhysicsMaterial Material_Metal{ 0.5, 0.7, 0.4 };
        inline const PhysicsMaterial Material_Rubber{ 0.8, 0.9, 0.6 };
        inline const PhysicsMaterial Material_Stone{ 0.1, 0.8, 0.5 };
        inline const PhysicsMaterial Material_Wood{ 0.2, 0.6, 0.4 };

    } // namespace Physics
} // namespace Prisma
