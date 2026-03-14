#pragma once

#include "input/InputManager.h"

namespace Prisma {

/**
 * @brief 静态输入轮询类 (Cherno Style)
 * 简化了在 Update 逻辑中对按键和鼠标状态的查询。
 */
class ENGINE_API Input {
public:
    static bool IsKeyPressed(Input::KeyCode key) {
        return Input::InputManager::Get()->IsKeyPressed(key);
    }

    static bool IsMouseButtonPressed(Input::MouseButton button) {
        return Input::InputManager::Get()->IsMouseButtonPressed(button);
    }

    static PrismaMath::vec2 GetMousePosition() {
        return Input::InputManager::Get()->GetMousePosition();
    }

    static float GetMouseX() {
        return GetMousePosition().x;
    }

    static float GetMouseY() {
        return GetMousePosition().y;
    }
};

} // namespace Prisma
