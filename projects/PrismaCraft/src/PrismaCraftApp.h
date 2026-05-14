#pragma once
#include "app/Application.h"
#include "scripting/ScriptEngine.h"

namespace Prisma {

class PrismaCraftApp : public Application {
public:
    PrismaCraftApp();
    ~PrismaCraftApp() override = default;

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;
};

} // namespace Prisma
