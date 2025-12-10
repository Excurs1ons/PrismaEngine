#pragma once
#include "Component.h"
#include "KeyCode.h"
#include "InputManager.h"

class Camera2D;

namespace Engine {

class CameraController : public Component {
public:
    CameraController();
    ~CameraController() override = default;

    void Initialize() override;
    void Update(float deltaTime) override;

    void SetMoveSpeed(float speed) { m_moveSpeed = speed; }
    float GetMoveSpeed() const { return m_moveSpeed; }

private:
    void HandleInput();

    float m_moveSpeed = 2.0f;  // 相机移动速度 (单位/秒)
    Camera2D* m_camera = nullptr;
};

} // namespace Engine