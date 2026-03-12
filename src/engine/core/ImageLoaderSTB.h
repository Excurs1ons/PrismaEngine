#pragma once

#include "IImageLoader.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace PrismaEngine::Core {

/**
 * @brief STB图像加载器实现
 *
 * 使用 STB 库加载图像文件（PNG、JPEG、BMP等）
 * 这是AsyncLoader的具体实现，用于异步加载纹理
 */
class ImageLoaderSTB : public IImageLoader {
public:
    ImageLoaderSTB() = default;
    ~ImageLoaderSTB() override = default;

    ImageLoadResult loadFromFile(const std::string& filePath) override;
    ImageLoadResult loadFromMemory(const uint8_t* data, size_t size) override;

    static std::vector<std::string> getSupportedFormats();

private:
    static bool isFormatSupported(const std::string& extension);
};

} // namespace PrismaEngine::Core

