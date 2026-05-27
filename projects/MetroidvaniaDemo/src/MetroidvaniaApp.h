#pragma once

#include "app/Application.h"
#include "scripting/ScriptEngine.h"
#include <memory>

namespace Prisma {

class MetroidvaniaApp : public Application {
public:
    MetroidvaniaApp();
    ~MetroidvaniaApp() override = default;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    bool m_autoQuit = false;
};

} // namespace Prisma
