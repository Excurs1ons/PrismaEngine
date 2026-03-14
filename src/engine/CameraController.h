#pragma once
#include "Component.h"
#include "input/InputManager.h"  // KeyCode 定义在此文件中

#include "Camera.h"

namespace Prisma {

class CameraController : public Component {
public:
    CameraController();
    ~CameraController() override = default;

    void Initialize() override;
    void Update(Timestep ts) override;

    void SetMoveSpeed(float speed) { m_moveSpeed = speed; }
    float GetMoveSpeed() const { return m_moveSpeed; }

    void SetRotationSpeed(float speed) { m_rotationSpeed = speed; }
    float GetRotationSpeed() const { return m_rotationSpeed; }

    // 是否启用鼠标控制
    void SetMouseControl(bool enable) { m_mouseControl = enable; }
    bool GetMouseControl() const { return m_mouseControl; }

private:
    void HandleKeyboardInput(Timestep ts);
    void HandleMouseInput(Timestep ts);

    float m_moveSpeed = 5.0f;        // 相机移动速度 (单位/秒)
    float m_rotationSpeed = 90.0f;   // 相机旋转速度 (度/秒)
    bool m_mouseControl = true;      // 是否启用鼠标控制

    std::shared_ptr<Prisma::Graphic::Camera> m_camera = nullptr;
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;
    bool m_firstMouse = true;
};

} // namespace Engine