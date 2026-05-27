#pragma once

#include "Export.h"
#include "ISubSystem.h"
#include "core/Timestep.h"
#include "localization/LocalizationManager.h"

#include <string>

namespace Prisma {
namespace Localization {

class ENGINE_API LocalizationSystem : public ISubSystem {
public:
    LocalizationSystem() = default;
    ~LocalizationSystem() override = default;

    LocalizationSystem(const LocalizationSystem&) = delete;
    LocalizationSystem& operator=(const LocalizationSystem&) = delete;

    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "LocalizationSystem"; }

    LocalizationManager& GetManager() { return m_Manager; }
    const LocalizationManager& GetManager() const { return m_Manager; }

    template<typename... Args>
    std::string Get(const std::string& key, Args&&... args) {
        return m_Manager.Get(key, std::forward<Args>(args)...);
    }

    void SetLocaleDirectory(const std::string& dir) { m_LocaleDirectory = dir; }
    const std::string& GetLocaleDirectory() const { return m_LocaleDirectory; }

private:
    void ScanAndLoadLocales();
    void SaveModifiedLocales();

    LocalizationManager m_Manager;
    std::string m_LocaleDirectory = "resources/localization";
    bool m_Dirty = false;
};

} // namespace Localization
} // namespace Prisma
