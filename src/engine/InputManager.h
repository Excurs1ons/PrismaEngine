#pragma once
#include "KeyCode.h"
#include "Platform.h"
#include "Singleton.h"

namespace Engine {

class InputManager : public Singleton<InputManager> {
public:
    friend class Singleton<InputManager>;

    bool IsKeyDown(KeyCode key) const;
    bool IsMouseButtonDown(MouseButton button) const;
    void GetMousePosition(float& x, float& y) const;

    void SetPlatform(Platform* platform);

private:
    InputManager() = default;
    Platform* m_platform = nullptr;
};

} // namespace Engine