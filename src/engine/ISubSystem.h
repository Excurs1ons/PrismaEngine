#pragma once
#include <string>

namespace PrismaEngine {

class ISubSystem {
public:
    virtual ~ISubSystem()                = default;
    virtual bool Initialize()            = 0;
    virtual void Shutdown()              = 0;
    virtual void Update(float deltaTime) {}
    // static ISubSystem& GetInstance() {
    //     static ISubSystem instance;
    //     return instance;
    // }
};

}  // namespace Engine