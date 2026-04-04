#pragma once

#include "Export.h"
#include "core/Timestep.h"
#include "math/MathTypes.h"
#include <string>

namespace Prisma {

class GameObject;

class ENGINE_API Component {
public:
    virtual ~Component() = default;
    virtual void Initialize(){};
    virtual void Update([[maybe_unused]] Timestep ts) {}
    virtual void Shutdown(){};
    
    void SetOwner(GameObject* gameObject) { this->owner = gameObject; }
    void Owner(GameObject* gameObject) {
        SetOwner(gameObject);
    }
    [[nodiscard]] GameObject* GetOwner() const { return owner; }

protected:
    GameObject* owner = nullptr;
};

} // namespace Prisma
