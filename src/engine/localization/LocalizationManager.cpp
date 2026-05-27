#include "LocalizationManager.h"

namespace Prisma::Localization {

LocalizationManager::LocalizationManager()
    : m_CurrentLocale(DEFAULT_LOCALE)
{
}

void LocalizationManager::SetLocale(const Locale& locale) {
    if (m_Locales.find(locale) != m_Locales.end()) {
        m_CurrentLocale = locale;
    }
}

void LocalizationManager::AddLocale(const LocaleData& data) {
    m_Locales[data.locale] = data;
}

bool LocalizationManager::HasLocale(const Locale& locale) const {
    return m_Locales.find(locale) != m_Locales.end();
}

std::vector<Locale> LocalizationManager::GetAvailableLocales() const {
    std::vector<Locale> locales;
    locales.reserve(m_Locales.size());
    for (const auto& [locale, _] : m_Locales) {
        locales.push_back(locale);
    }
    return locales;
}

void LocalizationManager::Reload(const std::vector<LocaleData>& allData) {
    m_Locales.clear();
    m_Locales.reserve(allData.size());
    for (const auto& data : allData) {
        m_Locales[data.locale] = data;
    }
    if (!HasLocale(m_CurrentLocale) && !m_Locales.empty()) {
        m_CurrentLocale = m_Locales.begin()->first;
    }
}

std::string LocalizationManager::FormatString(
    const std::string& format,
    const std::vector<std::string>& args) const
{
    std::string result = format;
    for (int i = static_cast<int>(args.size()) - 1; i >= 0; --i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }
    return result;
}

}
