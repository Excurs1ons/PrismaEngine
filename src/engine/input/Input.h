#pragma once

#include "input/InputManager.h"
#include "app/Engine.h"

namespace Prisma {

/**
 * @brief 静态输入轮询类 (Cherno Style)
 * 简化了在 Update 逻辑中对按键和鼠标状态的查询。
 */
class ENGINE_API Input {
public:
    static bool IsKeyPressed(Prisma::Input::KeyCode key) {
        auto* im = Engine::Get().GetInputManager();
        return im ? im->IsKeyPressed(key) : false;
    }

    static bool IsMouseButtonPressed(Input::MouseButton button) {
        auto* im = Engine::Get().GetInputManager();
        return im ? im->IsMouseButtonPressed(button) : false;
    }

    static PrismaMath::vec2 GetMousePosition() {
        auto* im = Engine::Get().GetInputManager();
        return im ? im->GetMousePosition() : PrismaMath::vec2(0.0f);
    }

    static float GetMouseX() {
        return GetMousePosition().x;
    }

    static float GetMouseY() {
        return GetMousePosition().y;
    }
};

} // namespace Prisma
