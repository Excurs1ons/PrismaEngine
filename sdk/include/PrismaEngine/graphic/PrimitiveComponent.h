#pragma once

#include "Export.h"
#include "core/Component.h"
#include "math/MathTypes.h"
#include <string>
#include <array>

namespace Prisma::Graphic {

/// @brief 原生图元类型（用于路径追踪等无需三角化的场景）
enum class PrimitiveShape : uint8_t {
    Sphere = 1,
    Box    = 2,
    Cone   = 3,
    Plane  = 0
};

/// @brief 原生图元渲染组件
/// 替代三角网格 MeshRenderer，直接提供解析几何体（球体/盒子/锥体/平面）。
/// 目前仅路径追踪管线使用，Forward 渲染器仍需要三角化 MeshRenderer。
class ENGINE_API PrimitiveComponent : public Component {
public:
    struct Data {
        std::string shape = "Sphere";       // "Sphere"/"Box"/"Cone"/"Plane"
        std::string material;               // .mat 材质文件路径
        std::array<float, 3> emissive = {0.0f, 0.0f, 0.0f};
    };

    PrimitiveComponent() = default;
    ~PrimitiveComponent() override;

    PrimitiveShape GetShape() const { return m_shape; }
    void SetShape(PrimitiveShape s) { m_shape = s; }

    const std::string& GetMaterialPath() const { return m_materialPath; }
    PrismaMath::vec3 GetEmissive() const { return m_emissive; }
    void SetEmissive(const PrismaMath::vec3& e) { m_emissive = e; }

    // 序列化
    const char* GetComponentTypeName() const override { return "PrimitiveComponent"; }
    Data GetData() const;
    void SetData(const Data& d);

    // 工具：字符串 ↔ PrimitiveShape
    static PrimitiveShape ShapeFromString(const std::string& s);
    static const char* ShapeToString(PrimitiveShape s);

private:
    PrimitiveShape m_shape = PrimitiveShape::Sphere;
    std::string m_materialPath;
    PrismaMath::vec3 m_emissive = {0.0f, 0.0f, 0.0f};
};

} // namespace Prisma::Graphic
