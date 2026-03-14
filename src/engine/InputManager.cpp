#include "InputManager.h"
#include "Logger.h"

namespace Prisma::Input {

InputManager::InputManager() : m_platform(nullptr) {
    ClearState();
}

InputManager::~InputManager() {
    Shutdown();
}

bool InputManager::IsKeyDown(KeyCode key) const {
    uint32_t k = static_cast<uint32_t>(key);
    return (k < 512) ? m_KeyState[k] : false;
}

bool InputManager::IsMouseButtonDown(MouseButton button) const {
    uint32_t b = static_cast<uint32_t>(button);
    return (b < 8) ? m_MouseState[b] : false;
}

void InputManager::GetMousePosition(float& x, float& y) const {
    x = m_MouseX;
    y = m_MouseY;
}

void InputManager::ClearState() {
    std::memset(m_KeyState, 0, sizeof(m_KeyState));
    std::memset(m_MouseState, 0, sizeof(m_MouseState));
}

void InputManager::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);

    // 键盘
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) {
        uint32_t k = static_cast<uint32_t>(event.GetKeyCode());
        if (k < 512) m_KeyState[k] = true;
        return false;
    });

    dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& event) {
        uint32_t k = static_cast<uint32_t>(event.GetKeyCode());
        if (k < 512) m_KeyState[k] = false;
        return false;
    });

    // 鼠标
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) {
        uint32_t b = static_cast<uint32_t>(event.GetMouseButton());
        if (b < 8) m_MouseState[b] = true;
        return false;
    });

    dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& event) {
        uint32_t b = static_cast<uint32_t>(event.GetMouseButton());
        if (b < 8) m_MouseState[b] = false;
        return false;
    });

    dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& event) {
        m_MouseX = event.GetX();
        m_MouseY = event.GetY();
        return false;
    });
}

void InputManager::SetPlatform(Platform* platform) {
    m_platform = platform;
}

bool InputManager::Initialize() {
    return true;
}

void InputManager::Shutdown() {
    ClearState();
}

} // namespace Prisma::Input