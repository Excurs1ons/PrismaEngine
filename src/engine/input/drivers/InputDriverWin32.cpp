#if defined(_WIN32) || (defined(PRISMA_ENABLE_INPUT_RAWINPUT) || defined(PRISMA_ENABLE_INPUT_XINPUT))
#include "InputDriverWin32.h"
#include <algorithm>

namespace PrismaEngine::Input {

// ========== InputDriverWin32 ==========

InputDriverWin32::InputDriverWin32() {
    m_mouseState.buttons[0] = {false, false, false};
    for (int i = 1; i < MAX_BUTTONS; ++i) {
        m_mouseState.buttons[i] = {false, false, false};
    }

    for (auto& state : m_gamepadStates) {
        state.connected = false;
        std::fill_n(state.axes, 6, 0.0f);
    }
}

InputDriverWin32::~InputDriverWin32() {
    Shutdown();
}

bool InputDriverWin32::Initialize() {
    if (m_initialized) {
        return true;
    }

    // 鑾峰彇涓荤獥鍙ｅ彞鏌勶紙鍋囪鏄帶鍒跺彴鎴栨闈㈢獥鍙ｏ級
    m_hwnd = GetActiveWindow();
    if (!m_hwnd) {
        m_hwnd = GetDesktopWindow();
    }

    if (!RegisterRawInputDevices()) {
        return false;
    }

    m_initialized = true;
    return true;
}

void InputDriverWin32::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // 鍋滄鎵€鏈夋尟鍔?
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed = 0;
        vibration.wRightMotorSpeed = 0;
        XInputSetState(i, &vibration);
    }

    m_initialized = false;
}

void InputDriverWin32::Update() {
    if (!m_initialized) {
        return;
    }

    // 淇濆瓨涓婁竴甯х姸鎬?
    m_prevKeyStates = m_keyStates;

    // 澶勭悊 Windows 娑堟伅闃熷垪
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_INPUT) {
            HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(msg.lParam);
            UINT size = 0;
            GetRawInputData(hRawInput, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

            if (size > 0) {
                std::vector<BYTE> buffer(size);
                if (GetRawInputData(hRawInput, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == size) {
                    ProcessRawInput(reinterpret_cast<const RAWINPUT*>(buffer.data()));
                }
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 鏇存柊鎵嬫焺
    UpdateXInput();

    // 鏇存柊榧犳爣婊氳疆
    m_mouseState.wheelDelta = static_cast<float>(m_mouseWheelAccumulator) / WHEEL_DELTA;
    m_mouseWheelAccumulator = 0;

    // 閲嶇疆鐩稿绉诲姩
    m_mouseState.deltaX = 0;
    m_mouseState.deltaY = 0;

    // 娓呯┖鏂囨湰杈撳叆
    m_textInput.clear();
}

BOOL InputDriverWin32::RegisterRawInputDevices(std::array<tagRAWINPUTDEVICE, 2>::pointer data, UINT count, size_t size) {
    return ::RegisterRawInputDevices(reinterpret_cast<PRAWINPUTDEVICE>(data), count, static_cast<UINT>(size));
}
bool InputDriverWin32::RegisterRawInputDevices() {
    // 娉ㄥ唽 RawInput 璁惧
    std::array<RAWINPUTDEVICE, 2> devices{};

    // 閿洏
    devices[0].usUsagePage = 0x01;
    devices[0].usUsage = 0x06;
    devices[0].dwFlags = RIDEV_INPUTSINK;
    devices[0].hwndTarget = m_hwnd;

    // 榧犳爣
    devices[1].usUsagePage = 0x01;
    devices[1].usUsage = 0x02;
    devices[1].dwFlags = RIDEV_INPUTSINK;
    devices[1].hwndTarget = m_hwnd;

    return RegisterRawInputDevices(devices.data(), static_cast<UINT>(devices.size()), sizeof(RAWINPUTDEVICE)) != FALSE;
}

void InputDriverWin32::ProcessRawInput(const RAWINPUT* raw) {
    if (!raw) {
        return;
    }

    switch (raw->header.dwType) {
        case RIM_TYPEKEYBOARD: {
            const RAWKEYBOARD& kb = raw->data.keyboard;
            UINT virtualKey = kb.VKey;
            bool down = (kb.Flags & RI_KEY_BREAK) == 0;

            // 澶勭悊鎸夐敭
            KeyCode key = MapVirtualKeyToKeyCode(virtualKey);
            if (key != KeyCode::Unknown) {
                m_keyStates[static_cast<int>(key)] = down;
            }

            // 鏂囨湰杈撳叆
            if (down && m_textInputEnabled) {
                // 绠€鍖栵細浠呭鐞?ASCII 瀛楃
                if (virtualKey >= 0x30 && virtualKey <= 0x5A) {
                    char ch = static_cast<char>(MapVirtualKey(virtualKey, MAPVK_VK_TO_CHAR));
                    if (ch != 0) {
                        m_textInput += ch;
                    }
                }
            }
            break;
        }

        case RIM_TYPEMOUSE: {
            const RAWMOUSE& mouse = raw->data.mouse;

            // 鐩稿绉诲姩
            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                // 缁濆浣嶇疆
                POINT pt = {mouse.lLastX, mouse.lLastY};
                m_mouseState.x = pt.x;
                m_mouseState.y = pt.y;
            } else {
                // 鐩稿绉诲姩
                m_mouseState.deltaX += mouse.lLastX;
                m_mouseState.deltaY += mouse.lLastY;
                m_mouseState.x += mouse.lLastX;
                m_mouseState.y += mouse.lLastY;
            }

            // 婊氳疆
            if (mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                m_mouseWheelAccumulator += static_cast<short>(mouse.usButtonData);
            }

            // 鎸夐挳鐘舵€?
            auto UpdateButton = [&](USHORT flags, MouseButton btn) {
                int idx = static_cast<int>(btn) - 1;
                bool pressed = (mouse.usButtonFlags & flags) != 0;
                bool released = (mouse.usButtonFlags & (flags << 1)) != 0;

                m_mouseState.buttons[idx].pressed = pressed;

                // 妫€娴嬬姸鎬佸彉鍖?
                if (pressed && !m_mouseState.buttons[idx].pressed) {
                    m_mouseState.buttons[idx].justPressed = true;
                } else {
                    m_mouseState.buttons[idx].justPressed = false;
                }

                if (released) {
                    m_mouseState.buttons[idx].justReleased = true;
                } else {
                    m_mouseState.buttons[idx].justReleased = false;
                }
            };

            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) UpdateButton(RI_MOUSE_BUTTON_1_DOWN, MouseButton::Left);
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) UpdateButton(RI_MOUSE_BUTTON_2_DOWN, MouseButton::Right);
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) UpdateButton(RI_MOUSE_BUTTON_3_DOWN, MouseButton::Middle);
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) UpdateButton(RI_MOUSE_BUTTON_4_DOWN, MouseButton::X1);
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) UpdateButton(RI_MOUSE_BUTTON_5_DOWN, MouseButton::X2);

            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) m_mouseState.buttons[0].justReleased = true;
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) m_mouseState.buttons[1].justReleased = true;
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP) m_mouseState.buttons[2].justReleased = true;
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) m_mouseState.buttons[3].justReleased = true;
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) m_mouseState.buttons[4].justReleased = true;
            break;
        }
    }
}

void InputDriverWin32::UpdateXInput() {
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
        XINPUT_STATE state;
        DWORD result = XInputGetState(i, &state);

        GamepadState& gs = m_gamepadStates[i];
        gs.connected = (result == ERROR_SUCCESS);

        if (!gs.connected) {
            continue;
        }

        // 鎸夐挳
        auto SetButton = [&](WORD mask, GamepadButton btn) {
            int idx = static_cast<int>(btn) - 1;
            bool pressed = (state.Gamepad.wButtons & mask) != 0;
            gs.buttons[idx].pressed = pressed;
            gs.buttons[idx].justPressed = pressed && !gs.buttons[idx].pressed;
            gs.buttons[idx].justReleased = !pressed && gs.buttons[idx].pressed;
        };

        SetButton(XINPUT_GAMEPAD_A, GamepadButton::A);
        SetButton(XINPUT_GAMEPAD_B, GamepadButton::B);
        SetButton(XINPUT_GAMEPAD_X, GamepadButton::X);
        SetButton(XINPUT_GAMEPAD_Y, GamepadButton::Y);
        SetButton(XINPUT_GAMEPAD_LEFT_SHOULDER, GamepadButton::LeftShoulder);
        SetButton(XINPUT_GAMEPAD_RIGHT_SHOULDER, GamepadButton::RightShoulder);
        SetButton(XINPUT_GAMEPAD_BACK, GamepadButton::Back);
        SetButton(XINPUT_GAMEPAD_START, GamepadButton::Start);
        SetButton(XINPUT_GAMEPAD_LEFT_THUMB, GamepadButton::LeftStick);
        SetButton(XINPUT_GAMEPAD_RIGHT_THUMB, GamepadButton::RightStick);
        SetButton(XINPUT_GAMEPAD_DPAD_UP, GamepadButton::DPadUp);
        SetButton(XINPUT_GAMEPAD_DPAD_DOWN, GamepadButton::DPadDown);
        SetButton(XINPUT_GAMEPAD_DPAD_LEFT, GamepadButton::DPadLeft);
        SetButton(XINPUT_GAMEPAD_DPAD_RIGHT, GamepadButton::DPadRight);

        // 鎵虫満
        gs.axes[4] = static_cast<float>(state.Gamepad.bLeftTrigger) / 255.0f;
        gs.axes[5] = static_cast<float>(state.Gamepad.bRightTrigger) / 255.0f;

        // 鎽囨潌
        gs.axes[0] = (std::abs(state.Gamepad.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) ?
            static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f : 0.0f;
        gs.axes[1] = (std::abs(state.Gamepad.sThumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) ?
            static_cast<float>(-state.Gamepad.sThumbLY) / 32767.0f : 0.0f;
        gs.axes[2] = (std::abs(state.Gamepad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) ?
            static_cast<float>(state.Gamepad.sThumbRX) / 32767.0f : 0.0f;
        gs.axes[3] = (std::abs(state.Gamepad.sThumbRY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) ?
            static_cast<float>(-state.Gamepad.sThumbRY) / 32767.0f : 0.0f;
    }
}

KeyCode InputDriverWin32::MapVirtualKeyToKeyCode(UINT virtualKey) const {
    // 绠€鍖栨槧灏勶紝瀹為檯搴旇鏇村畬鏁?
    static const std::unordered_map<UINT, KeyCode> keyMap = {
        {VK_RETURN, KeyCode::Enter},
        {VK_ESCAPE, KeyCode::Escape},
        {VK_BACK, KeyCode::Backspace},
        {VK_TAB, KeyCode::Tab},
        {VK_SPACE, KeyCode::Space},
        {VK_SHIFT, KeyCode::Shift},
        {VK_CONTROL, KeyCode::Ctrl},
        {VK_MENU, KeyCode::Alt},
        {VK_CAPITAL, KeyCode::CapsLock},
        {VK_PRIOR, KeyCode::PageUp},
        {VK_NEXT, KeyCode::PageDown},
        {VK_END, KeyCode::End},
        {VK_HOME, KeyCode::Home},
        {VK_LEFT, KeyCode::Left},
        {VK_UP, KeyCode::Up},
        {VK_RIGHT, KeyCode::Right},
        {VK_DOWN, KeyCode::Down},
        {VK_SNAPSHOT, KeyCode::PrintScreen},
        {VK_INSERT, KeyCode::Insert},
        {VK_DELETE, KeyCode::Delete},
        {0x30, KeyCode::Num0}, {0x31, KeyCode::Num1}, {0x32, KeyCode::Num2},
        {0x33, KeyCode::Num3}, {0x34, KeyCode::Num4}, {0x35, KeyCode::Num5},
        {0x36, KeyCode::Num6}, {0x37, KeyCode::Num7}, {0x38, KeyCode::Num8},
        {0x39, KeyCode::Num9},
        {0x41, KeyCode::A}, {0x42, KeyCode::B}, {0x43, KeyCode::C},
        {0x44, KeyCode::D}, {0x45, KeyCode::E}, {0x46, KeyCode::F},
        {0x47, KeyCode::G}, {0x48, KeyCode::H}, {0x49, KeyCode::I},
        {0x4A, KeyCode::J}, {0x4B, KeyCode::K}, {0x4C, KeyCode::L},
        {0x4D, KeyCode::M}, {0x4E, KeyCode::N}, {0x4F, KeyCode::O},
        {0x50, KeyCode::P}, {0x51, KeyCode::Q}, {0x52, KeyCode::R},
        {0x53, KeyCode::S}, {0x54, KeyCode::T}, {0x55, KeyCode::U},
        {0x56, KeyCode::V}, {0x57, KeyCode::W}, {0x58, KeyCode::X},
        {0x59, KeyCode::Y}, {0x5A, KeyCode::Z},
        {VK_F1, KeyCode::F1}, {VK_F2, KeyCode::F2}, {VK_F3, KeyCode::F3},
        {VK_F4, KeyCode::F4}, {VK_F5, KeyCode::F5}, {VK_F6, KeyCode::F6},
        {VK_F7, KeyCode::F7}, {VK_F8, KeyCode::F8}, {VK_F9, KeyCode::F9},
        {VK_F10, KeyCode::F10}, {VK_F11, KeyCode::F11}, {VK_F12, KeyCode::F12},
    };

    auto it = keyMap.find(virtualKey);
    return (it != keyMap.end()) ? it->second : KeyCode::Unknown;
}

// ========== 閿洏鏌ヨ ==========

bool InputDriverWin32::IsKeyDown(KeyCode key) const {
    int idx = static_cast<int>(key);
    return (idx >= 0 && idx < MAX_KEYS) ? m_keyStates[idx] : false;
}

bool InputDriverWin32::IsKeyJustPressed(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= MAX_KEYS) {
        return false;
    }
    return m_keyStates[idx] && !m_prevKeyStates[idx];
}

bool InputDriverWin32::IsKeyJustReleased(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= MAX_KEYS) {
        return false;
    }
    return !m_keyStates[idx] && m_prevKeyStates[idx];
}

// ========== 榧犳爣 ==========

void InputDriverWin32::SetMousePosition(int x, int y) {
    m_mouseState.x = x;
    m_mouseState.y = y;
    SetCursorPos(x, y);
}

// ========== 鎵嬫焺鏌ヨ ==========

bool InputDriverWin32::IsGamepadConnected(uint32_t index) const {
    if (index >= XUSER_MAX_COUNT) {
        return false;
    }
    return m_gamepadStates[index].connected;
}

const GamepadState& InputDriverWin32::GetGamepadState(uint32_t index) const {
    if (index >= XUSER_MAX_COUNT) {
        static const GamepadState empty{};
        return empty;
    }
    return m_gamepadStates[index];
}

void InputDriverWin32::SetVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) {
    if (index >= XUSER_MAX_COUNT) {
        return;
    }

    XINPUT_VIBRATION vibration{};
    vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);

    XInputSetState(index, &vibration);

    // 绠€鍖栵細杩欓噷搴旇娣诲姞瀹氭椂鍣ㄥ湪鎸佺画鏃堕棿鍚庡仠姝㈡尟鍔?
}

} // namespace PrismaEngine::Input
#endif

