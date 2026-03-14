#include "InputManager.h"
#include "core/Event.h"
#include <unordered_set>

namespace Prisma {
namespace Input {

struct InputManager::Impl {
    std::unordered_set<KeyCode> pressedKeys;
    std::unordered_set<KeyCode> justPressedKeys;
    std::unordered_set<KeyCode> justReleasedKeys;

    bool mouseButtons[(size_t)MouseButton::Count] = {false};
    bool justPressedMouseButtons[(size_t)MouseButton::Count] = {false};
    bool justReleasedMouseButtons[(size_t)MouseButton::Count] = {false};

    PrismaMath::vec2 mousePosition{0.0f, 0.0f};
    PrismaMath::vec2 lastMousePosition{0.0f, 0.0f};
    PrismaMath::vec2 mouseDelta{0.0f, 0.0f};
};

InputManager::InputManager() : m_impl(std::make_unique<Impl>()) {}

InputManager::~InputManager() {
    Shutdown();
}

int InputManager::Initialize() {
    ClearState();
    return 0;
}

void InputManager::Shutdown() {
    ClearState();
}

void InputManager::Update(Timestep ts) {
    // 每一帧清空 "Just" 状态
    m_impl->justPressedKeys.clear();
    m_impl->justReleasedKeys.clear();
    for (int i = 0; i < (int)MouseButton::Count; ++i) {
        m_impl->justPressedMouseButtons[i] = false;
        m_impl->justReleasedMouseButtons[i] = false;
    }
    m_impl->mouseDelta = m_impl->mousePosition - m_impl->lastMousePosition;
    m_impl->lastMousePosition = m_impl->mousePosition;
}

void InputManager::OnEvent(Event& e) {
    // 这里的逻辑可以根据你的 EventSystem 实现来写。
    // 关键是：这是实例方法，不是静态方法！
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    return m_impl->pressedKeys.find(key) != m_impl->pressedKeys.end();
}

bool InputManager::IsKeyJustPressed(KeyCode key) const {
    return m_impl->justPressedKeys.find(key) != m_impl->justPressedKeys.end();
}

bool InputManager::IsKeyJustReleased(KeyCode key) const {
    return m_impl->justReleasedKeys.find(key) != m_impl->justReleasedKeys.end();
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const {
    return m_impl->mouseButtons[(size_t)button];
}

bool InputManager::IsMouseButtonJustPressed(MouseButton button) const {
    return m_impl->justPressedMouseButtons[(size_t)button];
}

bool InputManager::IsMouseButtonJustReleased(MouseButton button) const {
    return m_impl->justReleasedMouseButtons[(size_t)button];
}

PrismaMath::vec2 InputManager::GetMousePosition() const {
    return m_impl->mousePosition;
}

PrismaMath::vec2 InputManager::GetMouseDelta() const {
    return m_impl->mouseDelta;
}

void InputManager::SetKeyState(KeyCode key, bool pressed) {
    if (pressed) {
        if (m_impl->pressedKeys.find(key) == m_impl->pressedKeys.end()) {
            m_impl->pressedKeys.insert(key);
            m_impl->justPressedKeys.insert(key);
        }
    } else {
        if (m_impl->pressedKeys.find(key) != m_impl->pressedKeys.end()) {
            m_impl->pressedKeys.erase(key);
            m_impl->justReleasedKeys.insert(key);
        }
    }
}

void InputManager::SetMouseButtonState(MouseButton button, bool pressed) {
    if (pressed) {
        if (!m_impl->mouseButtons[(size_t)button]) {
            m_impl->mouseButtons[(size_t)button] = true;
            m_impl->justPressedMouseButtons[(size_t)button] = true;
        }
    } else {
        if (m_impl->mouseButtons[(size_t)button]) {
            m_impl->mouseButtons[(size_t)button] = false;
            m_impl->justReleasedMouseButtons[(size_t)button] = true;
        }
    }
}

void InputManager::SetMousePosition(const PrismaMath::vec2& pos) {
    m_impl->mousePosition = pos;
}

void InputManager::ClearState() {
    m_impl->pressedKeys.clear();
    m_impl->justPressedKeys.clear();
    m_impl->justReleasedKeys.clear();
    for (int i = 0; i < (int)MouseButton::Count; ++i) {
        m_impl->mouseButtons[i] = false;
        m_impl->justPressedMouseButtons[i] = false;
        m_impl->justReleasedMouseButtons[i] = false;
    }
}

} // namespace Input
} // namespace Prisma
