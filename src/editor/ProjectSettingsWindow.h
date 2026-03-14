#pragma once
#include "../engine/core/ProjectSettings.h"
#include <string>

class ProjectSettingsWindow {
public:
    ProjectSettingsWindow();

    void Draw(bool* p_open);
    void LoadSettings();
    void SaveSettings();

private:
    Prisma::Core::ProjectSettings m_settings;
    std::string m_settingsPath = "project_settings.json";
};