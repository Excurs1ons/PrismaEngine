#pragma once
#include "LocalizationData.h"
#include <string>
#include <unordered_map>
#include <sstream>
#include <vector>

namespace Prisma::Localization {

class LocalizationManager {
public:
    static constexpr const char* DEFAULT_LOCALE = "en-US";

    LocalizationManager();

    template<typename... Args>
    std::string Get(const std::string& key, Args&&... args) const {
        std::string value;

        auto currentIt = m_Locales.find(m_CurrentLocale);
        if (currentIt != m_Locales.end()) {
            auto strIt = currentIt->second.strings.find(key);
            if (strIt != currentIt->second.strings.end()) {
                value = strIt->second;
            }
        }

        if (value.empty() && m_CurrentLocale != DEFAULT_LOCALE) {
            auto defaultIt = m_Locales.find(DEFAULT_LOCALE);
            if (defaultIt != m_Locales.end()) {
                auto strIt = defaultIt->second.strings.find(key);
                if (strIt != defaultIt->second.strings.end()) {
                    value = strIt->second;
                }
            }
        }

        if (value.empty()) {
            return key;
        }

        if constexpr (sizeof...(args) > 0) {
            return FormatStringImpl(value, std::forward<Args>(args)...);
        }

        return value;
    }

    void SetLocale(const Locale& locale);
    const Locale& GetCurrentLocale() const { return m_CurrentLocale; }

    void AddLocale(const LocaleData& data);
    bool HasLocale(const Locale& locale) const;
    std::vector<Locale> GetAvailableLocales() const;

    void Reload(const std::vector<LocaleData>& allData);

private:
    std::string FormatString(const std::string& format, const std::vector<std::string>& args) const;

    template<typename T>
    std::string ToString(T&& arg) const {
        std::ostringstream oss;
        oss << std::forward<T>(arg);
        return oss.str();
    }

    template<typename... Args>
    std::string FormatStringImpl(const std::string& format, Args&&... args) const {
        std::vector<std::string> strArgs;
        (strArgs.push_back(ToString(std::forward<Args>(args))), ...);
        return FormatString(format, strArgs);
    }

    std::unordered_map<Locale, LocaleData> m_Locales;
    Locale m_CurrentLocale;
};

}
