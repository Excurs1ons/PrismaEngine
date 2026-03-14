#include "ImageLoaderSTB.h"
#include "Logger.h"
#include <algorithm>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Prisma::Core {

ImageLoadResult ImageLoaderSTB::loadFromFile(const std::string& filePath) {
    ImageLoadResult result;

    std::string extension = filePath.substr(filePath.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (!isFormatSupported(extension)) {
        result.success = false;
        result.error = "Unsupported image format: " + extension;
        LOG_ERROR("ImageLoader", "Unsupported format: {}", extension);
        return result;
    }

    int width = 0, height = 0, channels = 0;
    stbi_uc* data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data) {
        result.success = false;
        result.error = "Failed to load image: " + std::string(stbi_failure_reason());
        LOG_ERROR("ImageLoader", "Failed to load {}: {}", filePath, stbi_failure_reason());
        return result;
    }

    result.success = true;
    result.width = static_cast<uint32_t>(width);
    result.height = static_cast<uint32_t>(height);
    result.channels = static_cast<uint32_t>(channels);

    size_t dataSize = width * height * channels;
    result.data.resize(dataSize);
    std::memcpy(result.data.data(), data, dataSize);

    LOG_INFO("ImageLoader", "Loaded image: {} ({}x{}x{} ch)", 
              filePath, width, height, channels);

    stbi_image_free(data);
    return result;
}

ImageLoadResult ImageLoaderSTB::loadFromMemory(const uint8_t* data, size_t size) {
    ImageLoadResult result;

    int width = 0, height = 0, channels = 0;
    stbi_uc* imageData = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data), 
                                                       static_cast<int>(size), 
                                                       &width, &height, &channels, STBI_rgb_alpha);

    if (!imageData) {
        result.success = false;
        result.error = "Failed to load image from memory";
        return result;
    }

    result.success = true;
    result.width = static_cast<uint32_t>(width);
    result.height = static_cast<uint32_t>(height);
    result.channels = static_cast<uint32_t>(channels);

    size_t dataSize = width * height * channels;
    result.data.resize(dataSize);
    std::memcpy(result.data.data(), imageData, dataSize);

    stbi_image_free(imageData);
    return result;
}

std::vector<std::string> ImageLoaderSTB::getSupportedFormats() {
    return {
        ".png",
        ".jpg",
        ".jpeg",
        ".bmp",
        ".tga",
        ".psd",
        ".gif",
        ".hdr",
        ".pic",
        ".pnm",
        ".pgm",
        ".ppm"
    };
}

bool ImageLoaderSTB::isFormatSupported(const std::string& extension) {
    auto formats = getSupportedFormats();
    return std::find(formats.begin(), formats.end(), extension) != formats.end();
}

} // namespace Core
