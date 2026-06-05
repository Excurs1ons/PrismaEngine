#include "localization/LocalizationFile.h"
#include <glaze/glaze.hpp>
#include <string>

namespace Prisma {
namespace Localization {

bool LocalizationFile::Load(const std::string& path, LocaleData& outData) {
    std::string buffer;
    auto fileEc = glz::file_to_buffer(buffer, path);
    if (bool(fileEc)) {
        return false;
    }

    auto err = glz::read<glz::opts{ .error_on_unknown_keys = false }>(outData, buffer);
    return !err;
}

bool LocalizationFile::Save(const std::string& path, const LocaleData& data) {
    auto err = glz::write_file_json(data, path, std::string{});
    return !err;
}

} // namespace Localization
} // namespace Prisma
