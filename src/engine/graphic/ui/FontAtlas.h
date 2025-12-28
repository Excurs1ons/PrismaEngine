#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace PrismaEngine {

class ITexture;
class IRenderDevice;

// 字形信息
struct CharGlyph {
    float u0, v0;         // 纹理坐标左上角
    float u1, v1;         // 纹理坐标右下角
    float xAdvance;       // 光标前进距离
    float xBearing;       // 左边距
    float yBearing;       // 顶边距
    float width;          // 字形宽度
    float height;         // 字形高度
};

// 字体图集
class FontAtlas {
public:
    FontAtlas();
    ~FontAtlas();

    // 从 TTF 文件加载字体
    // charRanges: 字符范围数组，格式为 {start, end}，以 0 结尾
    // 示例: {{0, 127}, {0x4E00, 0x9FFF}, {0, 0}} 支持 ASCII 和 CJK 统一汉字
    bool LoadFromTTF(const std::string& ttfPath, float fontSize, const uint32_t* charRanges);

    // 上传图集到 GPU
    void UploadToGPU(IRenderDevice* device);

    // 获取字形信息
    const CharGlyph* GetGlyph(char32_t codepoint) const;

    // 获取纹理
    ITexture* GetTexture() const { return m_texture.get(); }

    // 获取字体大小
    float GetFontSize() const { return m_fontSize; }

    // 获取行高
    float GetLineHeight() const { return m_lineHeight; }

    // 是否已加载
    bool IsLoaded() const { return m_loaded; }
    bool IsUploaded() const { return m_uploaded; }

    // 获取图集尺寸
    uint32_t GetAtlasWidth() const { return m_atlasWidth; }
    uint32_t GetAtlasHeight() const { return m_atlasHeight; }

private:
    uint32_t m_atlasWidth;
    uint32_t m_atlasHeight;
    std::vector<unsigned char> m_pixels;  // RGBA8
    std::unordered_map<char32_t, CharGlyph> m_glyphs;
    std::shared_ptr<ITexture> m_texture;
    float m_fontSize;
    float m_lineHeight;
    bool m_loaded;
    bool m_uploaded;
};

} // namespace PrismaEngine
