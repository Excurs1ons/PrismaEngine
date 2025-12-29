#include "InputManager.h"
#include "Logger.h"

namespace Engine::Input {

bool InputManager::IsKeyDown(KeyCode key) const {
    if (!m_platform) {
        LOG_WARNING("InputManager", "Platform not set, cannot check key state");
        return false;
    }
    return m_platform->IsKeyDown(key);
}

bool InputManager::IsMouseButtonDown(MouseButton button) const {
    if (!m_platform) {
        LOG_WARNING("InputManager", "Platform not set, cannot check mouse button state");
        return false;
    }
    return m_platform->IsMouseButtonDown(button);
}

void InputManager::GetMousePosition(float& x, float& y) const {
    if (!m_platform) {
        LOG_WARNING("InputManager", "Platform not set, cannot get mouse position");
        x = 0.0f;
        y = 0.0f;
        return;
    }
    m_platform->GetMousePosition(x, y);
}

void InputManager::SetPlatform(Platform* platform) {
    m_platform = platform;
    LOG_INFO("InputManager", "Platform instance set");
}

} // namespace Engine