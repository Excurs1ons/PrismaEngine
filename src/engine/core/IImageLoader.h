#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Prisma::Core {

class IImageLoader {
public:
    virtual ~IImageLoader() = default;

    struct ImageLoadResult {
        bool success = false;
        std::string error;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
        std::vector<uint8_t> data;
    };

    virtual ImageLoadResult loadFromFile(const std::string& filePath) = 0;
    virtual ImageLoadResult loadFromMemory(const uint8_t* data, size_t size) = 0;
};

} // namespace Prisma::Core
