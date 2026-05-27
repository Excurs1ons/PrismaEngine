#pragma once
#include <string>
#include <unordered_map>

namespace Prisma::Localization {

using Locale = std::string;
using StringTable = std::unordered_map<std::string, std::string>;

struct LocaleData {
    Locale locale;
    StringTable strings;
};

}
