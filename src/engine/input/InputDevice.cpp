#include "InputDevice.h"

// 平台驱动
#if defined(_WIN32) && (defined(PRISMA_ENABLE_INPUT_RAWINPUT) || defined(PRISMA_ENABLE_INPUT_XINPUT))
    #include "drivers/InputDriverWin32.h"
#elif defined(__ANDROID__) && defined(PRISMA_ENABLE_INPUT_GAMEACTIVITY)
    #include "drivers/InputDriverGameActivity.h"
#elif defined(PRISMA_ENABLE_INPUT_SDL3)
    #include "drivers/InputDriverSDL3.h"
#endif

#include <algorithm>

namespace PrismaEngine::Input {

// ========== InputDevice ==========

InputDevice::InputDevice() = default;

InputDevice::~InputDevice() {
    Shutdown();
}

bool InputDevice::Initialize(InputDriverType driverType) {
    if (m_initialized.load()) {
        return true;
    }

    m_driver = CreateDriver(driverType);
    if (!m_driver) {
        return false;
    }

    if (!m_driver->Initialize()) {
        m_driver.reset();
        return false;
    }

    m_initialized.store(true);
    return true;
}

void InputDevice::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    if (m_driver) {
        m_driver->Shutdown();
        m_driver.reset();
    }

    m_actionMappings.clear();
    m_initialized.store(false);
}

void InputDevice::Update() {
    if (!m_initialized.load()) {
        return;
    }

    m_driver->Update();
}

std::unique_ptr<IInputDriver> InputDevice::CreateDriver(InputDriverType type) {
    #if defined(_WIN32) && (defined(PRISMA_ENABLE_INPUT_RAWINPUT) || defined(PRISMA_ENABLE_INPUT_XINPUT))
        if (type == InputDriverType::Auto || type == InputDriverType::Win32) {
            return std::unique_ptr<IInputDriver>(CreateWin32InputDriver());
        }
    #endif

    #if defined(__ANDROID__) && defined(PRISMA_ENABLE_INPUT_GAMEACTIVITY)
        if (type == InputDriverType::Auto || type == InputDriverType::GameActivity) {
            return std::unique_ptr<IInputDriver>(CreateGameActivityInputDriver());
        }
    #endif

    #if defined(PRISMA_ENABLE_INPUT_SDL3)
        if (type == InputDriverType::Auto || type == InputDriverType::SDL3) {
            return std::unique_ptr<IInputDriver>(CreateSDL3InputDriver());
        }
    #endif

    return nullptr;
}

// ========== 键盘查询 ==========

bool InputDevice::IsKeyDown(KeyCode key) const {
    return m_driver ? m_driver->IsKeyDown(key) : false;
}

bool InputDevice::IsKeyJustPressed(KeyCode key) const {
    return m_driver ? m_driver->IsKeyJustPressed(key) : false;
}

bool InputDevice::IsKeyJustReleased(KeyCode key) const {
    return m_driver ? m_driver->IsKeyJustReleased(key) : false;
}

bool InputDevice::IsAnyKeyDown() const {
    if (!m_driver) {
        return false;
    }

    // 检查常用键
    std::vector<KeyCode> commonKeys = {
        KeyCode::W, KeyCode::A, KeyCode::S, KeyCode::D,
        KeyCode::Up, KeyCode::Down, KeyCode::Left, KeyCode::Right,
        KeyCode::Space, KeyCode::Enter, KeyCode::Escape
    };

    for (auto key : commonKeys) {
        if (m_driver->IsKeyDown(key)) {
            return true;
        }
    }

    return false;
}

// ========== 鼠标查询 ==========

void InputDevice::GetMousePosition(int& x, int& y) const {
    if (!m_driver) {
        x = y = 0;
        return;
    }

    const auto& state = m_driver->GetMouseState();
    x = state.x;
    y = state.y;
}

void InputDevice::GetMouseDelta(int& deltaX, int& deltaY) const {
    if (!m_driver) {
        deltaX = deltaY = 0;
        return;
    }

    const auto& state = m_driver->GetMouseState();
    deltaX = state.deltaX;
    deltaY = state.deltaY;
}

bool InputDevice::IsMouseButtonDown(MouseButton button) const {
    if (!m_driver) {
        return false;
    }

    const auto& state = m_driver->GetMouseState();
    int idx = static_cast<int>(button) - 1;
    if (idx < 0 || idx >= 6) {
        return false;
    }
    return state.buttons[idx].pressed;
}

bool InputDevice::IsMouseButtonJustPressed(MouseButton button) const {
    if (!m_driver) {
        return false;
    }

    const auto& state = m_driver->GetMouseState();
    int idx = static_cast<int>(button) - 1;
    if (idx < 0 || idx >= 6) {
        return false;
    }
    return state.buttons[idx].justPressed;
}

bool InputDevice::IsMouseButtonJustReleased(MouseButton button) const {
    if (!m_driver) {
        return false;
    }

    const auto& state = m_driver->GetMouseState();
    int idx = static_cast<int>(button) - 1;
    if (idx < 0 || idx >= 6) {
        return false;
    }
    return state.buttons[idx].justReleased;
}

int InputDevice::GetMouseWheelDelta() const {
    if (!m_driver) {
        return 0;
    }
    return m_driver->GetMouseState().wheelDelta;
}

// ========== 手柄查询 ==========

uint32_t InputDevice::GetGamepadCount() const {
    return m_driver ? m_driver->GetGamepadCount() : 0;
}

bool InputDevice::IsGamepadConnected(uint32_t index) const {
    return m_driver ? m_driver->IsGamepadConnected(index) : false;
}

bool InputDevice::IsGamepadButtonDown(uint32_t index, GamepadButton button) const {
    if (!m_driver) {
        return false;
    }

    const auto& state = m_driver->GetGamepadState(index);
    int btnIdx = static_cast<int>(button) - 1;
    if (btnIdx < 0 || btnIdx >= 18) {
        return false;
    }
    return state.buttons[btnIdx].pressed;
}

float InputDevice::GetGamepadAxis(uint32_t index, GamepadAxis axis) const {
    if (!m_driver) {
        return 0.0f;
    }

    const auto& state = m_driver->GetGamepadState(index);
    int axisIdx = static_cast<int>(axis);
    if (axisIdx < 0 || axisIdx >= 6) {
        return 0.0f;
    }

    float value = state.axes[axisIdx];

    // 应用死区
    if (std::abs(value) < 0.15f) {
        return 0.0f;
    }

    return value;
}

void InputDevice::SetGamepadVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) {
    if (m_driver) {
        m_driver->SetVibration(index, leftMotor, rightMotor, duration);
    }
}

// ========== 输入映射 ==========

void InputDevice::AddActionMapping(const std::string& name, KeyCode key, KeyCode altKey) {
    ActionMapping mapping;
    mapping.name = name;
    mapping.primaryKey = key;
    mapping.alternateKey = altKey;
    m_actionMappings[name] = std::move(mapping);
}

void InputDevice::AddActionMapping(const std::string& name, GamepadButton button) {
    ActionMapping mapping;
    mapping.name = name;
    mapping.gamepadButton = button;
    m_actionMappings[name] = std::move(mapping);
}

bool InputDevice::IsActionPressed(const std::string& name) const {
    auto it = m_actionMappings.find(name);
    if (it == m_actionMappings.end()) {
        return false;
    }

    const auto& mapping = it->second;

    // 检查键盘
    if (mapping.primaryKey != KeyCode::Unknown) {
        if (IsKeyDown(mapping.primaryKey)) {
            return true;
        }
    }
    if (mapping.alternateKey != KeyCode::Unknown) {
        if (IsKeyDown(mapping.alternateKey)) {
            return true;
        }
    }

    // 检查手柄
    if (mapping.gamepadButton != GamepadButton::None) {
        if (IsGamepadButtonDown(0, mapping.gamepadButton)) {
            return true;
        }
    }

    return false;
}

bool InputDevice::IsActionJustPressed(const std::string& name) const {
    auto it = m_actionMappings.find(name);
    if (it == m_actionMappings.end()) {
        return false;
    }

    const auto& mapping = it->second;

    // 检查键盘
    if (mapping.primaryKey != KeyCode::Unknown) {
        if (IsKeyJustPressed(mapping.primaryKey)) {
            return true;
        }
    }
    if (mapping.alternateKey != KeyCode::Unknown) {
        if (IsKeyJustPressed(mapping.alternateKey)) {
            return true;
        }
    }

    // 检查手柄
    if (mapping.gamepadButton != GamepadButton::None) {
        if (IsGamepadButtonDown(0, mapping.gamepadButton)) {
            // 简化：手柄需要额外状态跟踪
            return true;
        }
    }

    return false;
}

// ========== 文本输入 ==========

void InputDevice::StartTextInput() {
    if (m_driver) {
        m_driver->StartTextInput();
    }
}

void InputDevice::StopTextInput() {
    if (m_driver) {
        m_driver->StopTextInput();
    }
}

const std::string& InputDevice::GetTextInput() const {
    static const std::string empty;
    return m_driver ? m_driver->GetTextInput() : empty;
}

// ========== 光标控制 ==========

void InputDevice::SetCursorVisible(bool visible) {
    m_cursorVisible = visible;
    // TODO: 实现光标显示/隐藏
}

void InputDevice::SetCursorLocked(bool locked) {
    m_cursorLocked = locked;
    // TODO: 实现光标锁定
}

} // namespace PrismaEngine::Input
