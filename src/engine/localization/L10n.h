#pragma once
#include "app/Engine.h"
#include "localization/LocalizationSystem.h"

namespace Prisma {

template<typename... Args>
inline std::string L10n(const std::string& key, Args&&... args) {
    auto* sys = Engine::Get().GetLocalizationSystem();
    if (sys) {
        return sys->Get(key, std::forward<Args>(args)...);
    }
    return key;
}

}
