#pragma once

#include "Application.h"
#include "scripting/ScriptEngine.h"
#include <memory>
#include <vector>

namespace Prisma {

class Template2DApp : public Application {
public:
    Template2DApp();
    ~Template2DApp() override = default;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    bool m_autoQuit = false;
};

} // namespace Prisma
