#pragma once
#include <string>
namespace PrismaEngine {
    namespace Input {

        using MouseButton = int;
        enum KeyCode {
            // 字母键
            A = 0,
            B,
            C,
            D,
            E,
            F,
            G,
            H,
            I,
            J,
            K,
            L,
            M,
            N,
            O,
            P,
            Q,
            R,
            S,
            T,
            U,
            V,
            W,
            X,
            Y,
            Z,

            // 数字键
            Num0,
            Num1,
            Num2,
            Num3,
            Num4,
            Num5,
            Num6,
            Num7,
            Num8,
            Num9,

            // 功能键
            F1,
            F2,
            F3,
            F4,
            F5,
            F6,
            F7,
            F8,
            F9,
            F10,
            F11,
            F12,

            // 方向键
            ArrowUp,
            ArrowDown,
            ArrowLeft,
            ArrowRight,

            // 特殊键
            Space,
            Enter,
            Escape,
            Backspace,
            Tab,
            CapsLock,

            // 修饰键
            LeftShift,
            RightShift,
            LeftControl,
            RightControl,
            LeftAlt,
            RightAlt,
            LeftSuper,  // Windows键/Command键
            RightSuper,

            // 符号键
            Grave,      // ` ~
            Minus,      // - _
            Equal,      // = +
            LeftBracket,  // [ {
            RightBracket, // ] }
            Backslash,    // \ |
            Semicolon,    // ; :
            Apostrophe,   // ' "
            Comma,        // , <
            Period,       // . >
            Slash,        // / ?

            // 小键盘
            Keypad0,
            Keypad1,
            Keypad2,
            Keypad3,
            Keypad4,
            Keypad5,
            Keypad6,
            Keypad7,
            Keypad8,
            Keypad9,
            KeypadDecimal,
            KeypadDivide,
            KeypadMultiply,
            KeypadSubtract,
            KeypadAdd,
            KeypadEnter,
            KeypadEqual,

            // 其他
            Insert,
            Delete,
            Home,
            End,
            PageUp,
            PageDown,
            PrintScreen,
            ScrollLock,
            Pause,

            // 鼠标按钮（可选）
            MouseLeft,
            MouseRight,
            MouseMiddle,
            MouseButton4,
            MouseButton5,

            // 控制器按钮（可选）
            GamepadA,
            GamepadB,
            GamepadX,
            GamepadY,
            GamepadStart,
            GamepadBack,
            GamepadLeftShoulder,
            GamepadRightShoulder,
            GamepadDPadUp,
            GamepadDPadDown,
            GamepadDPadLeft,
            GamepadDPadRight,

            Count,
            Unknown = Count
        };
        inline const char* KeyCodeToCString(KeyCode key)
        {
            switch (key)
            {
                // 字母键
            case KeyCode::A: return "A";
            case KeyCode::B: return "B";
            case KeyCode::C: return "C";
            case KeyCode::D: return "D";
            case KeyCode::E: return "E";
            case KeyCode::F: return "F";
            case KeyCode::G: return "G";
            case KeyCode::H: return "H";
            case KeyCode::I: return "I";
            case KeyCode::J: return "J";
            case KeyCode::K: return "K";
            case KeyCode::L: return "L";
            case KeyCode::M: return "M";
            case KeyCode::N: return "N";
            case KeyCode::O: return "O";
            case KeyCode::P: return "P";
            case KeyCode::Q: return "Q";
            case KeyCode::R: return "R";
            case KeyCode::S: return "S";
            case KeyCode::T: return "T";
            case KeyCode::U: return "U";
            case KeyCode::V: return "V";
            case KeyCode::W: return "W";
            case KeyCode::X: return "X";
            case KeyCode::Y: return "Y";
            case KeyCode::Z: return "Z";

                // 数字键
            case KeyCode::Num0: return "0";
            case KeyCode::Num1: return "1";
            case KeyCode::Num2: return "2";
            case KeyCode::Num3: return "3";
            case KeyCode::Num4: return "4";
            case KeyCode::Num5: return "5";
            case KeyCode::Num6: return "6";
            case KeyCode::Num7: return "7";
            case KeyCode::Num8: return "8";
            case KeyCode::Num9: return "9";

                // 功能键
            case KeyCode::F1: return "F1";
            case KeyCode::F2: return "F2";
            case KeyCode::F3: return "F3";
            case KeyCode::F4: return "F4";
            case KeyCode::F5: return "F5";
            case KeyCode::F6: return "F6";
            case KeyCode::F7: return "F7";
            case KeyCode::F8: return "F8";
            case KeyCode::F9: return "F9";
            case KeyCode::F10: return "F10";
            case KeyCode::F11: return "F11";
            case KeyCode::F12: return "F12";

                // 方向键
            case KeyCode::ArrowUp: return "Up";
            case KeyCode::ArrowDown: return "Down";
            case KeyCode::ArrowLeft: return "Left";
            case KeyCode::ArrowRight: return "Right";

                // 特殊键
            case KeyCode::Space: return "Space";
            case KeyCode::Enter: return "Enter";
            case KeyCode::Escape: return "Escape";
            case KeyCode::Backspace: return "Backspace";
            case KeyCode::Tab: return "Tab";
            case KeyCode::CapsLock: return "CapsLock";

                // 修饰键
            case KeyCode::LeftShift: return "Left Shift";
            case KeyCode::RightShift: return "Right Shift";
            case KeyCode::LeftControl: return "Left Ctrl";
            case KeyCode::RightControl: return "Right Ctrl";
            case KeyCode::LeftAlt: return "Left Alt";
            case KeyCode::RightAlt: return "Right Alt";
            case KeyCode::LeftSuper: return "Left Super";
            case KeyCode::RightSuper: return "Right Super";

                // 符号键
            case KeyCode::Grave: return "`";
            case KeyCode::Minus: return "-";
            case KeyCode::Equal: return "=";
            case KeyCode::LeftBracket: return "[";
            case KeyCode::RightBracket: return "]";
            case KeyCode::Backslash: return "\\";
            case KeyCode::Semicolon: return ";";
            case KeyCode::Apostrophe: return "'";
            case KeyCode::Comma: return ",";
            case KeyCode::Period: return ".";
            case KeyCode::Slash: return "/";

                // 小键盘
            case KeyCode::Keypad0: return "Keypad 0";
            case KeyCode::Keypad1: return "Keypad 1";
            case KeyCode::Keypad2: return "Keypad 2";
            case KeyCode::Keypad3: return "Keypad 3";
            case KeyCode::Keypad4: return "Keypad 4";
            case KeyCode::Keypad5: return "Keypad 5";
            case KeyCode::Keypad6: return "Keypad 6";
            case KeyCode::Keypad7: return "Keypad 7";
            case KeyCode::Keypad8: return "Keypad 8";
            case KeyCode::Keypad9: return "Keypad 9";
            case KeyCode::KeypadDecimal: return "Keypad .";
            case KeyCode::KeypadDivide: return "Keypad /";
            case KeyCode::KeypadMultiply: return "Keypad *";
            case KeyCode::KeypadSubtract: return "Keypad -";
            case KeyCode::KeypadAdd: return "Keypad +";
            case KeyCode::KeypadEnter: return "Keypad Enter";
            case KeyCode::KeypadEqual: return "Keypad =";

                // 其他
            case KeyCode::Insert: return "Insert";
            case KeyCode::Delete: return "Delete";
            case KeyCode::Home: return "Home";
            case KeyCode::End: return "End";
            case KeyCode::PageUp: return "Page Up";
            case KeyCode::PageDown: return "Page Down";
            case KeyCode::PrintScreen: return "Print Screen";
            case KeyCode::ScrollLock: return "Scroll Lock";
            case KeyCode::Pause: return "Pause";

                // 鼠标按钮
            case KeyCode::MouseLeft: return "Mouse Left";
            case KeyCode::MouseRight: return "Mouse Right";
            case KeyCode::MouseMiddle: return "Mouse Middle";
            case KeyCode::MouseButton4: return "Mouse Button 4";
            case KeyCode::MouseButton5: return "Mouse Button 5";

                // 控制器按钮
            case KeyCode::GamepadA: return "Gamepad A";
            case KeyCode::GamepadB: return "Gamepad B";
            case KeyCode::GamepadX: return "Gamepad X";
            case KeyCode::GamepadY: return "Gamepad Y";
            case KeyCode::GamepadStart: return "Gamepad Start";
            case KeyCode::GamepadBack: return "Gamepad Back";
            case KeyCode::GamepadLeftShoulder: return "Left Shoulder";
            case KeyCode::GamepadRightShoulder: return "Right Shoulder";
            case KeyCode::GamepadDPadUp: return "DPad Up";
            case KeyCode::GamepadDPadDown: return "DPad Down";
            case KeyCode::GamepadDPadLeft: return "DPad Left";
            case KeyCode::GamepadDPadRight: return "DPad Right";

            default: return "Unknown";
            }
        }

        // 返回 std::string 版本
        inline std::string KeyCodeToString(KeyCode key)
        {
            return KeyCodeToCString(key);
        }

    }
}
