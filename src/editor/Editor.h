#pragma once
#include "Export.h"
#include "IApplication.h"
#include "Singleton.h"
#include "Platform.h"  // 包含WindowHandle定义
#include "ProjectSettingsWindow.h"

class Editor : public PrismaEngine::IApplication<Editor>
{
public:
    friend class IApplication;
    Editor();
	~Editor() override;
	bool Initialize() override;
	int Run() override;
	void Shutdown() override;

private:
	bool InitializeImGui();
    void DrawMainMenu();

	// 窗口句柄
	WindowHandle m_window = nullptr;

    // Editor Windows
    ProjectSettingsWindow m_projectSettingsWindow;
    bool m_showProjectSettings = false;
};

extern "C" {
    // 导出其他函数供动态加载使用
    ENGINE_API bool Initialize();
    ENGINE_API int Run();
    ENGINE_API void Shutdown();
    ENGINE_API void Update();
}