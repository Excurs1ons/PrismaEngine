#pragma once

#include "app/Application.h"
#include "core/Node.h"

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
    float m_elapsedTime = 0;
    float m_autoExitTimeout = 30.0f;
};

} // namespace Prisma
