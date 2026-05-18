#include "ProjectSettingsWindow.h"
#include "../UIStrings.h"
#include "../core/Editor.h"
#include "resource/ArchiveJson.h"
#include "logger/Logger.h"
#include <cstring>  // for strncpy
#include <fstream>
#include <imgui.h>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
using json = glz::json_t;

using namespace Prisma;

ProjectSettingsWindow::ProjectSettingsWindow() {
    LoadSettings();
}

void ProjectSettingsWindow::Draw(bool* p_open) {
    if (!ImGui::Begin(UI::WINDOW_PROJECT_SETTINGS, p_open)) {
        ImGui::End();
        return;
    }

    // Company & Product Info
    if (ImGui::CollapsingHeader("Product Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        char buffer[256];

        strncpy(buffer, m_settings.companyName.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Company Name", buffer, sizeof(buffer))) {
            m_settings.companyName = buffer;
            Editor::Get().SetProjectDirty(true);
        }

        strncpy(buffer, m_settings.productName.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Product Name", buffer, sizeof(buffer))) {
            m_settings.productName = buffer;
            Editor::Get().SetProjectDirty(true);
        }

        strncpy(buffer, m_settings.version.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Version", buffer, sizeof(buffer))) {
            m_settings.version = buffer;
            Editor::Get().SetProjectDirty(true);
        }
    }

    // Resolution & Presentation
    if (ImGui::CollapsingHeader("Resolution and Presentation", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::InputInt("Default Screen Width", &m_settings.screenWidth)) {
            Editor::Get().SetProjectDirty(true);
        }
        if (ImGui::InputInt("Default Screen Height", &m_settings.screenHeight)) {
            Editor::Get().SetProjectDirty(true);
        }
        if (ImGui::Checkbox("Fullscreen Mode", &m_settings.fullscreen)) {
            Editor::Get().SetProjectDirty(true);
        }
        if (ImGui::Checkbox("Resizable Window", &m_settings.resizable)) {
            Editor::Get().SetProjectDirty(true);
        }
    }

    // Save Button
    if (ImGui::Button(UI::ITEM_SAVE_SETTINGS)) {
        SaveSettings();
        Editor::Get().SetProjectDirty(false);
    }

    ImGui::End();
}

void ProjectSettingsWindow::LoadSettings() {
    std::ifstream file(m_settingsPath);
    if (file.is_open()) {
        try {
            std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            json j;
            if (auto ec = glz::read_json(j, jsonStr)) {
                LOG_WARN("ProjectSettings", "解析设置 JSON 失败: {}", ec.custom_error_message);
            } else {
                Prisma::Serialization::JsonInputArchive archive(j);
                m_settings.Deserialize(archive);
            }
        } catch (const std::exception& e) {
            LOG_WARN("ProjectSettings", "加载设置异常: {}", e.what());
        }
    }
}

void ProjectSettingsWindow::SaveSettings() {
    Prisma::Serialization::JsonOutputArchive archive;
    m_settings.Serialize(archive);

    std::ofstream file(m_settingsPath);
    if (file.is_open()) {
        std::string buffer;
        if (auto ec = glz::write<glz::opts{.prettify = true}>(archive.GetJson(), buffer)) {
            LOG_WARN("ProjectSettings", "序列化设置 JSON 失败: {}", ec.custom_error_message);
        }
        file << buffer;
    }
}