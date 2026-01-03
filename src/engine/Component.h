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
    void SetOwner(GameObject* gameObject) { this->owner = gameObject; }
    void Owner(GameObject* gameObject){
        SetOwner(gameObject);
    }
    [[nodiscard]] GameObject* GetOwner() const { return owner; }

    GameObject* owner = nullptr;
};
} // namespace Engine