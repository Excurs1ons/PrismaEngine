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
    bool m_simInput = true;  // 默认开启模拟输入（自动化测试）
    int m_simFrame = 0;
    float m_elapsedTime = 0;
    float m_autoExitTimeout = 15.0f; // 15秒自动退出
};

} // namespace Prisma
