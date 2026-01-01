#pragma once


#include "math/MathTypes.h"
#include <string>

namespace PrismaEngine {

class GameObject;

class Component {
public:
    virtual ~Component() = default;
    virtual void Update(float deltaTime) {}

    void SetOwner(GameObject* owner) { this->owner = owner; }
    [[nodiscard]] GameObject* GetOwner() const { return owner; }

protected:
    GameObject* owner = nullptr;
};
} // namespace Engine