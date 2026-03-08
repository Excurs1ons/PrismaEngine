#pragma once

#include "Export.h"
#include "IApplication.h"
#include "Logger.h"
#include "Platform.h"
#include "ProjectSettingsWindow.h"
#include "Singleton.h"
#include "ManagerBase.h"

namespace PrismaEngine {

#pragma warning(push)
#pragma warning(disable: 4251 4275)
class EDITOR_API Editor : public IApplication<Editor>, public ManagerBase<Editor> {
public:
    Editor();
    ~Editor() override;

    static std::shared_ptr<Editor> GetInstance();

    int Initialize() override;
    int Run() override;
    void Shutdown() override;

    bool InitializeImGui();

private:
    void DrawMainMenu();

    // 窗口句柄
    WindowHandle m_window = nullptr;

    // Editor Windows
    ProjectSettingsWindow m_projectSettingsWindow;
    bool m_showProjectSettings = false;
};
#pragma warning(pop)

} // namespace PrismaEngine

extern "C" {
    /// @brief 编辑器统一入口点
    EDITOR_API int PrismaEditor_Main(int argc, char** argv, Logger* externalLogger);
}
