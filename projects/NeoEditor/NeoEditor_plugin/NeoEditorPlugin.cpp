#include "NeoEditorPlugin.h"
#include "app/Engine.h"
#include "Logger.h"

namespace Prisma {

NeoEditorPlugin::NeoEditorPlugin() = default;

NeoEditorPlugin::~NeoEditorPlugin() = default;

int NeoEditorPlugin::OnInitialize() {
    LOG_INFO("NeoEditor", "NeoEditor 插件模式初始化（无头）");

    auto& engine = Engine::Get();

    // Engine 已在 Headless 模式下启动（由 Launcher 或 EngineThread.cs 配置）
    // 插件模式不创建窗口，不初始化渲染管线

    m_initialized = true;
    LOG_INFO("NeoEditor", "NeoEditor 插件就绪，等待 MCP/CLI 指令");
    return 0;
}

void NeoEditorPlugin::OnUpdate([[maybe_unused]] Timestep ts) {
}

void NeoEditorPlugin::OnRender() {
}

void NeoEditorPlugin::OnShutdown() {
    LOG_INFO("NeoEditor", "NeoEditor 插件关闭");
    m_initialized = false;
}

}
