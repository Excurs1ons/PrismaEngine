#pragma once

#if defined(PRISMA_ENABLE_INPUT_SDL3)

#include "core/IInputDriver.h"
#include <SDL3/SDL.h>
#include <array>

namespace PrismaEngine::Input {

/// @brief SDL3 跨平台输入驱动
/// 支持：键盘、鼠标、手柄
class InputDriverSDL3 : public IInputDriver {
public:
    InputDriverSDL3();
    ~InputDriverSDL3() override;

    const char* GetName() const override { return "SDL3"; }

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
    uint32_t GetGamepadCount() const override;
    bool IsGamepadConnected(uint32_t index) const override;
    const GamepadState& GetGamepadState(uint32_t index) const override;
    void SetVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) override;

    // 文本输入
    const std::string& GetTextInput() const override { return m_textInput; }
    void StartTextInput() override;
    void StopTextInput() override;

private:
    // ========== 常量 ==========
    static constexpr uint32_t MAX_KEYS = 256;
    static constexpr uint32_t MAX_GAMEPADS = 16;

    // ========== SDL3 事件处理 ==========
    void ProcessEvent(const SDL_Event& event);

    /// @brief 映射 SDL 键码
    KeyCode MapSDLKey(SDL_Keycode sdlKey) const;

    /// @brief 映射 SDL 手柄按钮
    GamepadButton MapSDLGamepadButton(SDL_GamepadButton button) const;

    // ========== 手柄更新 ==========
    void UpdateGamepads();

    // ========== 成员变量 ==========

    // 键盘状态
    std::array<bool, MAX_KEYS> m_keyStates{};
    std::array<bool, MAX_KEYS> m_prevKeyStates{};

    // 鼠标状态
    MouseState m_mouseState{};

    // 手柄状态
    std::array<GamepadState, MAX_GAMEPADS> m_gamepadStates{};
    SDL_JoystickID m_gamepadIds[MAX_GAMEPADS] = {};

    // 文本输入
    std::string m_textInput;
    bool m_textInputEnabled = false;

    // 状态
    bool m_initialized = false;
};

/// @brief 创建 SDL3 输入驱动实例
inline IInputDriver* CreateSDL3InputDriver() {
    return new InputDriverSDL3();
}

} // namespace PrismaEngine::Input

#endif // PRISMA_ENABLE_INPUT_SDL3
