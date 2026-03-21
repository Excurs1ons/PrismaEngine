#include "BlenderPanel.h"

// ImGui includes
#include <imgui.h>
#include "../Logger.h"

namespace Prisma {
namespace Editor {

BlenderIntegrationPanel::BlenderIntegrationPanel() 
    : importer_(BlenderSceneImporter::Get())
    , task_system_(ParallelTaskSystem::Get()) {
    
    // 启动任务系统
    if (!task_system_->IsRunning()) {
        task_system_->Start();
    }
}

BlenderIntegrationPanel::~BlenderIntegrationPanel() {
    // 停止任务系统
    if (task_system_->IsRunning()) {
        task_system_->Stop();
    }
}

void BlenderIntegrationPanel::Render() {
    if (!show_window_) return;

    ImGui::Begin("Blender Integration", &show_window_);

    // 连接状态
    RenderConnectionSection();
    
    ImGui::Separator();

    // 导入选项
    RenderImportOptions();
    
    ImGui::Separator();

    // 实时同步
    RenderRealtimeSync();
    
    ImGui::Separator();

    // 任务状态
    RenderTaskStatus();
    
    ImGui::Separator();

    // 统计信息
    RenderStats();

    ImGui::End();
}

void BlenderIntegrationPanel::RenderConnectionSection() {
    ImGui::Text("Connection");
    ImGui::Indent();

    auto ipc = BlenderIPCServer::Get();
    bool connected = ipc ? ipc->IsConnected() : false;

    // 连接状态指示
    ImGui::BeginDisabled(true);
    ImGui::Selectable(connected ? "Connected" : "Not Connected", false);
    ImGui::EndDisabled();

    if (connected) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "✓");
    }

    // 连接按钮
    if (ImGui::Button(connected ? "Disconnect" : "Connect")) {
        if (connected) {
            // 断开连接
            LOG_INFO("BlenderIntegration", "Disconnecting from Blender");
        } else {
            // 连接
            if (ipc && !ipc->IsRunning()) {
                ipc->Start();
                LOG_INFO("BlenderIntegration", "Connecting to Blender");
            }
        }
    }

    ImGui::Unindent();
}

void BlenderIntegrationPanel::RenderImportOptions() {
    ImGui::Text("Import Options");
    ImGui::Indent();

    static bool import_meshes = true;
    static bool import_materials = true;
    static bool import_textures = true;
    static bool import_lights = true;
    static bool import_cameras = true;
    static bool optimize_meshes = true;
    static bool convert_to_y_up = true;

    ImGui::Checkbox("Import Meshes", &import_meshes);
    ImGui::Checkbox("Import Materials", &import_materials);
    ImGui::Checkbox("Import Textures", &import_textures);
    ImGui::Checkbox("Import Lights", &import_lights);
    ImGui::Checkbox("Import Cameras", &import_cameras);

    ImGui::Separator();

    ImGui::Checkbox("Optimize Meshes", &optimize_meshes);
    ImGui::Checkbox("Convert to Y-Up (Blender Z-up)", &convert_to_y_up);

    // 导入按钮
    ImGui::Separator();
    if (ImGui::Button("Import Scene")) {
        ImportOptions options;
        options.import_meshes = import_meshes;
        options.import_materials = import_materials;
        options.import_textures = import_textures;
        options.import_lights = import_lights;
        options.import_cameras = import_cameras;
        options.optimize_meshes = optimize_meshes;
        options.convert_to_y_up = convert_to_y_up;

        if (importer_) {
            importer_->StartImport(options);
            status_message_ = "Importing...";
        }
    }

    ImGui::Unindent();
}

void BlenderIntegrationPanel::RenderRealtimeSync() {
    ImGui::Text("Realtime Sync");
    ImGui::Indent();

    static bool realtime_sync = false;
    bool sync_enabled = importer_ ? importer_->IsRealtimeSyncEnabled() : false;

    if (ImGui::Checkbox("Enable Realtime Sync", &realtime_sync)) {
        if (importer_) {
            importer_->EnableRealtimeSync(realtime_sync);
            status_message_ = realtime_sync ? "Realtime sync enabled" : "Realtime sync disabled";
        }
    }

    if (sync_enabled) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "●");

        ImGui::TextDisabled("Changes in Blender will be reflected in real-time");
    }

    // 手动同步按钮
    if (ImGui::Button("Sync Now")) {
        auto ipc = BlenderIPCServer::Get();
        if (ipc && ipc->IsConnected()) {
            ipc->SendSceneRequest();
            status_message_ = "Sync requested";
        }
    }

    ImGui::Unindent();
}

void BlenderIntegrationPanel::RenderTaskStatus() {
    ImGui::Text("Task Status");
    ImGui::Indent();

    // 状态消息
    ImGui::Text("Status: %s", status_message_.c_str());

    // 进度条
    if (importer_ && importer_->IsImporting()) {
        ImGui::ProgressBar(0.5f, ImVec2(0.0f, 0.0f), "Importing...");
    }

    // 任务系统状态
    if (task_system_) {
        size_t pending = task_system_->GetPendingTaskCount();
        size_t running = task_system_->GetRunningTaskCount();
        float load = task_system_->GetSystemLoad();

        ImGui::Text("Tasks - Pending: %zu, Running: %zu", pending, running);
        ImGui::ProgressBar(load, ImVec2(0.0f, 0.0f), "System Load");
    }

    ImGui::Unindent();
}

void BlenderIntegrationPanel::RenderStats() {
    ImGui::Text("Statistics");
    ImGui::Indent();

    if (importer_) {
        auto stats = importer_->GetStats();

        ImGui::Text("Total Objects: %zu", stats.total_objects);
        ImGui::Text("Imported Objects: %zu", stats.imported_objects);
        ImGui::Text("Total Vertices: %zu", stats.total_vertices);
        ImGui::Text("Total Triangles: %zu", stats.total_triangles);
        ImGui::Text("Import Time: %.2f ms", stats.import_time_ms);
    }

    if (task_system_) {
        auto stats = task_system_->GetStats();

        ImGui::Separator();
        ImGui::Text("Task System:");
        ImGui::Text("  Total: %zu", stats.total_tasks_processed);
        ImGui::Text("  Succeeded: %zu", stats.tasks_succeeded);
        ImGui::Text("  Failed: %zu", stats.tasks_failed);
        ImGui::Text("  Cancelled: %zu", stats.tasks_cancelled);
        ImGui::Text("  Avg Time: %.2f ms", stats.avg_task_time_ms);
    }

    ImGui::Unindent();
}

} // namespace Editor
} // namespace Prisma