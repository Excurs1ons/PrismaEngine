#include "InputManager.h"
#include "Logger.h"
#include <unordered_map>

namespace PrismaEngine::Input {

struct InputManager::Impl {
    PrismaMath::vec2 mousePosition = {0, 0};
    PrismaMath::vec2 mouseDelta = {0, 0};
    std::unordered_map<KeyCode, bool> keyStates;
    std::unordered_map<KeyCode, bool> prevKeyStates;
    std::unordered_map<MouseButton, bool> mouseButtonStates;
    std::unordered_map<MouseButton, bool> prevMouseButtonStates;
};

std::shared_ptr<InputManager> InputManager::GetInstance() {
    static std::shared_ptr<InputManager> instance = std::shared_ptr<InputManager>(new InputManager());
    return instance;
}

InputManager::InputManager() : m_impl(std::make_unique<Impl>()) {
}

InputManager::~InputManager() {
}

int InputManager::Initialize() {
    LOG_INFO("Input", "Input manager initialized.");
    return 0;
}

void InputManager::Shutdown() {
}

void InputManager::Update(float deltaTime) {
    (void)deltaTime;
    m_impl->prevKeyStates = m_impl->keyStates;
    m_impl->prevMouseButtonStates = m_impl->mouseButtonStates;
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    auto it = m_impl->keyStates.find(key);
    return it != m_impl->keyStates.end() && it->second;
}

bool InputManager::IsKeyJustPressed(KeyCode key) const {
    auto it = m_impl->keyStates.find(key);
    auto prevIt = m_impl->prevKeyStates.find(key);
    bool current = it != m_impl->keyStates.end() && it->second;
    bool previous = prevIt != m_impl->prevKeyStates.end() && prevIt->second;
    return current && !previous;
}

bool InputManager::IsKeyJustReleased(KeyCode key) const {
    auto it = m_impl->keyStates.find(key);
    auto prevIt = m_impl->prevKeyStates.find(key);
    bool current = it != m_impl->keyStates.end() && it->second;
    bool previous = prevIt != m_impl->prevKeyStates.end() && prevIt->second;
    return !current && previous;
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const {
    auto it = m_impl->mouseButtonStates.find(button);
    return it != m_impl->mouseButtonStates.end() && it->second;
}

bool InputManager::IsMouseButtonJustPressed(MouseButton button) const {
    auto it = m_impl->mouseButtonStates.find(button);
    auto prevIt = m_impl->prevMouseButtonStates.find(button);
    bool current = it != m_impl->mouseButtonStates.end() && it->second;
    bool previous = prevIt != m_impl->prevMouseButtonStates.end() && prevIt->second;
    return current && !previous;
}

bool InputManager::IsMouseButtonJustReleased(MouseButton button) const {
    auto it = m_impl->mouseButtonStates.find(button);
    auto prevIt = m_impl->prevMouseButtonStates.find(button);
    bool current = it != m_impl->mouseButtonStates.end() && it->second;
    bool previous = prevIt != m_impl->prevMouseButtonStates.end() && prevIt->second;
    return !current && previous;
}

PrismaMath::vec2 InputManager::GetMousePosition() const {
    return m_impl->mousePosition;
}

PrismaMath::vec2 InputManager::GetMouseDelta() const {
    return m_impl->mouseDelta;
}

void InputManager::SetKeyState(KeyCode key, bool pressed) {
    m_impl->keyStates[key] = pressed;
}

void InputManager::SetMouseButtonState(MouseButton button, bool pressed) {
    m_impl->mouseButtonStates[button] = pressed;
}

void InputManager::SetMousePosition(const PrismaMath::vec2& pos) {
    m_impl->mouseDelta = { pos.x - m_impl->mousePosition.x, pos.y - m_impl->mousePosition.y };
    m_impl->mousePosition = pos;
}

} // namespace PrismaEngine::Input
