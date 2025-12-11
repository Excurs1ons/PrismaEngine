#pragma once
#include "KeyCode.h"
namespace Engine {
    namespace Input {

        enum InputBackendType {
            Win32,
            SDL3,
            DirectInput
        };

        class IInputBackend {
        public:
            virtual bool GetKeyDown(KeyCode key) { return false; }
            virtual bool GetKeyUp(KeyCode key) { return false; }
            virtual bool GetPointerDown(MouseButton button) { return false; }
            virtual bool GetPointerUp(MouseButton button) { return false; }
        };
    }
}
