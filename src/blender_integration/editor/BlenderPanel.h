#pragma once

#include "BlenderSceneImporter.h"
#include "ParallelTaskSystem.h"

namespace Prisma {
namespace Editor {

// Blender集成面板UI
class BlenderIntegrationPanel {
public:
    BlenderIntegrationPanel();
    ~BlenderIntegrationPanel();

    void Render();

private:
    void RenderConnectionSection();
    void RenderImportOptions();
    void RenderRealtimeSync();
    void RenderTaskStatus();
    void RenderStats();

private:
    std::shared_ptr<BlenderSceneImporter> importer_;
    std::shared_ptr<ParallelTaskSystem> task_system_;

    // UI状态
    bool show_window_ = true;
    std::string status_message_ = "Ready";
    float progress_ = 0.0f;
};

// 在现有编辑器中调用的渲染函数
inline void RenderBlenderIntegrationUI() {
    static BlenderIntegrationPanel panel;
    panel.Render();
}

} // namespace Editor
} // namespace Prisma