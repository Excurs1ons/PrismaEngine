#pragma once
#include "LocalizationData.h"
#include <glaze/glaze.hpp>
#include <string>

namespace Prisma::Localization {

class LocalizationFile {
public:
    static bool Load(const std::string& path, LocaleData& outData);
    static bool Save(const std::string& path, const LocaleData& data);
};

}

template<>
struct glz::meta<Prisma::Localization::LocaleData> {
    using T = Prisma::Localization::LocaleData;
    static constexpr auto value = glz::object(
        &T::locale,
        &T::strings
    );
};
