#include "InputManager.h"
#include "Logger.h"
#include <algorithm>
#include <fstream>

namespace Engine {
namespace Input {

InputManager& InputManager::GetInstance()
{
    static InputManager instance;
    return instance;
}

void InputManager::Initialize()
{
    LOG_INFO("InputManager", "输入管理器初始化");

    // 初始化手柄
    // TODO: 初始化SDL/XInput手柄支持

    // 加载输入映射
    LoadInputMappings("input.json");

    // 设置默认输入模式
    SetInputMode(InputMode::Game);
}

void InputManager::Update()
{
    // 清除单帧状态
    m_keyPressedThisFrame.clear();
    m_keyReleasedThisFrame.clear();
    m_mouseDelta = DirectX::XMFLOAT2(0, 0);
    m_scrollDelta = DirectX::XMFLOAT2(0, 0);

    // 更新手柄状态
    UpdateGamepadStates();

    // 更新键盘状态
    for (auto& pair : m_keyStates) {
        m_keyStatesPrev[pair.first] = pair.second;
    }

    // 更新鼠标状态
    for (auto& pair : m_mouseButtonStates) {
        m_mouseButtonStatesPrev[pair.first] = pair.second;
    }

    m_mousePositionPrev = m_mousePosition;

    // 处理输入绑定
    ProcessBindings();

    // 清空字符输入
    ClearInputCharacters();
}

void InputManager::OnKeyPressed(KeyCode key, uint32_t modifiers)
{
    if (!m_keyStates[key]) {
        m_keyPressedThisFrame[key] = true;
    }

    m_keyStates[key] = true;

    // 发送事件
    InputEvent event;
    event.type = InputEvent::Type::Key;
    event.key.key = key;
    event.key.action = InputAction::Pressed;
    event.key.modifiers = modifiers;
    SendEvent(event);
}

void InputManager::OnKeyReleased(KeyCode key, uint32_t modifiers)
{
    m_keyReleasedThisFrame[key] = true;
    m_keyStates[key] = false;

    // 发送事件
    InputEvent event;
    event.type = InputEvent::Type::Key;
    event.key.key = key;
    event.key.action = InputAction::Released;
    event.key.modifiers = modifiers;
    SendEvent(event);
}

void InputManager::OnMouseButtonPressed(MouseButton button, uint32_t modifiers)
{
    m_mouseButtonStates[button] = true;

    // 发送事件
    InputEvent event;
    event.type = InputEvent::Type::Mouse;
    event.mouse.button = button;
    event.mouse.action = InputAction::Pressed;
    event.mouse.modifiers = modifiers;
    SendEvent(event);
}

void InputManager::OnMouseButtonReleased(MouseButton button, uint32_t modifiers)
{
    m_mouseButtonStates[button] = false;

    // 发送事件
    InputEvent event;
    event.type = InputEvent::Type::Mouse;
    event.mouse.button = button;
    event.mouse.action = InputAction::Released;
    event.mouse.modifiers = modifiers;
    SendEvent(event);
}

void InputManager::OnMouseMove(float x, float y)
{
    m_mouseDelta.x += x - m_mousePosition.x;
    m_mouseDelta.y += y - m_mousePosition.y;
    m_mousePosition.x = x;
    m_mousePosition.y = y;

    // 发送事件
    InputEvent event;
    event.type = InputEvent::Type::MouseMove;
    event.mouseMove.x = x;
    event.mouseMove.y = y;
    event.mouseMove.deltaX = m_mouseDelta.x;
    event.mouseMove.deltaY = m_mouseDelta.y;
    SendEvent(event);
}

void InputManager::OnMouseScroll(float deltaX, float deltaY)
{
    m_scrollDelta.x += deltaX;
    m_scrollDelta.y += deltaY;

    // 发送事件
    InputEvent event;
    event.type = InputEvent::Type::Scroll;
    event.scroll.deltaX = deltaX;
    event.scroll.deltaY = deltaY;
    SendEvent(event);
}

void InputManager::OnTouchEvent(int fingerId, float x, float y, InputAction action)
{
    // 发送事件
    InputEvent event;
    event.type = InputEvent::Type::Touch;
    event.touch.fingerId = fingerId;
    event.touch.x = x;
    event.touch.y = y;
    event.touch.action = action;
    SendEvent(event);
}

bool InputManager::IsKeyPressed(KeyCode key) const
{
    auto it = m_keyStates.find(key);
    return it != m_keyStates.end() && it->second;
}

bool InputManager::IsKeyJustPressed(KeyCode key) const
{
    auto it = m_keyPressedThisFrame.find(key);
    return it != m_keyPressedThisFrame.end() && it->second;
}

bool InputManager::IsKeyJustReleased(KeyCode key) const
{
    auto it = m_keyReleasedThisFrame.find(key);
    return it != m_keyReleasedThisFrame.end() && it->second;
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const
{
    auto it = m_mouseButtonStates.find(button);
    return it != m_mouseButtonStates.end() && it->second;
}

bool InputManager::IsMouseButtonJustPressed(MouseButton button) const
{
    auto it = m_mouseButtonStatesPrev.find(button);
    if (it == m_mouseButtonStatesPrev.end() || !it->second) {
        auto currentIt = m_mouseButtonStates.find(button);
        return currentIt != m_mouseButtonStates.end() && currentIt->second;
    }
    return false;
}

bool InputManager::IsMouseButtonJustReleased(MouseButton button) const
{
    auto it = m_mouseButtonStatesPrev.find(button);
    if (it != m_mouseButtonStatesPrev.end() && it->second) {
        auto currentIt = m_mouseButtonStates.find(button);
        return currentIt == m_mouseButtonStates.end() || !currentIt->second;
    }
    return false;
}

const GamepadState& InputManager::GetGamepadState(int gamepadId) const
{
    static GamepadState emptyState;
    auto it = m_gamepadStates.find(gamepadId);
    return (it != m_gamepadStates.end()) ? it->second : emptyState;
}

bool InputManager::IsGamepadConnected(int gamepadId) const
{
    auto it = m_gamepadStates.find(gamepadId);
    return it != m_gamepadStates.end() && it->second.connected;
}

std::shared_ptr<InputBinding> InputManager::CreateBinding(const std::string& name,
                                                         KeyCode key,
                                                         InputAction action,
                                                         InputBinding::Callback callback)
{
    auto binding = std::make_shared<InputBinding>(name, key, action, callback);
    m_bindings[name] = binding;
    return binding;
}

void InputManager::RemoveBinding(const std::string& name)
{
    m_bindings.erase(name);
}

std::shared_ptr<InputBinding> InputManager::GetBinding(const std::string& name)
{
    auto it = m_bindings.find(name);
    return (it != m_bindings.end()) ? it->second : nullptr;
}

void InputManager::LoadInputMappings(const std::string& filePath)
{
    // TODO: 实现JSON加载
    // 这里只是示例
    LOG_INFO("InputManager", "加载输入映射: {0}", filePath);
}

void InputManager::SaveInputMappings(const std::string& filePath)
{
    // TODO: 实现JSON保存
    LOG_INFO("InputManager", "保存输入映射: {0}", filePath);
}

void InputManager::SetInputMapping(const std::string& actionName, const InputMapping& mapping)
{
    m_inputMappings[actionName] = mapping;
}

const InputManager::InputMapping& InputManager::GetInputMapping(const std::string& actionName) const
{
    static InputMapping emptyMapping;
    auto it = m_inputMappings.find(actionName);
    return (it != m_inputMappings.end()) ? it->second : emptyMapping;
}

void InputManager::RegisterEventCallback(EventCallback callback)
{
    m_eventCallbacks.push_back(callback);
}

void InputManager::SetInputMode(InputMode mode)
{
    if (m_inputMode != mode) {
        m_inputMode = mode;
        LOG_INFO("InputManager", "输入模式切换为: {0}", static_cast<int>(mode));

        // 根据模式设置鼠标状态
        if (mode == InputMode::UI) {
            SetCursorVisible(true);
            SetCursorLocked(false);
        } else if (mode == InputMode::Game) {
            // 根据游戏需求设置
        }
    }
}

void InputManager::SetCursorVisible(bool visible)
{
    if (m_cursorVisible != visible) {
        m_cursorVisible = visible;
        // TODO: 实际设置光标可见性
        LOG_DEBUG("InputManager", "光标可见性: {0}", visible);
    }
}

void InputManager::SetCursorLocked(bool locked)
{
    if (m_cursorLocked != locked) {
        m_cursorLocked = locked;
        // TODO: 实际锁定光标
        LOG_DEBUG("InputManager", "光标锁定: {0}", locked);
    }
}

void InputManager::ClearInputCharacters()
{
    m_inputCharacters.clear();
}

void InputManager::UpdateGamepadStates()
{
    // 保存上一帧状态
    m_gamepadStatesPrev = m_gamepadStates;

    // TODO: 实际更新手柄状态
    // 这里应该调用SDL或XInput获取手柄数据
}

void InputManager::ProcessBindings()
{
    for (auto& pair : m_bindings) {
        auto& binding = pair.second;
        if (binding) {
            binding->Update();
        }
    }
}

void InputManager::SendEvent(const InputEvent& event)
{
    for (auto& callback : m_eventCallbacks) {
        if (callback) {
            callback(event);
        }
    }
}

// InputBinding 实现
InputBinding::InputBinding(const std::string& name, KeyCode key, InputAction action, Callback callback)
    : m_name(name),
      m_key(key),
      m_action(action),
      m_callback(callback)
{
}

void InputBinding::Update()
{
    if (!m_enabled || !m_callback) {
        return;
    }

    auto& input = GetInputManager();
    bool triggered = false;

    switch (m_action) {
    case InputAction::Pressed:
        triggered = input.IsKeyJustPressed(m_key);
        break;
    case InputAction::Released:
        triggered = input.IsKeyJustReleased(m_key);
        break;
    case InputAction::Held:
        triggered = input.IsKeyPressed(m_key);
        break;
    case InputAction::DoubleClick:
        // TODO: 实现双击检测
        triggered = input.IsKeyJustPressed(m_key);
        break;
    }

    if (triggered && !m_wasPressed) {
        m_callback();
        m_wasPressed = true;
    } else if (!triggered) {
        m_wasPressed = false;
    }
}

bool InputBinding::IsTriggered() const
{
    auto& input = GetInputManager();

    switch (m_action) {
    case InputAction::Pressed:
        return input.IsKeyJustPressed(m_key);
    case InputAction::Released:
        return input.IsKeyJustReleased(m_key);
    case InputAction::Held:
        return input.IsKeyPressed(m_key);
    case InputAction::DoubleClick:
        return input.IsKeyJustPressed(m_key); // TODO: 修复双击检测
    default:
        return false;
    }
}

} // namespace Input
} // namespace Engine