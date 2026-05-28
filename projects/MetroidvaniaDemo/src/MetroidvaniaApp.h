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
    void SetSimInput(bool enable) { m_simInput = enable; }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    bool m_autoQuit = false;
    bool m_simInput = true;  // 测试模式：自动移动角色 + 相机跟随
    int m_simFrame = 0;
    float m_elapsedTime = 0;
    float m_autoExitTimeout = 30.0f; // 30秒自动退出
};

} // namespace Prisma
