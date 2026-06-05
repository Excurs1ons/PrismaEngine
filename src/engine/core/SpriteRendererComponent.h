#pragma once

#include "Export.h"
#include "Component.h"
#include "math/MathTypes.h"
#include <cstdint>
#include <array>

namespace Prisma {
namespace Core {

/**
 * @brief SpriteRenderer 组件 (SoA 版本)
 *
 * 轻量级 2D 精灵渲染组件，数据直接映射到 EntityManager 的 SoA 缓冲区。
 */
class ENGINE_API SpriteRendererComponent : public Component {
public:
    SpriteRendererComponent() = default;
    virtual ~SpriteRendererComponent() = default;

    void Initialize() override;
    void Update(Timestep ts) override;
    void Shutdown() override;

    ComponentId GetComponentId() const override { return GetComponentTypeId<SpriteRendererComponent>(); }
    const char* GetComponentTypeName() const override { return "SpriteRendererComponent"; }

    /// RGBA 颜色 (默认白色)
    Color color = Color{1.0f, 1.0f, 1.0f, 1.0f};
    /// 宽度/高度 (默认 100x100)
    Vector2 size = Vector2{100.0f, 100.0f};

    // 序列化数据结构
    struct Data {
        std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
        std::array<float, 2> size = {100.0f, 100.0f};
    };

    Data GetData() const;
    void SetData(const Data& d);

    /// 将组件数据写入 EntityManager 的 SoA 缓冲区
    void WriteToSoA(uint32_t entityIndex) const;
};

} // namespace Core
} // namespace Prisma
