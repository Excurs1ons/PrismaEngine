#pragma once

#include "app/Application.h"
#include "scripting/ScriptEngine.h"
#include <memory>
#include <vector>

namespace Prisma {

class Prisma2DApp : public Application {
public:
    Prisma2DApp();
    ~Prisma2DApp() override = default;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    bool m_autoQuit = false;
};

} // namespace Prisma
