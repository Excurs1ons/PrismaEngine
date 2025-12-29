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
        bool wPressed = Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::W);
        bool aPressed = Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::A);
        bool sPressed = Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::S);
        bool dPressed = Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::D);
        LOG_TRACE("CameraController", "Input test - W:{0} A:{1} S:{2} D:{3}", wPressed, aPressed, sPressed, dPressed);
    }

    // 获取当前相机位置
    PrismaMath::vec3 currentPos = m_camera->GetPosition();

    // WASD 或 方向键控制相机移动
    if (Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::W) || Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::ArrowUp)) {
        // 向上移动 (在2D中通常是减少Y值，因为我们使用右手坐标系)
        currentPos.y -= moveSpeed;
        moved = true;
    }
    if (Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::S) || Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::ArrowDown)) {
        // 向下移动 (增加Y值)
        currentPos.y += moveSpeed;
        moved = true;
    }
    if (Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::A) || Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::ArrowLeft)) {
        // 向左移动 (减少X值)
        currentPos.x -= moveSpeed;
        moved = true;
    }
    if (Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::D) || Input::InputManager::GetInstance().IsKeyDown(Input::KeyCode::ArrowRight)) {
        // 向右移动 (增加X值)
        currentPos.x += moveSpeed;
        moved = true;
    }

    // 如果有移动，设置新位置并打印位置信息（用于调试）
    if (moved) {
        m_camera->SetPosition(currentPos.x, currentPos.y, currentPos.z);
        LOG_INFO("CameraController", "Camera moved to position: ({0}, {1}, {2})", currentPos.x, currentPos.y, currentPos.z);
    }
}

} // namespace Engine