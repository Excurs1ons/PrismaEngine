#pragma once
#include "Export.h"
#include "IApplication.h"
#include "Singleton.h"
#include "Platform.h"  // 包含WindowHandle定义
#include "ProjectSettingsWindow.h"

// Editor 也是一个动态库，需要导出符号
#if defined(_WIN32) || defined(_MSC_VER)
    #ifdef EDITOR_EXPORTS
        #define EDITOR_API __declspec(dllexport)
    #else
        #define EDITOR_API __declspec(dllimport)
    #endif
// Linux/Unix 平台使用 GCC/Clang 属性
#elif defined(__GNUC__) || defined(__clang__)
    #ifdef EDITOR_EXPORTS
        #define EDITOR_API __attribute__((visibility("default")))
    #else
        #define EDITOR_API
    #endif
#else
    #define EDITOR_API
#endif

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
    EDITOR_API bool Initialize();
    EDITOR_API int Run();
    EDITOR_API void Shutdown();
    EDITOR_API void Update();
}