#pragma once

#if defined(__ANDROID__) && defined(PRISMA_ENABLE_INPUT_GAMEACTIVITY)

#include "core/IInputDriver.h"
#include <game-activity/GameActivity.h>
#include <android/input.h>
#include <array>
#include <memory>

namespace PrismaEngine::Input {

/// @brief Android GameActivity 输入驱动
/// 使用 GameActivity API 处理触摸和键盘输入
class InputDriverGameActivity : public IInputDriver {
public:
    InputDriverGameActivity();
    ~InputDriverGameActivity() override;

    const char* GetName() const override { return "GameActivity"; }

    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override { return m_initialized; }
    void Update() override;

    // 键盘
    bool IsKeyDown(KeyCode key) const override;
    bool IsKeyJustPressed(KeyCode key) const override;
    bool IsKeyJustReleased(KeyCode key) const override;

    // 鼠标（在 Android 上作为触摸处理）
    const MouseState& GetMouseState() const override { return m_mouseState; }
    void SetMousePosition(int x, int y) override;
    bool SupportsAbsolutePosition() const override { return true; }

    // 手柄（Android 支持外接手柄）
    uint32_t GetGamepadCount() const override { return 4; }
    bool IsGamepadConnected(uint32_t index) const override;
    const GamepadState& GetGamepadState(uint32_t index) const override;
    void SetVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) override;

    // 文本输入
    const std::string& GetTextInput() const override { return m_textInput; }
    void StartTextInput() override { m_textInputEnabled = true; }
    void StopTextInput() override { m_textInputEnabled = false; }

    // ========== GameActivity 回调 ==========

    /// @brief 处理 GameActivity 输入事件
    /// @param event Android 输入事件
    void HandleInputEvent(const AInputEvent* event);

    /// @brief 设置 GameActivity 实例
    void SetGameActivity(GameActivity* activity);

private:
    // ========== 常量 ==========
    static constexpr uint32_t MAX_KEYS = 256;
    static constexpr uint32_t MAX_TOUCHES = 10;  // Android 最多支持 10 点触控

    // ========== 触摸处理 ==========
    void ProcessTouchEvent(const AInputEvent* event);

    /// @brief 触摸点
    struct TouchPoint {
        int id;
        float x, y;
        bool isDown;
    };

    // ========== 按键处理 ==========
    void ProcessKeyEvent(const AInputEvent* event);
    KeyCode MapAndroidKeyCode(int keyCode) const;

    // ========== 手柄处理 ==========
    void ProcessMotionEvent(const AInputEvent* event);
    void UpdateGamepadStates();

    // ========== 成员变量 ==========

    // 键盘状态
    std::array<bool, MAX_KEYS> m_keyStates{};
    std::array<bool, MAX_KEYS> m_prevKeyStates{};

    // 触摸点
    std::array<TouchPoint, MAX_TOUCHES> m_touches{};

    // 鼠标状态（将触摸映射为鼠标）
    MouseState m_mouseState{};

    // 手柄状态
    std::array<GamepadState, 4> m_gamepadStates{};
    int32_t m_gamepadDeviceIds[4] = {-1, -1, -1, -1};

    // 文本输入
    std::string m_textInput;
    bool m_textInputEnabled = false;

    // 屏幕尺寸
    int m_screenWidth = 0;
    int m_screenHeight = 0;

    // GameActivity
    GameActivity* m_activity = nullptr;

    // 状态
    bool m_initialized = false;
};

/// @brief 创建 GameActivity 输入驱动实例
inline IInputDriver* CreateGameActivityInputDriver() {
    return new InputDriverGameActivity();
}

} // namespace PrismaEngine::Input

#endif // __ANDROID__ && PRISMA_ENABLE_INPUT_GAMEACTIVITY
