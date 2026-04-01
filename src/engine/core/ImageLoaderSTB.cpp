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
        result.error = "不支持的图像格式: " + extension;
        LOG_ERROR("ImageLoader", "不支持的格式: {}", extension);
        return result;
    }

    int width = 0, height = 0, channels = 0;
    stbi_uc* data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data) {
        result.success = false;
        result.error = "加载图像失败: " + std::string(stbi_failure_reason());
        LOG_ERROR("ImageLoader", "无法加载 {}: {}", filePath, stbi_failure_reason());
        return result;
    }

    result.success = true;
    result.width = static_cast<uint32_t>(width);
    result.height = static_cast<uint32_t>(height);
    result.channels = static_cast<uint32_t>(channels);

    size_t dataSize = width * height * channels;
    result.data.resize(dataSize);
    std::memcpy(result.data.data(), data, dataSize);

    LOG_INFO("ImageLoader", "已加载图像: {} ({}x{}x{} 通道)", 
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
        result.error = "无法从内存加载图像";
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
