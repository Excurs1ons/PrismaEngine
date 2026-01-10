#pragma once

#include "Asset.h"
#include <vector>
#include <cstdint>

namespace PrismaEngine {
// 纹理资产类
class TextureAsset : public Asset {
public:
    TextureAsset()           = default;
    ~TextureAsset() override = default;

    // IResource接口实现
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    [[nodiscard]] bool IsLoaded() const override { return m_isLoaded; }
    [[nodiscard]] AssetType GetType() const override { return AssetType::Texture; }

    // Serializable接口实现
    void Serialize(Serialization::OutputArchive& archive) const override;
    void Deserialize(Serialization::InputArchive& archive) override;

    // 重写Asset的DeserializeFromFile方法
    bool
    DeserializeFromFile(const std::filesystem::path& path,
                        Serialization::SerializationFormat format = Serialization::SerializationFormat::JSON) override;

    // Asset特定方法
    [[nodiscard]] std::string GetAssetType() const override { return "Texture"; }
    [[nodiscard]] std::string GetAssetVersion() const override { return "1.0.0"; }

    // 纹理属性
    [[nodiscard]] uint32_t GetWidth() const { return width; }
    [[nodiscard]] uint32_t GetHeight() const { return height; }
    [[nodiscard]] uint32_t GetChannels() const { return channels; }
    [[nodiscard]] const std::vector<uint8_t>& GetData() const { return m_data; }

    void SetDimensions(uint32_t width, uint32_t height, uint32_t channels);
    void SetData(const std::vector<uint8_t>& data);

private:
    uint32_t width    = 0;
    uint32_t height   = 0;
    uint32_t channels = 0;
    std::vector<uint8_t> m_data;
    bool m_isLoaded = false;
};

}  // namespace Engine