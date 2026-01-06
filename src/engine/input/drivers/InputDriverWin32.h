#pragma once

#if defined(_WIN32) && (defined(PRISMA_ENABLE_INPUT_RAWINPUT) || defined(PRISMA_ENABLE_INPUT_XINPUT))

#include "core/IInputDriver.h"
#include <windows.h>
#include <XInput.h>
#include <array>
#include <unordered_map>

namespace PrismaEngine::Input {

/// @brief Windows 输入驱动 (RawInput + XInput)
/// - RawInput: 键盘和鼠标
/// - XInput: 游戏手柄
class InputDriverWin32 : public IInputDriver {
public:
    InputDriverWin32();
    ~InputDriverWin32() override;

    const char* GetName() const override { return "Win32"; }

    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override { return m_initialized; }
    void Update() override;

    // 键盘
    bool IsKeyDown(KeyCode key) const override;
    bool IsKeyJustPressed(KeyCode key) const override;
    bool IsKeyJustReleased(KeyCode key) const override;

    // 鼠标
    const MouseState& GetMouseState() const override { return m_mouseState; }
    void SetMousePosition(int x, int y) override;
    bool SupportsAbsolutePosition() const override { return true; }

    // 手柄
    uint32_t GetGamepadCount() const override { return XUSER_MAX_COUNT; }
    bool IsGamepadConnected(uint32_t index) const override;
    const GamepadState& GetGamepadState(uint32_t index) const override;
    void SetVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) override;

    // 文本输入
    const std::string& GetTextInput() const override { return m_textInput; }
    void StartTextInput() override { m_textInputEnabled = true; }
    void StopTextInput() override { m_textInputEnabled = false; }

private:
    // ========== 常量 ==========
    static constexpr uint32_t MAX_KEYS = 256;
    static constexpr uint32_t MAX_BUTTONS = 6;

    // ========== RawInput 初始化 ==========
    bool RegisterRawInputDevices();
    void ProcessRawInput(const RAWINPUT* raw);

    // ========== XInput 更新 ==========
    void UpdateXInput();

    // ========== 键盘映射 ==========
    KeyCode MapVirtualKeyToKeyCode(UINT virtualKey) const;

    // ========== 成员变量 ==========

    // 键盘状态 (当前帧和上一帧)
    std::array<bool, MAX_KEYS> m_keyStates{};
    std::array<bool, MAX_KEYS> m_prevKeyStates{};

    // 鼠标状态
    MouseState m_mouseState{};
    int m_mouseWheelAccumulator = 0;

    // 手柄状态
    std::array<GamepadState, XUSER_MAX_COUNT> m_gamepadStates{};
    std::array<XINPUT_VIBRATION, XUSER_MAX_COUNT> m_vibrationTimers{};

    // 文本输入
    std::string m_textInput;
    bool m_textInputEnabled = false;

    // 窗口句柄
    HWND m_hwnd = nullptr;

    // 状态
    bool m_initialized = false;
};

/// @brief 创建 Win32 输入驱动实例
inline IInputDriver* CreateWin32InputDriver() {
    return new InputDriverWin32();
}

} // namespace PrismaEngine::Input

#endif // _WIN32 && (PRISMA_ENABLE_INPUT_RAWINPUT || PRISMA_ENABLE_INPUT_XINPUT)
