#include "CameraController.h"
#include "Camera.h"
#include "InputManager.h"
#include "Logger.h"
#include "Platform.h"
#include "GameObject.h"
#include "math/MathTypes.h"
#include <cmath>
using namespace PrismaEngine::Graphic;

namespace PrismaEngine {
using namespace Input;
CameraController::CameraController() : Component() {
    m_mouseControl = false;  // 默认关闭鼠标控制
    m_firstMouse = true;
}

void CameraController::Initialize() {
    // 获取同一GameObject上的Camera组件
    m_camera = m_owner->GetComponent<Camera>();
    if (!m_camera) {
        LOG_WARNING("CameraController", "No Camera component found on GameObject '{0}'", m_owner->name);
    } else {
        LOG_INFO("CameraController", "CameraController initialized for GameObject '{0}'", m_owner->name);
    }

    // 获取初始鼠标位置
    Input::InputManager::GetInstance()->GetMousePosition(m_lastMouseX, m_lastMouseY);
}

void CameraController::Update(float deltaTime) {
    if (m_camera == nullptr) {
        LOG_WARNING("CameraController", "Camera not found on GameObject");
        return;
    }

    HandleKeyboardInput(deltaTime);
    HandleMouseInput(deltaTime);
}

void CameraController::HandleKeyboardInput(float deltaTime) {
    float moveAmount = m_moveSpeed * deltaTime;
    bool moved = false;

    // WASD 控制移动
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::W)) {
        LOG_INFO("CameraController", "W key pressed - moving forward");
        m_camera->MoveLocal(moveAmount, 0.0f, 0.0f);  // 前进
        moved = true;
    }
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::S)) {
        m_camera->MoveLocal(-moveAmount, 0.0f, 0.0f);  // 后退
        moved = true;
    }
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::A)) {
        m_camera->MoveLocal(0.0f, -moveAmount, 0.0f);  // 左移
        moved = true;
    }
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::D)) {
        m_camera->MoveLocal(0.0f, moveAmount, 0.0f);   // 右移
        moved = true;
    }

    // Q/E 控制上下移动
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::Q)) {
        m_camera->MoveLocal(0.0f, 0.0f, -moveAmount);  // 下降
        moved = true;
    }
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::E)) {
        m_camera->MoveLocal(0.0f, 0.0f, moveAmount);   // 上升
        moved = true;
    }

    // 方向键控制旋转
    float rotationAmount = m_rotationSpeed * deltaTime;
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::ArrowLeft)) {
        m_camera->Rotate(0.0f, -PrismaEngine::Math::Radians(rotationAmount), 0.0f);  // 左转
    }
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::ArrowRight)) {
        m_camera->Rotate(0.0f, PrismaEngine::Math::Radians(rotationAmount), 0.0f);   // 右转
    }
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::ArrowUp)) {
        m_camera->Rotate(-PrismaEngine::Math::Radians(rotationAmount), 0.0f, 0.0f);  // 上看
    }
    if (InputManager::GetInstance()->IsKeyDown(KeyCode::ArrowDown)) {
        m_camera->Rotate(PrismaEngine::Math::Radians(rotationAmount), 0.0f, 0.0f);   // 下看
    }
}

void CameraController::HandleMouseInput(float deltaTime) {
    if (!m_mouseControl) {
        return;
    }

    // 获取当前鼠标位置
    float mouseX, mouseY;
    InputManager::GetInstance()->GetMousePosition(mouseX, mouseY);

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
        m_camera->Rotate(PrismaEngine::Math::Radians(yOffset), PrismaEngine::Math::Radians(xOffset), 0.0f);
    }
}

} // namespace Engine