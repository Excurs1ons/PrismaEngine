#include "localization/LocalizationSystem.h"
#include "localization/LocalizationFile.h"
#include "Logger.h"

#include <filesystem>
#include <vector>

namespace Prisma {
namespace Localization {

int LocalizationSystem::Initialize() {
    ScanAndLoadLocales();
    LOG_INFO("Localization", "本地化系统已初始化 (目录: {}, 区域: {})",
             m_LocaleDirectory, m_Manager.GetCurrentLocale());
    return 0;
}

void LocalizationSystem::Shutdown() {
    SaveModifiedLocales();
    m_Manager = LocalizationManager();
    LOG_INFO("Localization", "本地化系统已关闭");
}

void LocalizationSystem::Update(Timestep ts) {
    (void)ts;
}

void LocalizationSystem::ScanAndLoadLocales() {
    namespace fs = std::filesystem;

    if (!fs::exists(m_LocaleDirectory) || !fs::is_directory(m_LocaleDirectory)) {
        LOG_WARNING("Localization", "本地化目录不存在: {0}", m_LocaleDirectory);
        return;
    }

    std::vector<LocaleData> allData;

    for (const auto& entry : fs::directory_iterator(m_LocaleDirectory)) {
        if (!entry.is_regular_file()) continue;

        auto path = entry.path();
        if (path.extension() != ".json") continue;

        LocaleData data;
        if (LocalizationFile::Load(path.string(), data)) {
            allData.push_back(std::move(data));
            LOG_INFO("Localization", "已加载区域文件: {0} (locale: {1})",
                     path.filename().string(), allData.back().locale);
        } else {
            LOG_ERROR("Localization", "加载区域文件失败: {0}", path.string());
        }
    }

    m_Manager.Reload(allData);
    m_Dirty = false;

    auto locales = m_Manager.GetAvailableLocales();
    LOG_INFO("Localization", "已加载 {0} 个区域", locales.size());
}

void LocalizationSystem::SaveModifiedLocales() {
    if (!m_Dirty) return;
    m_Dirty = false;
}

} // namespace Localization
} // namespace Prisma
