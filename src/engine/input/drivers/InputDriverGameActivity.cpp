#if defined(__ANDROID__) || defined(PRISMA_ENABLE_INPUT_GAMEACTIVITY)
#include "InputDriverGameActivity.h"
#include <algorithm>

namespace PrismaEngine::Input {

// ========== InputDriverGameActivity ==========

InputDriverGameActivity::InputDriverGameActivity() {
    m_touches.fill({0, 0.0f, 0.0f, false});

    for (auto& state : m_gamepadStates) {
        state.connected = false;
        std::fill_n(state.axes, 6, 0.0f);
    }

    m_mouseState = {0, 0, 0, 0, {{false, false, false}}, 0};
}

InputDriverGameActivity::~InputDriverGameActivity() {
    Shutdown();
}

bool InputDriverGameActivity::Initialize() {
    if (m_initialized) {
        return true;
    }

    if (!m_activity) {
        return false;
    }

    // 获取屏幕尺寸
    auto* window = m_activity->window;
    if (window) {
        m_screenWidth = window->size.x;
        m_screenHeight = window->size.y;
    }

    m_initialized = true;
    return true;
}

void InputDriverGameActivity::Shutdown() {
    m_initialized = false;
}

void InputDriverGameActivity::Update() {
    if (!m_initialized) {
        return;
    }

    // 保存上一帧状态
    m_prevKeyStates = m_keyStates;

    // 更新手柄状态
    UpdateGamepadStates();

    // 重置相对移动
    m_mouseState.deltaX = 0;
    m_mouseState.deltaY = 0;

    // 重置滚轮
    m_mouseState.wheelDelta = 0;

    // 清空文本输入
    m_textInput.clear();
}

void InputDriverGameActivity::SetGameActivity(GameActivity* activity) {
    m_activity = activity;

    if (activity && activity->window) {
        m_screenWidth = activity->window->size.x;
        m_screenHeight = activity->window->size.y;
    }
}

void InputDriverGameActivity::HandleInputEvent(const AInputEvent* event) {
    if (!event) {
        return;
    }

    int type = AInputEvent_getType(event);

    switch (type) {
        case AINPUT_EVENT_TYPE_KEY:
            ProcessKeyEvent(event);
            break;

        case AINPUT_EVENT_TYPE_MOTION:
            ProcessMotionEvent(event);
            break;

        default:
            break;
    }
}

void InputDriverGameActivity::ProcessKeyEvent(const AInputEvent* event) {
    int action = AKeyEvent_getAction(event);
    int keyCode = AKeyEvent_getKeyCode(event);

    // 映射按键
    KeyCode key = MapAndroidKeyCode(keyCode);
    if (key != KeyCode::Unknown) {
        bool down = (action == AKEY_EVENT_ACTION_DOWN);
        m_keyStates[static_cast<int>(key)] = down;
    }

    // 文本输入
    if (m_textInputEnabled && action == AKEY_EVENT_ACTION_DOWN) {
        int metaState = AKeyEvent_getMetaState(event);
        int unicodeChar = AKeyEvent_getKeyCode(event);

        // 简化处理：仅处理基本字符
        if (unicodeChar >= 0x20 && unicodeChar <= 0x7E) {
            m_textInput += static_cast<char>(unicodeChar);
        }
    }
}

void InputDriverGameActivity::ProcessTouchEvent(const AInputEvent* event) {
    int action = AMotionEvent_getAction(event);
    int actionMasked = action & AMOTION_EVENT_ACTION_MASK;
    int pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    int pointerId = AMotionEvent_getPointerId(event, pointerIndex);
    float x = AMotionEvent_getX(event, pointerIndex);
    float y = AMotionEvent_getY(event, pointerIndex);

    // 更新触摸点
    for (auto& touch : m_touches) {
        if (touch.id == pointerId || !touch.isDown) {
            touch.id = pointerId;
            touch.x = x;
            touch.y = y;

            if (actionMasked == AMOTION_EVENT_ACTION_DOWN || actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                touch.isDown = true;
            } else if (actionMasked == AMOTION_EVENT_ACTION_UP || actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
                touch.isDown = false;
            } else if (actionMasked == AMOTION_EVENT_ACTION_MOVE) {
                // 更新位置
            }
            break;
        }
    }

    // 将第一个触摸点映射为鼠标
    for (const auto& touch : m_touches) {
        if (touch.isDown) {
            int newX = static_cast<int>(touch.x);
            int newY = static_cast<int>(touch.y);

            m_mouseState.deltaX = newX - m_mouseState.x;
            m_mouseState.deltaY = newY - m_mouseState.y;
            m_mouseState.x = newX;
            m_mouseState.y = newY;

            // 左键状态
            m_mouseState.buttons[0].pressed = true;
            break;
        }
    }
}

void InputDriverGameActivity::ProcessMotionEvent(const AInputEvent* event) {
    int source = AMotionEvent_getSource(event);

    // 触摸事件
    if (source == AINPUT_SOURCE_TOUCHSCREEN) {
        ProcessTouchEvent(event);
        return;
    }

    // 手柄事件
    if (source & AINPUT_SOURCE_GAMEPAD ||
        source & AINPUT_SOURCE_JOYSTICK) {

        int axisCount = AMotionEvent_getAxisCount(event);
        int pointerId = AMotionEvent_getPointerId(event, 0);

        // 查找或分配手柄槽位
        int slot = -1;
        for (int i = 0; i < 4; ++i) {
            if (m_gamepadDeviceIds[i] == pointerId) {
                slot = i;
                break;
            }
            if (slot < 0 && m_gamepadDeviceIds[i] < 0) {
                slot = i;
            }
        }

        if (slot < 0) {
            return;  // 没有可用槽位
        }

        m_gamepadDeviceIds[slot] = pointerId;
        GamepadState& state = m_gamepadStates[slot];
        state.connected = true;

        // 读取按钮
        int action = AMotionEvent_getAction(event);
        bool pressed = (action == AMOTION_EVENT_ACTION_DOWN ||
                        action == AMOTION_EVENT_ACTION_BUTTON_PRESS);

        // 映射按钮（简化）
        static const std::unordered_map<int, GamepadButton> buttonMap = {
            {AMOTION_EVENT_BUTTON_A, GamepadButton::A},
            {AMOTION_EVENT_BUTTON_B, GamepadButton::B},
            {AMOTION_EVENT_BUTTON_X, GamepadButton::X},
            {AMOTION_EVENT_BUTTON_Y, GamepadButton::Y},
            {AMOTION_EVENT_BUTTON_L1, GamepadButton::LeftShoulder},
            {AMOTION_EVENT_BUTTON_R1, GamepadButton::RightShoulder},
            {AMOTION_EVENT_BUTTON_L2, GamepadButton::LeftTrigger},
            {AMOTION_EVENT_BUTTON_R2, GamepadButton::RightTrigger},
            {AMOTION_EVENT_BUTTON_SELECT, GamepadButton::Back},
            {AMOTION_EVENT_BUTTON_START, GamepadButton::Start},
            {AMOTION_EVENT_BUTTON_THUMBL, GamepadButton::LeftStick},
            {AMOTION_EVENT_BUTTON_THUMBR, GamepadButton::RightStick},
        };

        int btnCode = AMotionEvent_getActionButton(event);
        auto it = buttonMap.find(btnCode);
        if (it != buttonMap.end()) {
            int idx = static_cast<int>(it->second) - 1;
            state.buttons[idx].pressed = pressed;
        }

        // 方向键
        int dpadState = 0;
        if (axisCount > AMOTION_EVENT_AXIS_HAT_X) {
            dpadState = static_cast<int>(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X));
        }

        // 读取轴
        if (axisCount > AMOTION_EVENT_AXIS_X) {
            state.axes[0] = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X) / 32768.0f;
        }
        if (axisCount > AMOTION_EVENT_AXIS_Y) {
            state.axes[1] = -AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y) / 32768.0f;
        }
        if (axisCount > AMOTION_EVENT_AXIS_Z) {
            state.axes[2] = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z) / 32768.0f;
        }
        if (axisCount > AMOTION_EVENT_AXIS_RZ) {
            state.axes[3] = -AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ) / 32768.0f;
        }
        if (axisCount > AMOTION_EVENT_AXIS_LTRIGGER) {
            state.axes[4] = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_LTRIGGER) / 255.0f;
        }
        if (axisCount > AMOTION_EVENT_AXIS_RTRIGGER) {
            state.axes[5] = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RTRIGGER) / 255.0f;
        }

        // 方向键映射到轴
        if (dpadState != 0) {
            if (dpadState & AMOTION_EVENT_AXIS_HAT_LEFT) {
                state.axes[0] = -1.0f;
                state.buttons[static_cast<int>(GamepadButton::DPadLeft) - 1].pressed = true;
            } else if (dpadState & AMOTION_EVENT_AXIS_HAT_RIGHT) {
                state.axes[0] = 1.0f;
                state.buttons[static_cast<int>(GamepadButton::DPadRight) - 1].pressed = true;
            }
        }
    }
}

void InputDriverGameActivity::UpdateGamepadStates() {
    // Android 手柄状态通过事件实时更新，这里只需处理振动超时等
}

KeyCode InputDriverGameActivity::MapAndroidKeyCode(int keyCode) const {
    static const std::unordered_map<int, KeyCode> keyMap = {
        {AKEYCODE_ENTER, KeyCode::Enter},
        {AKEYCODE_DEL, KeyCode::Backspace},
        {AKEYCODE_TAB, KeyCode::Tab},
        {AKEYCODE_SPACE, KeyCode::Space},
        {AKEYCODE_SHIFT_LEFT, KeyCode::Shift},
        {AKEYCODE_SHIFT_RIGHT, KeyCode::Shift},
        {AKEYCODE_CTRL_LEFT, KeyCode::Ctrl},
        {AKEYCODE_CTRL_RIGHT, KeyCode::Ctrl},
        {AKEYCODE_ALT_LEFT, KeyCode::Alt},
        {AKEYCODE_ALT_RIGHT, KeyCode::Alt},
        {AKEYCODE_DPAD_UP, KeyCode::Up},
        {AKEYCODE_DPAD_DOWN, KeyCode::Down},
        {AKEYCODE_DPAD_LEFT, KeyCode::Left},
        {AKEYCODE_DPAD_RIGHT, KeyCode::Right},
        {AKEYCODE_0, KeyCode::Num0}, {AKEYCODE_1, KeyCode::Num1},
        {AKEYCODE_2, KeyCode::Num2}, {AKEYCODE_3, KeyCode::Num3},
        {AKEYCODE_4, KeyCode::Num4}, {AKEYCODE_5, KeyCode::Num5},
        {AKEYCODE_6, KeyCode::Num6}, {AKEYCODE_7, KeyCode::Num7},
        {AKEYCODE_8, KeyCode::Num8}, {AKEYCODE_9, KeyCode::Num9},
        {AKEYCODE_A, KeyCode::A}, {AKEYCODE_B, KeyCode::B},
        {AKEYCODE_C, KeyCode::C}, {AKEYCODE_D, KeyCode::D},
        {AKEYCODE_E, KeyCode::E}, {AKEYCODE_F, KeyCode::F},
        {AKEYCODE_G, KeyCode::G}, {AKEYCODE_H, KeyCode::H},
        {AKEYCODE_I, KeyCode::I}, {AKEYCODE_J, KeyCode::J},
        {AKEYCODE_K, KeyCode::K}, {AKEYCODE_L, KeyCode::L},
        {AKEYCODE_M, KeyCode::M}, {AKEYCODE_N, KeyCode::N},
        {AKEYCODE_O, KeyCode::O}, {AKEYCODE_P, KeyCode::P},
        {AKEYCODE_Q, KeyCode::Q}, {AKEYCODE_R, KeyCode::R},
        {AKEYCODE_S, KeyCode::S}, {AKEYCODE_T, KeyCode::T},
        {AKEYCODE_U, KeyCode::U}, {AKEYCODE_V, KeyCode::V},
        {AKEYCODE_W, KeyCode::W}, {AKEYCODE_X, KeyCode::X},
        {AKEYCODE_Y, KeyCode::Y}, {AKEYCODE_Z, KeyCode::Z},
        {AKEYCODE_F1, KeyCode::F1}, {AKEYCODE_F2, KeyCode::F2},
        {AKEYCODE_F3, KeyCode::F3}, {AKEYCODE_F4, KeyCode::F4},
        {AKEYCODE_F5, KeyCode::F5}, {AKEYCODE_F6, KeyCode::F6},
        {AKEYCODE_F7, KeyCode::F7}, {AKEYCODE_F8, KeyCode::F8},
        {AKEYCODE_F9, KeyCode::F9}, {AKEYCODE_F10, KeyCode::F10},
        {AKEYCODE_F11, KeyCode::F11}, {AKEYCODE_F12, KeyCode::F12},
        {AKEYCODE_ESCAPE, KeyCode::Escape},
    };

    auto it = keyMap.find(keyCode);
    return (it != keyMap.end()) ? it->second : KeyCode::Unknown;
}

// ========== 键盘查询 ==========

bool InputDriverGameActivity::IsKeyDown(KeyCode key) const {
    int idx = static_cast<int>(key);
    return (idx >= 0 && idx < MAX_KEYS) ? m_keyStates[idx] : false;
}

bool InputDriverGameActivity::IsKeyJustPressed(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= MAX_KEYS) {
        return false;
    }
    return m_keyStates[idx] && !m_prevKeyStates[idx];
}

bool InputDriverGameActivity::IsKeyJustReleased(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= MAX_KEYS) {
        return false;
    }
    return !m_keyStates[idx] && m_prevKeyStates[idx];
}

// ========== 鼠标 ==========

void InputDriverGameActivity::SetMousePosition(int x, int y) {
    m_mouseState.x = x;
    m_mouseState.y = y;
}

// ========== 手柄查询 ==========

bool InputDriverGameActivity::IsGamepadConnected(uint32_t index) const {
    if (index >= 4) {
        return false;
    }
    return m_gamepadStates[index].connected;
}

const GamepadState& InputDriverGameActivity::GetGamepadState(uint32_t index) const {
    if (index >= 4) {
        static const GamepadState empty{};
        return empty;
    }
    return m_gamepadStates[index];
}

void InputDriverGameActivity::SetVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) {
    // Android 振动通过 GameActivity 或 Vibrator API 实现
    // 这里简化处理
}

} // namespace PrismaEngine::Input
#endif
