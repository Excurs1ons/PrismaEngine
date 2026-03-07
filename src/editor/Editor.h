#pragma once

#include "Export.h"
#include "IApplication.h"
#include "Logger.h"
#include "Platform.h"  // 包含WindowHandle定义
#include "ProjectSettingsWindow.h"
#include "Singleton.h"
#include <d3d12.h>
#include <wrl/client.h>

class EDITOR_API Editor : public PrismaEngine::IApplication<Editor> {
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

#if defined(PRISMA_ENABLE_RENDER_DX12)
    // ImGui DX12 SRV 描述符堆（用于字体纹理）
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
#endif

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