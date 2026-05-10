#if defined(PRISMA_ENABLE_INPUT_SDL3)
#include "InputDriverSDL3.h"
#include "../logger/Logger.h"
#include <algorithm>
#include <unordered_map>

namespace Prisma::Input {

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

    if (!SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            LOG_ERROR("Input", "SDL 输入子系统初始化失败: {0}", SDL_GetError());
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

    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (m_openGamepads[i]) {
            SDL_CloseGamepad(m_openGamepads[i]);
            m_openGamepads[i] = nullptr;
        }
    }

    m_initialized = false;
}

void InputDriverSDL3::Update() {
    if (!m_initialized) {
        return;
    }

    m_prevKeyStates = m_keyStates;

    m_mouseState.deltaX = 0;
    m_mouseState.deltaY = 0;
    m_mouseState.wheelDelta = 0;

    // 更新已连接的手柄状态，不再每帧 Open/Close
    UpdateGamepads();

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
            break;
        }

        case SDL_EVENT_TEXT_INPUT: {
            if (m_textInputEnabled && event.text.text) {
                m_textInput += event.text.text;
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

        case SDL_EVENT_GAMEPAD_ADDED: {
            LOG_INFO("Input", "检测到手柄连接 ID: {0}", event.gdevice.which);
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            LOG_INFO("Input", "检测到手柄断开 ID: {0}", event.gdevice.which);
            for (int i = 0; i < MAX_GAMEPADS; ++i) {
                if (m_gamepadIds[i] == event.gdevice.which) {
                    if (m_openGamepads[i]) {
                        SDL_CloseGamepad(m_openGamepads[i]);
                        m_openGamepads[i] = nullptr;
                    }
                    m_gamepadIds[i] = 0;
                    m_gamepadStates[i].connected = false;
                }
            }
            break;
        }

        default:
            break;
    }
}

void InputDriverSDL3::UpdateGamepads() {
    static double lastRefresh = 0.0;
    double now = SDL_GetTicks() / 1000.0;
    
    if (now - lastRefresh > 1.0) {
        int count = 0;
        SDL_JoystickID* gamepadIds = SDL_GetGamepads(&count);
        if (gamepadIds) {
            for (int i = 0; i < count && i < MAX_GAMEPADS; ++i) {
                if (m_gamepadIds[i] != gamepadIds[i]) {
                    if (m_openGamepads[i]) SDL_CloseGamepad(m_openGamepads[i]);
                    m_openGamepads[i] = nullptr;
                    m_gamepadIds[i] = gamepadIds[i];
                    m_gamepadStates[i].connected = true;
                }
            }
            for (int i = count; i < MAX_GAMEPADS; ++i) {
                if (m_openGamepads[i]) SDL_CloseGamepad(m_openGamepads[i]);
                m_openGamepads[i] = nullptr;
                m_gamepadIds[i] = 0;
                m_gamepadStates[i].connected = false;
            }
            SDL_free(gamepadIds);
        }
        lastRefresh = now;
    }

    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (!m_gamepadStates[i].connected) continue;

        if (!m_openGamepads[i]) {
            m_openGamepads[i] = SDL_OpenGamepad(m_gamepadIds[i]);
        }

        SDL_Gamepad* gamepad = m_openGamepads[i];
        if (!gamepad) continue;

        if (!SDL_GamepadConnected(gamepad)) {
            SDL_CloseGamepad(gamepad);
            m_openGamepads[i] = nullptr;
            m_gamepadStates[i].connected = false;
            continue;
        }

        GamepadState& state = m_gamepadStates[i];

        auto UpdateButton = [&](SDL_GamepadButton sdlBtn, GamepadButton btn) {
            int idx = static_cast<int>(btn) - 1;
            if (idx < 0 || idx >= 18) return;
            bool pressed = SDL_GetGamepadButton(gamepad, sdlBtn);
            state.buttons[idx].justPressed = pressed && !state.buttons[idx].pressed;
            state.buttons[idx].justReleased = !pressed && state.buttons[idx].pressed;
            state.buttons[idx].pressed = pressed;
        };

        UpdateButton(SDL_GAMEPAD_BUTTON_SOUTH, GamepadButton::A);
        UpdateButton(SDL_GAMEPAD_BUTTON_EAST, GamepadButton::B);
        UpdateButton(SDL_GAMEPAD_BUTTON_WEST, GamepadButton::X);
        UpdateButton(SDL_GAMEPAD_BUTTON_NORTH, GamepadButton::Y);
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

        state.axes[0] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
        state.axes[1] = -SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
        state.axes[2] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;
        state.axes[3] = -SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;
        state.axes[4] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
        state.axes[5] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f;
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

bool InputDriverSDL3::IsKeyDown(KeyCode key) const {
    int idx = static_cast<int>(key);
    return (idx >= 0 && idx < MAX_KEYS) ? m_keyStates[idx] : false;
}

bool InputDriverSDL3::IsKeyJustPressed(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= MAX_KEYS) return false;
    return m_keyStates[idx] && !m_prevKeyStates[idx];
}

bool InputDriverSDL3::IsKeyJustReleased(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= MAX_KEYS) return false;
    return !m_keyStates[idx] && m_prevKeyStates[idx];
}

void InputDriverSDL3::SetMousePosition(int x, int y) {
    SDL_WarpMouseInWindow(nullptr, static_cast<float>(x), static_cast<float>(y));
}

void InputDriverSDL3::StartTextInput() {
    SDL_StartTextInput(nullptr);
    m_textInputEnabled = true;
}

void InputDriverSDL3::StopTextInput() {
    SDL_StopTextInput(nullptr);
    m_textInputEnabled = false;
}

uint32_t InputDriverSDL3::GetGamepadCount() const {
    uint32_t count = 0;
    for (int i = 0; i < MAX_GAMEPADS; ++i) if (m_gamepadStates[i].connected) count++;
    return count;
}

bool InputDriverSDL3::IsGamepadConnected(uint32_t index) const {
    if (index >= MAX_GAMEPADS) return false;
    return m_gamepadStates[index].connected;
}

const GamepadState& InputDriverSDL3::GetGamepadState(uint32_t index) const {
    if (index >= MAX_GAMEPADS) { static const GamepadState empty{}; return empty; }
    return m_gamepadStates[index];
}

void InputDriverSDL3::SetVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) {
    if (index >= MAX_GAMEPADS || m_openGamepads[index] == nullptr) return;
    SDL_RumbleGamepad(m_openGamepads[index], static_cast<uint16_t>(leftMotor * 65535.0f), static_cast<uint16_t>(rightMotor * 65535.0f), duration);
}

} // namespace Prisma::Input
#endif
