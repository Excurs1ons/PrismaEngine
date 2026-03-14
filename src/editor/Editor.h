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

#pragma warning(push)
#pragma warning(disable: 4251 4275)
class EDITOR_API Editor : public Application {
public:
    Editor();
    ~Editor() override;

    static std::shared_ptr<Editor> Get();

    // 被动初始化接口，仅负责应用层自身的逻辑
    int OnInitialize() override;
    void OnShutdown() override;

    void OnUpdate(Timestep ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

    bool InitializeImGui();

private:
    // Editor Windows
    ProjectSettingsWindow m_projectSettingsWindow;
    bool m_showProjectSettings = false;
};

} // namespace Prisma
