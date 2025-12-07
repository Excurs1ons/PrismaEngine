#pragma once
#include "RenderBackend.h"
#include "Export.h"
#include "IApplication.h"
#include "Singleton.h"

class ApplicationEditor : public Engine::IApplication<ApplicationEditor>
{
public:
    friend class Engine::IApplication<ApplicationEditor>;
    ApplicationEditor();
	~ApplicationEditor() override;
	bool Initialize() override;
	int Run() override;
    void Update();
	void Shutdown() override;
};

extern "C" {
    // 导出其他函数供动态加载使用
    __declspec(dllexport) bool InitializeEditor();
    __declspec(dllexport) int RunEditor();
    __declspec(dllexport) void ShutdownEditor();
    __declspec(dllexport) void UpdateEditor();
    }