#pragma once


#include "math/MathTypes.h"
#include <string>

namespace PrismaEngine {

class GameObject;

class Component {
public:
    virtual ~Component() = default;
    virtual void Initialize(){};
    virtual void Update(float deltaTime) {}
    virtual void Shutdown(){};
    void SetOwner(GameObject* owner) { this->owner = owner; }
    void Owner(GameObject* owner){
        SetOwner(owner);
    }
    [[nodiscard]] GameObject* GetOwner() const { return owner; }

protected:
    GameObject* owner = nullptr;
};
} // namespace Engine