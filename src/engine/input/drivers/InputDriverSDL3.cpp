#if defined(PRISMA_ENABLE_INPUT_SDL3)
#include "InputDriverSDL3.h"
#include <algorithm>

namespace PrismaEngine::Input {

// ========== InputDriverSDL3 ==========

InputDriverSDL3::InputDriverSDL3() {
    m_mouseState = {0, 0, 0, 0, {{false, false, false}}, 0};

    for (auto& state : m_gamepadStates) {
        state.connected = false;
        std::fill_n(state.axes, 6, 0.0f);
    }
}

InputDriverSDL3::~InputDriverSDL3() {
    Shutdown();
}

bool InputDriverSDL3::Initialize() {
    if (m_initialized) {
        return true;
    }

    // SDL3 应该在外部初始化，这里只检查
    if (!SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        // 初始化 SDL 子系统
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD) < 0) {
            return false;
        }
    }

    m_initialized = true;
    return true;
}

void InputDriverSDL3::Shutdown() {
    if (!m_initialized) {
        return;
    }

    m_initialized = false;
}

void InputDriverSDL3::Update() {
    if (!m_initialized) {
        return;
    }

    // 保存上一帧状态
    m_prevKeyStates = m_keyStates;

    // 重置鼠标相对移动
    m_mouseState.deltaX = 0;
    m_mouseState.deltaY = 0;
    m_mouseState.wheelDelta = 0;

    // 处理事件
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ProcessEvent(event);
    }

    // 更新手柄
    UpdateGamepads();

    // 清空文本输入
    m_textInput.clear();
}

void InputDriverSDL3::ProcessEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            bool down = (event.type == SDL_EVENT_KEY_DOWN);
            KeyCode key = MapSDLKey(static_cast<SDL_Keycode>(event.key.key));
            if (key != KeyCode::Unknown) {
                m_keyStates[static_cast<int>(key)] = down;
            }

            // 文本输入
            if (down && m_textInputEnabled && event.key.text) {
                m_textInput += event.key.text;
            }
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            m_mouseState.x = event.motion.x;
            m_mouseState.y = event.motion.y;
            m_mouseState.deltaX += event.motion.xrel;
            m_mouseState.deltaY += event.motion.yrel;
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {
            m_mouseState.wheelDelta = event.wheel.y;
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            bool down = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            MouseButton btn = static_cast<MouseButton>(event.button.button);
            int idx = static_cast<int>(btn) - 1;
            if (idx >= 0 && idx < 6) {
                m_mouseState.buttons[idx].pressed = down;
                m_mouseState.buttons[idx].justReleased = !down;
            }
            break;
        }

        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED:
        case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
        case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
        case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            // 手柄事件在 UpdateGamepads 中处理
            break;

        default:
            break;
    }
}

void InputDriverSDL3::UpdateGamepads() {
    // 更新所有连接的手柄
    for (int i = 0; i < SDL_GetNumGamepads(); ++i) {
        SDL_Gamepad* gamepad = SDL_GetGamepad(i);
        if (!gamepad) {
            continue;
        }

        SDL_JoystickID instanceId = SDL_GetGamepadInstanceID(gamepad);
        GamepadState& state = m_gamepadStates[i];
        state.connected = true;

        // 按钮
        auto UpdateButton = [&](SDL_GamepadButton sdlBtn, GamepadButton btn) {
            int idx = static_cast<int>(btn) - 1;
            if (idx < 0 || idx >= 18) return;

            bool pressed = SDL_GetGamepadButton(gamepad, sdlBtn);
            state.buttons[idx].pressed = pressed;
            state.buttons[idx].justPressed = pressed && !state.buttons[idx].pressed;
            state.buttons[idx].justReleased = !pressed && state.buttons[idx].pressed;
        };

        UpdateButton(SDL_GAMEPAD_BUTTON_A, GamepadButton::A);
        UpdateButton(SDL_GAMEPAD_BUTTON_B, GamepadButton::B);
        UpdateButton(SDL_GAMEPAD_BUTTON_X, GamepadButton::X);
        UpdateButton(SDL_GAMEPAD_BUTTON_Y, GamepadButton::Y);
        UpdateButton(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, GamepadButton::LeftShoulder);
        UpdateButton(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, GamepadButton::RightShoulder);
        UpdateButton(SDL_GAMEPAD_BUTTON_BACK, GamepadButton::Back);
        UpdateButton(SDL_GAMEPAD_BUTTON_START, GamepadButton::Start);
        UpdateButton(SDL_GAMEPAD_BUTTON_LEFT_STICK, GamepadButton::LeftStick);
        UpdateButton(SDL_GAMEPAD_BUTTON_RIGHT_STICK, GamepadButton::RightStick);
        UpdateButton(SDL_GAMEPAD_BUTTON_DPAD_UP, GamepadButton::DPadUp);
        UpdateButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN, GamepadButton::DPadDown);
        UpdateButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT, GamepadButton::DPadLeft);
        UpdateButton(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, GamepadButton::DPadRight);

        // 轴
        state.axes[0] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        state.axes[1] = -SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        state.axes[2] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        state.axes[3] = -SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
        state.axes[4] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        state.axes[5] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

        SDL_CloseGamepad(gamepad);
    }
}

KeyCode InputDriverSDL3::MapSDLKey(SDL_Keycode sdlKey) const {
    static const std::unordered_map<SDL_Keycode, KeyCode> keyMap = {
        {SDLK_RETURN, KeyCode::Enter},
        {SDLK_ESCAPE, KeyCode::Escape},
        {SDLK_BACKSPACE, KeyCode::Backspace},
        {SDLK_TAB, KeyCode::Tab},
        {SDLK_SPACE, KeyCode::Space},
        {SDLK_LSHIFT, KeyCode::Shift}, {SDLK_RSHIFT, KeyCode::Shift},
        {SDLK_LCTRL, KeyCode::Ctrl}, {SDLK_RCTRL, KeyCode::Ctrl},
        {SDLK_LALT, KeyCode::Alt}, {SDLK_RALT, KeyCode::Alt},
        {SDLK_CAPSLOCK, KeyCode::CapsLock},
        {SDLK_PAGEUP, KeyCode::PageUp},
        {SDLK_PAGEDOWN, KeyCode::PageDown},
        {SDLK_END, KeyCode::End},
        {SDLK_HOME, KeyCode::Home},
        {SDLK_LEFT, KeyCode::Left},
        {SDLK_UP, KeyCode::Up},
        {SDLK_RIGHT, KeyCode::Right},
        {SDLK_DOWN, KeyCode::Down},
        {SDLK_PRINTSCREEN, KeyCode::PrintScreen},
        {SDLK_INSERT, KeyCode::Insert},
        {SDLK_DELETE, KeyCode::Delete},
        {SDLK_0, KeyCode::Num0}, {SDLK_1, KeyCode::Num1}, {SDLK_2, KeyCode::Num2},
        {SDLK_3, KeyCode::Num3}, {SDLK_4, KeyCode::Num4}, {SDLK_5, KeyCode::Num5},
        {SDLK_6, KeyCode::Num6}, {SDLK_7, KeyCode::Num7}, {SDLK_8, KeyCode::Num8},
        {SDLK_9, KeyCode::Num9},
        {SDLK_A, KeyCode::A}, {SDLK_B, KeyCode::B}, {SDLK_C, KeyCode::C},
        {SDLK_D, KeyCode::D}, {SDLK_E, KeyCode::E}, {SDLK_F, KeyCode::F},
        {SDLK_G, KeyCode::G}, {SDLK_H, KeyCode::H}, {SDLK_I, KeyCode::I},
        {SDLK_J, KeyCode::J}, {SDLK_K, KeyCode::K}, {SDLK_L, KeyCode::L},
        {SDLK_M, KeyCode::M}, {SDLK_N, KeyCode::N}, {SDLK_O, KeyCode::O},
        {SDLK_P, KeyCode::P}, {SDLK_Q, KeyCode::Q}, {SDLK_R, KeyCode::R},
        {SDLK_S, KeyCode::S}, {SDLK_T, KeyCode::T}, {SDLK_U, KeyCode::U},
        {SDLK_V, KeyCode::V}, {SDLK_W, KeyCode::W}, {SDLK_X, KeyCode::X},
        {SDLK_Y, KeyCode::Y}, {SDLK_Z, KeyCode::Z},
        {SDLK_F1, KeyCode::F1}, {SDLK_F2, KeyCode::F2}, {SDLK_F3, KeyCode::F3},
        {SDLK_F4, KeyCode::F4}, {SDLK_F5, KeyCode::F5}, {SDLK_F6, KeyCode::F6},
        {SDLK_F7, KeyCode::F7}, {SDLK_F8, KeyCode::F8}, {SDLK_F9, KeyCode::F9},
        {SDLK_F10, KeyCode::F10}, {SDLK_F11, KeyCode::F11}, {SDLK_F12, KeyCode::F12},
    };

    auto it = keyMap.find(sdlKey);
    return (it != keyMap.end()) ? it->second : KeyCode::Unknown;
}

// ========== 键盘查询 ==========

bool InputDriverSDL3::IsKeyDown(KeyCode key) const {
    int idx = static_cast<int>(key);
    return (idx >= 0 && idx < MAX_KEYS) ? m_keyStates[idx] : false;
}

bool InputDriverSDL3::IsKeyJustPressed(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= MAX_KEYS) {
        return false;
    }
    return m_keyStates[idx] && !m_prevKeyStates[idx];
}

bool InputDriverSDL3::IsKeyJustReleased(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= MAX_KEYS) {
        return false;
    }
    return !m_keyStates[idx] && m_prevKeyStates[idx];
}

// ========== 鼠标 ==========

void InputDriverSDL3::SetMousePosition(int x, int y) {
    SDL_WarpMouseInWindow(nullptr, x, y);
}

void InputDriverSDL3::StartTextInput() {
    SDL_StartTextInput();
    m_textInputEnabled = true;
}

void InputDriverSDL3::StopTextInput() {
    SDL_StopTextInput();
    m_textInputEnabled = false;
}

// ========== 手柄查询 ==========

uint32_t InputDriverSDL3::GetGamepadCount() const {
    return SDL_GetNumGamepads();
}

bool InputDriverSDL3::IsGamepadConnected(uint32_t index) const {
    if (index >= MAX_GAMEPADS) {
        return false;
    }
    return m_gamepadStates[index].connected;
}

const GamepadState& InputDriverSDL3::GetGamepadState(uint32_t index) const {
    if (index >= MAX_GAMEPADS) {
        static const GamepadState empty{};
        return empty;
    }
    return m_gamepadStates[index];
}

void InputDriverSDL3::SetVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) {
    if (index >= MAX_GAMEPADS) {
        return;
    }

    SDL_JoystickID instanceId = m_gamepadIds[index];
    SDL_Joystick* joystick = SDL_OpenJoystick(instanceId);
    if (joystick) {
        Uint16 low = static_cast<Uint16>(leftMotor * 65535.0f);
        Uint16 high = static_cast<Uint16>(rightMotor * 65535.0f);
        SDL_SetJoystickVibration(joystick, low, high, duration);
        SDL_CloseJoystick(joystick);
    }
}

} // namespace PrismaEngine::Input
#endif
