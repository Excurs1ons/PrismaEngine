#pragma once

#include "Export.h"
#include "core/Component.h"
#include "interfaces/RenderTypes.h"
#include "math/MathTypes.h"
#include <array>

namespace Prisma::Graphic {

/**
 * @brief 3D 光源组件
 */
class ENGINE_API LightComponent : public Component {
public:
    enum class LightType : uint32_t {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    struct Data {
        LightType type = LightType::Directional;
        std::array<float, 3> color = { 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;
        float range = 10.0f;
        float spotAngle = 45.0f;
    };

    LightComponent();
    ~LightComponent() override = default;

    const char* GetComponentTypeName() const override { return "Light"; }

    void SetData(const Data& data) { m_Data = data; }
    const Data& GetData() const { return m_Data; }

    // 获取用于渲染的 Light 结构
    Light GetLightData() const;

private:
    Data m_Data;
};

} // namespace Prisma::Graphic
