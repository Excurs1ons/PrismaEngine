#include "ProjectSettingsWindow.h"
#include <imgui.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "../engine/resource/ArchiveJson.h"
#include <cstring> // for strncpy

using json = nlohmann::json;

ProjectSettingsWindow::ProjectSettingsWindow() {
    LoadSettings();
}

void ProjectSettingsWindow::Draw(bool* p_open) {
    if (!ImGui::Begin("Project Settings", p_open)) {
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
        }

        strncpy(buffer, m_settings.productName.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Product Name", buffer, sizeof(buffer))) {
            m_settings.productName = buffer;
        }

        strncpy(buffer, m_settings.version.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Version", buffer, sizeof(buffer))) {
            m_settings.version = buffer;
        }
    }

    // Resolution & Presentation
    if (ImGui::CollapsingHeader("Resolution and Presentation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputInt("Default Screen Width", &m_settings.screenWidth);
        ImGui::InputInt("Default Screen Height", &m_settings.screenHeight);
        ImGui::Checkbox("Fullscreen Mode", &m_settings.fullscreen);
        ImGui::Checkbox("Resizable Window", &m_settings.resizable);
    }

    // Save Button
    if (ImGui::Button("Save Settings")) {
        SaveSettings();
    }

    ImGui::End();
}

void ProjectSettingsWindow::LoadSettings() {
    std::ifstream file(m_settingsPath);
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            PrismaEngine::Serialization::JsonInputArchive archive(j);
            m_settings.Deserialize(archive);
        } catch (...) {
            // Handle error or use defaults
        }
    }
}

void ProjectSettingsWindow::SaveSettings() {
    PrismaEngine::Serialization::JsonOutputArchive archive;
    m_settings.Serialize(archive);

    std::ofstream file(m_settingsPath);
    if (file.is_open()) {
        file << archive.GetJson().dump(4);
    }
}