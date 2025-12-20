#include "CameraController.h"
#include "Camera2D.h"
#include "InputManager.h"
#include "Logger.h"
#include "Platform.h"
#include "GameObject.h"
#include "math/MathTypes.h"

namespace Engine {

CameraController::CameraController() : Component() {
}

void CameraController::Initialize() {
    // 获取同一GameObject上的Camera2D组件
    m_camera = m_owner->GetComponent<Camera2D>();
    if (!m_camera) {
        LOG_WARNING("CameraController", "No Camera2D component found on GameObject '{0}'", m_owner->name);
    } else {
        LOG_INFO("CameraController", "CameraController initialized for GameObject '{0}'", m_owner->name);
    }
}

void CameraController::Update(float deltaTime) {
    if (!m_camera) {
        return;
    }

    HandleInput();
}

void CameraController::HandleInput() {
    float moveSpeed = m_moveSpeed * Time::DeltaTime;  // 基于时间的移动
    bool moved = false;

    // 测试输入系统是否工作
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0) {  // 每秒打印一次
        bool wPressed = InputManager::GetInstance().IsKeyDown(KeyCode::W);
        bool aPressed = InputManager::GetInstance().IsKeyDown(KeyCode::A);
        bool sPressed = InputManager::GetInstance().IsKeyDown(KeyCode::S);
        bool dPressed = InputManager::GetInstance().IsKeyDown(KeyCode::D);
        LOG_TRACE("CameraController", "Input test - W:{0} A:{1} S:{2} D:{3}", wPressed, aPressed, sPressed, dPressed);
    }

    // 获取当前相机位置
    XMVECTOR currentPos = m_camera->GetPosition();
    XMFLOAT3 currentPosFloat;
    XMStoreFloat3(&currentPosFloat, currentPos);

    // WASD 或 方向键控制相机移动
    if (InputManager::GetInstance().IsKeyDown(KeyCode::W) || InputManager::GetInstance().IsKeyDown(KeyCode::ArrowUp)) {
        // 向上移动 (在2D中通常是减少Y值，因为我们使用右手坐标系)
        currentPosFloat.y -= moveSpeed;
        moved = true;
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::S) || InputManager::GetInstance().IsKeyDown(KeyCode::ArrowDown)) {
        // 向下移动 (增加Y值)
        currentPosFloat.y += moveSpeed;
        moved = true;
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::A) || InputManager::GetInstance().IsKeyDown(KeyCode::ArrowLeft)) {
        // 向左移动 (减少X值)
        currentPosFloat.x -= moveSpeed;
        moved = true;
    }
    if (InputManager::GetInstance().IsKeyDown(KeyCode::D) || InputManager::GetInstance().IsKeyDown(KeyCode::ArrowRight)) {
        // 向右移动 (增加X值)
        currentPosFloat.x += moveSpeed;
        moved = true;
    }

    // 如果有移动，设置新位置并打印位置信息（用于调试）
    if (moved) {
        XMVECTOR newPos = XMLoadFloat3(&currentPosFloat);
        m_camera->SetPosition(currentPosFloat.x, currentPosFloat.y, currentPosFloat.z);
        LOG_INFO("CameraController", "Camera moved to position: ({0}, {1}, {2})", currentPosFloat.x, currentPosFloat.y, currentPosFloat.z);
    }
}

} // namespace Engine