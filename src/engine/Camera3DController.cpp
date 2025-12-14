#include "Camera3DController.h"
#include "Camera3D.h"
#include "InputManager.h"
#include "Logger.h"
#include "Platform.h"
#include "GameObject.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;
using namespace Engine::Graphic;

namespace Engine {

Camera3DController::Camera3DController() : Component() {
    m_mouseControl = false;  // 默认关闭鼠标控制
    m_firstMouse = true;
}

void Camera3DController::Initialize() {
    // 获取同一GameObject上的Camera3D组件
    m_camera = m_owner->GetComponent<Camera3D>();
    if (!m_camera) {
        LOG_WARNING("Camera3DController", "No Camera3D component found on GameObject '{0}'", m_owner->name);
    } else {
        LOG_INFO("Camera3DController", "Camera3DController initialized for GameObject '{0}'", m_owner->name);
    }

    // 获取初始鼠标位置
    InputManager::GetInstance().GetMousePosition(m_lastMouseX, m_lastMouseY);
}

void Camera3DController::Update(float deltaTime) {
    if (m_camera == nullptr) {
        LOG_WARNING("Camera3DController", "Camera3D not found on GameObject");
        return;
    }

    HandleKeyboardInput(deltaTime);
    HandleMouseInput(deltaTime);
}

void Camera3DController::HandleKeyboardInput(float deltaTime) {
    float moveAmount = m_moveSpeed * deltaTime;
    bool moved = false;

    // WASD 控制移动
    if (InputManager::GetInstance().IsKeyDown(KeyCode::W)) {
        LOG_INFO("Camera3DController", "W key pressed - moving forward");
        m_camera->MoveLocal(moveAmount, 0.0f, 0.0f);  // 前进
        moved = true;
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::S)) {
        m_camera->MoveLocal(-moveAmount, 0.0f, 0.0f);  // 后退
        moved = true;
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::A)) {
        m_camera->MoveLocal(0.0f, -moveAmount, 0.0f);  // 左移
        moved = true;
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::D)) {
        m_camera->MoveLocal(0.0f, moveAmount, 0.0f);   // 右移
        moved = true;
    }

    // Q/E 控制上下移动
    if (InputManager::GetInstance().IsKeyDown(KeyCode::Q)) {
        m_camera->MoveLocal(0.0f, 0.0f, -moveAmount);  // 下降
        moved = true;
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::E)) {
        m_camera->MoveLocal(0.0f, 0.0f, moveAmount);   // 上升
        moved = true;
    }

    // 方向键控制旋转
    float rotationAmount = m_rotationSpeed * deltaTime;
    if (InputManager::GetInstance().IsKeyDown(KeyCode::ArrowLeft)) {
        m_camera->Rotate(0.0f, -XMConvertToRadians(rotationAmount), 0.0f);  // 左转
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::ArrowRight)) {
        m_camera->Rotate(0.0f, XMConvertToRadians(rotationAmount), 0.0f);   // 右转
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::ArrowUp)) {
        m_camera->Rotate(-XMConvertToRadians(rotationAmount), 0.0f, 0.0f);  // 上看
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::ArrowDown)) {
        m_camera->Rotate(XMConvertToRadians(rotationAmount), 0.0f, 0.0f);   // 下看
    }
}

void Camera3DController::HandleMouseInput(float deltaTime) {
    if (!m_mouseControl) {
        return;
    }

    // 获取当前鼠标位置
    float mouseX, mouseY;
    InputManager::GetInstance().GetMousePosition(mouseX, mouseY);

    if (m_firstMouse) {
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        m_firstMouse = false;
        return;
    }

    // 计算鼠标偏移
    float xOffset = mouseX - m_lastMouseX;
    float yOffset = m_lastMouseY - mouseY;  // 反转Y轴

    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;

    // 应用旋转
    float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    if (std::abs(xOffset) > 0.01f || std::abs(yOffset) > 0.01f) {
        m_camera->Rotate(XMConvertToRadians(yOffset), XMConvertToRadians(xOffset), 0.0f);
    }
}

} // namespace Engine