#pragma once

#include "Export.h"
#include "math/MathTypes.h"
#include <cstdint>

namespace Prisma {
namespace Core {

/**
 * @brief SpriteRenderer 组件 (SoA 版本)
 *
 * 轻量级 2D 精灵渲染组件，数据直接映射到 EntityManager 的 SoA 缓冲区。
 * 不继承旧的 Component 基类（GameObject 架构），而是直接读写 SoA。
 */
struct ENGINE_API SpriteRendererComponent {
    /// RGBA 颜色 (默认白色)
    Color color = Color{1.0f, 1.0f, 1.0f, 1.0f};
    /// 宽度/高度 (默认 100x100)
    Vector2 size = Vector2{100.0f, 100.0f};

    /// 将组件数据写入 EntityManager 的 SoA 缓冲区
    void WriteToSoA(uint32_t entityIndex) const;
};

} // namespace Core
} // namespace Prisma
