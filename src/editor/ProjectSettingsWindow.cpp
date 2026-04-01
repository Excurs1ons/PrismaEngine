#include "ProjectSettingsWindow.h"
#include "UIStrings.h"
#include <imgui.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "../engine/resource/ArchiveJson.h"
#include <cstring> // for strncpy
#include "Editor.h"
using json = nlohmann::json;

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
            json j;
            file >> j;
            Prisma::Serialization::JsonInputArchive archive(j);
            m_settings.Deserialize(archive);
        } catch (...) {
            // Handle error or use defaults
        }
    }
}

void ProjectSettingsWindow::SaveSettings() {
    Prisma::Serialization::JsonOutputArchive archive;
    m_settings.Serialize(archive);

    std::ofstream file(m_settingsPath);
    if (file.is_open()) {
        file << archive.GetJson().dump(4);
    }
}