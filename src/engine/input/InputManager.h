#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include "math/MathTypes.h"

namespace PrismaEngine {
namespace Input {

// 按键码定义
enum class KeyCode : uint32_t {
    // 字母
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72, I = 73, J = 74, K = 75, L = 76, M = 77,
    N = 78, O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88, Y = 89, Z = 90,

    // 数字
    Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52, Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,

    // 功能键
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295, F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,

    // 特殊键
    Space = 32,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Delete = 261,
    Escape = 256,

    // 方向键
    Left = 263,
    Right = 262,
    Up = 265,
    Down = 264,

    // 修饰键
    LeftShift = 340,
    RightShift = 344,
    LeftCtrl = 341,
    RightCtrl = 345,
    LeftAlt = 342,
    RightAlt = 346,

    // 其他
    Minus = 45,
    Equals = 61,
    LeftBracket = 91,
    RightBracket = 93,
    Backslash = 92,
    Semicolon = 59,
    Apostrophe = 39,
    Comma = 44,
    Period = 46,
    Slash = 47,

    Unknown = 0
};

// 鼠标按钮
enum class MouseButton : uint32_t {
    Left = 0,
    Right = 1,
    Middle = 2,
    X1 = 3,
    X2 = 4,

    Count
};

// 输入动作类型
enum class InputAction {
    Pressed,      // 刚按下
    Released,    // 刚释放
    Held,        // 持续按下
    DoubleClick  // 双击
};

// 输入事件
struct InputEvent {
    enum class Type {
        Key,
        Mouse,
        Scroll,
        MouseMove,
        Touch,
        Gamepad
    } type;

    union {
        struct {
            KeyCode key;
            InputAction action;
            uint32_t modifiers;
        } key;

        struct {
            MouseButton button;
            InputAction action;
            uint32_t modifiers;
        } mouse;

        struct {
            float deltaX;
            float deltaY;
        } scroll;

        struct {
            float x;
            float y;
            float deltaX;
            float deltaY;
        } mouseMove;

        struct {
            int fingerId;
            float x;
            float y;
            InputAction action;
        } touch;
    };
};

// 输入绑定
class InputBinding {
public:
    using Callback = std::function<void()>;

    InputBinding(const std::string& name, KeyCode key, InputAction action, Callback callback);

    // 更新绑定
    void Update();

    // 检查是否触发
    bool IsTriggered() const;

    // 设置启用状态
    void SetEnabled(bool enabled) { m_enabled = enabled; }

private:
    std::string m_name;
    KeyCode m_key;
    InputAction m_action;
    Callback m_callback;
    bool m_enabled = true;
    bool m_wasPressed = false;
};

// 手柄输入
struct GamepadState {
    bool connected = false;
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
    bool buttons[15] = {false}; // 标准手柄按钮数
};

// 输入管理器
class InputManager {
public:
    static InputManager& GetInstance();

    // 初始化
    void Initialize();

    // 更新（每帧调用）
    void Update();

    // 事件处理
    void OnKeyPressed(KeyCode key, uint32_t modifiers);
    void OnKeyReleased(KeyCode key, uint32_t modifiers);
    void OnMouseButtonPressed(MouseButton button, uint32_t modifiers);
    void OnMouseButtonReleased(MouseButton button, uint32_t modifiers);
    void OnMouseMove(float x, float y);
    void OnMouseScroll(float deltaX, float deltaY);
    void OnTouchEvent(int fingerId, float x, float y, InputAction action);

    // 键盘状态查询
    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyJustPressed(KeyCode key) const;
    bool IsKeyJustReleased(KeyCode key) const;

    // 鼠标状态查询
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonJustPressed(MouseButton button) const;
    bool IsMouseButtonJustReleased(MouseButton button) const;
    PrismaMath::vec2 GetMousePosition() const { return m_mousePosition; }
    PrismaMath::vec2 GetMouseDelta() const { return m_mouseDelta; }
    PrismaMath::vec2 GetScrollDelta() const { return m_scrollDelta; }

    // 手柄状态查询
    const GamepadState& GetGamepadState(int gamepadId = 0) const;
    bool IsGamepadConnected(int gamepadId = 0) const;

    // 输入绑定管理
    std::shared_ptr<InputBinding> CreateBinding(const std::string& name,
                                               KeyCode key,
                                               InputAction action,
                                               InputBinding::Callback callback);

    void RemoveBinding(const std::string& name);
    std::shared_ptr<InputBinding> GetBinding(const std::string& name);

    // 输入映射配置
    struct InputMapping {
        std::string actionName;
        KeyCode primaryKey = KeyCode::Unknown;
        KeyCode secondaryKey = KeyCode::Unknown;
        MouseButton mouseButton = MouseButton::Count;
        bool requiresModifier = false;
        KeyCode modifierKey = KeyCode::Unknown;
    };

    void LoadInputMappings(const std::string& filePath);
    void SaveInputMappings(const std::string& filePath);
    void SetInputMapping(const std::string& actionName, const InputMapping& mapping);
    const InputMapping& GetInputMapping(const std::string& actionName) const;

    // 事件系统
    using EventCallback = std::function<void(const InputEvent&)>;
    void RegisterEventCallback(EventCallback callback);

    // 输入模式
    enum class InputMode {
        Game,       // 游戏输入
        UI,         // UI输入
        Debug       // 调试输入
    };

    void SetInputMode(InputMode mode);
    InputMode GetInputMode() const { return m_inputMode; }

    // 光标显示
    void SetCursorVisible(bool visible);
    bool IsCursorVisible() const { return m_cursorVisible; }

    // 光标锁定
    void SetCursorLocked(bool locked);
    bool IsCursorLocked() const { return m_cursorLocked; }

    // 获取字符输入（用于文本输入）
    const std::string& GetInputCharacters() const { return m_inputCharacters; }
    void ClearInputCharacters();

private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    // 键盘状态
    std::unordered_map<KeyCode, bool> m_keyStates;
    std::unordered_map<KeyCode, bool> m_keyStatesPrev;
    std::unordered_map<KeyCode, bool> m_keyPressedThisFrame;
    std::unordered_map<KeyCode, bool> m_keyReleasedThisFrame;

    // 鼠标状态
    std::unordered_map<MouseButton, bool> m_mouseButtonStates;
    std::unordered_map<MouseButton, bool> m_mouseButtonStatesPrev;
    PrismaMath::vec2 m_mousePosition = PrismaMath::vec2(0, 0);
    PrismaMath::vec2 m_mousePositionPrev = PrismaMath::vec2(0, 0);
    PrismaMath::vec2 m_mouseDelta = PrismaMath::vec2(0, 0);
    PrismaMath::vec2 m_scrollDelta = PrismaMath::vec2(0, 0);

    // 手柄状态
    std::unordered_map<int, GamepadState> m_gamepadStates;
    std::unordered_map<int, GamepadState> m_gamepadStatesPrev;

    // 输入绑定
    std::unordered_map<std::string, std::shared_ptr<InputBinding>> m_bindings;

    // 输入映射
    std::unordered_map<std::string, InputMapping> m_inputMappings;

    // 事件回调
    std::vector<EventCallback> m_eventCallbacks;

    // 模式和设置
    InputMode m_inputMode = InputMode::Game;
    bool m_cursorVisible = true;
    bool m_cursorLocked = false;

    // 字符输入
    std::string m_inputCharacters;

    // 更新手柄状态
    void UpdateGamepadStates();

    // 处理输入绑定
    void ProcessBindings();

    // 发送事件
    void SendEvent(const InputEvent& event);
};

// 便利函数
inline InputManager& GetInputManager() {
    return InputManager::GetInstance();
}

// 快速输入检查
inline bool IsKeyDown(KeyCode key) {
    return GetInputManager().IsKeyPressed(key);
}

inline bool IsKeyJustDown(KeyCode key) {
    return GetInputManager().IsKeyJustPressed(key);
}

inline bool IsMouseDown(MouseButton button) {
    return GetInputManager().IsMouseButtonPressed(button);
}

inline PrismaMath::vec2 GetMousePos() {
    return GetInputManager().GetMousePosition();
}

} // namespace Input
} // namespace Engine