#pragma once

#include "../core/Asset.h"
#include <vector>
#include <cstdint>

namespace Prisma {

// 纹理资产类
class TextureAsset : public Asset {
public:
    TextureAsset()           = default;
    ~TextureAsset() override = default;

    // Asset接口实现
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    AssetType GetType() const override { return AssetType::Texture; }

    // Serializable接口实现
    void Serialize(Serialization::OutputArchive& archive) const override;
    void Deserialize(Serialization::InputArchive& archive) override;

    // Asset特定方法
    std::string GetAssetType() const override { return "Texture"; }

    // 纹理属性
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetChannels() const { return m_Channels; }
    const std::vector<uint8_t>& GetData() const { return m_Data; }

    void SetDimensions(uint32_t width, uint32_t height, uint32_t channels);
    void SetData(const std::vector<uint8_t>& data);

private:
    uint32_t m_Width    = 0;
    uint32_t m_Height   = 0;
    uint32_t m_Channels = 0;
    std::vector<uint8_t> m_Data;
};

} // namespace Prisma
