#pragma once
#include "Export.h"
#include "IApplication.h"
#include "Singleton.h"
#include "Platform.h"  // 包含WindowHandle定义


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

	// 窗口句柄
	WindowHandle m_window = nullptr;
};

extern "C" {
    // 导出其他函数供动态加载使用
    ENGINE_API bool Initialize();
    ENGINE_API int Run();
    ENGINE_API void Shutdown();
    ENGINE_API void Update();
}