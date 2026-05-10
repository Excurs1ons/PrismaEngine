#pragma once

#include "core/Component.h"
#include "graphic/ICamera.h"
#include "graphic/OrthographicCamera.h"
#include <memory>

namespace Prisma {

/**
 * @brief 相机组件
 * 将相机功能封装为组件，可挂载到 GameObject 或 ECS 实体
 */
class ENGINE_API CameraComponent : public Component {
public:
    CameraComponent();
    virtual ~CameraComponent() override = default;

    void Initialize() override;
    
    std::shared_ptr<Graphic::ICamera> GetCamera() const { return m_Camera; }

    // 常用设置转发
    void SetProjection(float left, float right, float bottom, float top);
    void SetClearColor(float r, float g, float b, float a);

private:
    std::shared_ptr<Graphic::OrthographicCamera> m_Camera;
};

} // namespace Prisma
