#pragma once

#include "app/Application.h"

namespace Prisma {

/**
 NeoEditor Plugin Application — 由 Launcher 加载
 *
 * 插件模式 = 无头控制台模式，通过 MCP/CLI 交互。
 * 不创建窗口，不启动渲染管线。
 */
class NeoEditorPlugin : public Application {
public:
    NeoEditorPlugin();
    ~NeoEditorPlugin() override;

    int OnInitialize() override;
    void OnShutdown() override;
    void OnUpdate(Timestep ts) override;
    void OnRender() override;

    bool IsHeadless() const { return true; }

private:
    bool m_initialized = false;
};

}
