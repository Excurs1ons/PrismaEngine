#pragma once

#include "Export.h"
#include "Application.h"
#include "Logger.h"
#include "Platform.h"
#include "ProjectSettingsWindow.h"
#include "Singleton.h"
#include "ManagerBase.h"

// 显式包含 SDL3
#include <SDL3/SDL.h>

namespace Prisma {

class EDITOR_API Editor : public Application {
public:
    Editor();
    ~Editor() override;

    // 被动初始化接口，仅负责应用层自身的逻辑
    int OnInitialize() override;
    void OnShutdown() override;

    void OnUpdate(Timestep ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

    int OnImGuiInitialize() override;

    void* GetImGuiContext() override;
    
    void OpenProjectSettings() { m_showProjectSettings = true; }

private:
    // Editor Windows
    ProjectSettingsWindow m_projectSettingsWindow;
    bool m_showProjectSettings = false;
};

} // namespace Prisma
