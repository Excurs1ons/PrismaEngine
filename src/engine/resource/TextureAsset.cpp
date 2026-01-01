#include "TextureAsset.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

// 这里假设有一个简单的图像加载函数，实际项目中可能需要使用stb_image等库
bool LoadImageFromFile(const std::filesystem::path& path, 
                      uint32_t& width, uint32_t& height, uint32_t& channels, 
                      std::vector<uint8_t>& data) {
    // 简单的BMP加载实现，仅用于演示
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    // 读取BMP文件头
    char header[54];
    file.read(header, 54);
    
    // 检查BMP标识符
    if (header[0] != 'B' || header[1] != 'M') {
        return false;
    }

    // 获取图像信息
    width = *reinterpret_cast<uint32_t*>(&header[18]);
    height = *reinterpret_cast<uint32_t*>(&header[22]);
    uint16_t bitsPerPixel = *reinterpret_cast<uint16_t*>(&header[28]);
    channels = bitsPerPixel / 8;

    // 计算数据大小
    uint32_t dataSize = width * height * channels;
    data.resize(dataSize);

    // 读取像素数据
    file.read(reinterpret_cast<char*>(data.data()), dataSize);

    // BMP的像素顺序是BGR，转换为RGB
    if (channels >= 3) {
        for (size_t i = 0; i < data.size(); i += channels) {
            std::swap(data[i], data[i + 2]);
        }
    }

    return true;
}

namespace PrismaEngine {
using namespace Serialization;
bool TextureAsset::Load(const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            LOG_ERROR("Texture", "Texture file does not exist: {0}", path.string());
            return false;
        }

        // 加载图像数据
        if (!LoadImageFromFile(path, m_width, m_height, m_channels, m_data)) {
            LOG_ERROR("Texture", "Failed to load texture from file: {0}", path.string());
            return false;
        }

        // 设置路径和名称
        m_path                = path;
        m_name                = path.filename().string();
        m_metadata.sourcePath = path;
        m_metadata.name       = m_name;

        m_isLoaded = true;
        LOG_INFO("Texture", "Successfully loaded texture: {0} ({1}x{2})", m_name, m_width, m_height);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Texture", "Exception while loading texture: {0}", e.what());
        return false;
    }
}

void TextureAsset::Unload() {
    m_data.clear();
    m_width    = 0;
    m_height   = 0;
    m_channels = 0;
    m_isLoaded = false;
    LOG_INFO("Texture", "Unloaded texture: {0}", m_name);
}

void TextureAsset::Serialize(OutputArchive& archive) const {
    // 序列化元数据
    archive.BeginObject();
    archive("metadata", m_metadata);

    // 序列化纹理属性
    archive("width", m_width);
    archive("height", m_height);
    archive("channels", m_channels);

    // 序列化纹理数据
    archive.BeginArray(m_data.size());
    for (const auto& byte : m_data) {
        archive("", static_cast<uint32_t>(byte));
    }
    archive.EndArray();

    archive.EndObject();
}

void TextureAsset::Deserialize(InputArchive& archive) {
    // 反序列化元数据
    size_t fieldCount = archive.BeginObject();

    for (size_t i = 0; i < fieldCount; ++i) {
        if (archive.HasNextField("metadata")) {
            m_metadata.Deserialize(archive);
        } else if (archive.HasNextField("width")) {
            m_width = archive.ReadUInt32();
        } else if (archive.HasNextField("height")) {
            m_height = archive.ReadUInt32();
        } else if (archive.HasNextField("channels")) {
            m_channels = archive.ReadUInt32();
        } else if (archive.HasNextField("data")) {
            size_t dataSize = archive.BeginArray();
            m_data.resize(dataSize);
            for (size_t j = 0; j < dataSize; ++j) {
                m_data[j] = static_cast<uint8_t>(archive.ReadUInt32());
            }
            archive.EndArray();
        }
    }

    archive.EndObject();

    // 设置加载状态
    m_isLoaded = !m_data.empty();
    m_name     = m_metadata.name;
}

bool TextureAsset::DeserializeFromFile(const std::filesystem::path& path, SerializationFormat format) {
    try {
        auto deserializedAsset = AssetSerializer::DeserializeFromFile<TextureAsset>(path, format);
        if (deserializedAsset) {
            // 复制数据到当前对象
            m_width    = deserializedAsset->m_width;
            m_height   = deserializedAsset->m_height;
            m_channels = deserializedAsset->m_channels;
            m_data     = std::move(deserializedAsset->m_data);
            m_metadata = deserializedAsset->m_metadata;
            m_path     = path;
            m_name     = deserializedAsset->m_name;
            m_isLoaded = true;

            LOG_INFO("Texture", "Successfully deserialized texture: {0}", m_name);
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Texture", "Exception while deserializing texture: {0}", e.what());
        return false;
    }
}

void TextureAsset::SetDimensions(uint32_t width, uint32_t height, uint32_t channels) {
    m_width    = width;
    m_height   = height;
    m_channels = channels;
    m_data.resize(width * height * channels);
}

void TextureAsset::SetData(const std::vector<uint8_t>& data) {
    m_data     = data;
    m_isLoaded = !m_data.empty();
}
}  // namespace Engine