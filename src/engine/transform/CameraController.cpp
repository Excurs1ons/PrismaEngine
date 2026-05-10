#include "CameraController.h"
#include "Camera.h"
#include "GameObject.h"
#include "Logger.h"
#include "Platform.h"
#include "input/InputManager.h"
#include "math/MathTypes.h"
#include "Engine.h"
#include <cmath>

namespace Prisma {

using namespace Input;
using namespace Graphic;

CameraController::CameraController() : Component() {
    m_mouseControl = false;  // 默认关闭鼠标控制
    m_firstMouse   = true;
}

void CameraController::Initialize() {
    // 获取同一GameObject上的Camera组件
    m_camera = GetOwner()->GetComponent<Camera>();
    if (!m_camera) {
        LOG_WARNING("CameraController", "在 GameObject '{0}' 上未找到 Camera 组件", GetOwner()->name);
    } else {
        LOG_DEBUG("CameraController", "GameObject '{0}' 的 CameraController 已初始化", GetOwner()->name);
    }

    // 获取初始鼠标位置
    auto inputManager = Engine::Get().GetInputManager();
    if (inputManager) {
        auto mousePos = inputManager->GetMousePosition();
        m_lastMouseX  = mousePos.x;
        m_lastMouseY  = mousePos.y;
    }
}

void CameraController::Update(Timestep ts) {
    if (m_camera == nullptr) {
        return;
    }

    HandleKeyboardInput(ts);
    HandleMouseInput(ts);
}

void CameraController::HandleKeyboardInput(Timestep ts) {
    auto inputManager = Engine::Get().GetInputManager();
    if (!inputManager) return;

    float moveAmount = m_moveSpeed * ts;

    // WASD 控制移动
    if (inputManager->IsKeyPressed(KeyCode::W)) {
        m_camera->MoveLocal(moveAmount, 0.0f, 0.0f);  // 前进
    }
    if (inputManager->IsKeyPressed(KeyCode::S)) {
        m_camera->MoveLocal(-moveAmount, 0.0f, 0.0f);  // 后退
    }
    if (inputManager->IsKeyPressed(KeyCode::A)) {
        m_camera->MoveLocal(0.0f, -moveAmount, 0.0f);  // 左移
    }
    if (inputManager->IsKeyPressed(KeyCode::D)) {
        m_camera->MoveLocal(0.0f, moveAmount, 0.0f);  // 右移
    }

    // Q/E 控制上下移动
    if (inputManager->IsKeyPressed(KeyCode::Q)) {
        m_camera->MoveLocal(0.0f, 0.0f, -moveAmount);  // 下降
    }
    if (inputManager->IsKeyPressed(KeyCode::E)) {
        m_camera->MoveLocal(0.0f, 0.0f, moveAmount);  // 上升
    }

    // 方向键控制旋转
    float rotationAmount = m_rotationSpeed * ts;
    if (inputManager->IsKeyPressed(KeyCode::ArrowLeft)) {
        m_camera->Rotate(0.0f, -glm::radians(rotationAmount), 0.0f);  // 左转
    }
    if (inputManager->IsKeyPressed(KeyCode::ArrowRight)) {
        m_camera->Rotate(0.0f, glm::radians(rotationAmount), 0.0f);  // 右转
    }
    if (inputManager->IsKeyPressed(KeyCode::ArrowUp)) {
        m_camera->Rotate(-glm::radians(rotationAmount), 0.0f, 0.0f);  // 上看
    }
    if (inputManager->IsKeyPressed(KeyCode::ArrowDown)) {
        m_camera->Rotate(glm::radians(rotationAmount), 0.0f, 0.0f);  // 下看
    }
}

void CameraController::HandleMouseInput(Timestep ts) {
    if (!m_mouseControl) {
        return;
    }

    auto inputManager = Engine::Get().GetInputManager();
    if (!inputManager) return;

    // 获取当前鼠标位置
    auto mousePos = inputManager->GetMousePosition();
    float mouseX  = mousePos.x;
    float mouseY  = mousePos.y;

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
    const float frameScale = std::max(ts.GetSeconds(), 1.0f / 240.0f) * 60.0f;
    float sensitivity = 0.1f * frameScale;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    if (std::abs(xOffset) > 0.01f || std::abs(yOffset) > 0.01f) {
        m_camera->Rotate(glm::radians(yOffset), glm::radians(xOffset), 0.0f);
    }
}

}  // namespace Prisma
