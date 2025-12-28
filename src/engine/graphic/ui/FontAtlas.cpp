#include "FontAtlas.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceFactory.h"
#include "interfaces/ITexture.h"

// stb 头文件（由 FetchContent 下载，通过 CMake include 目录添加）
// 注意：STB_IMPLEMENTATION 宏在 stb_impl.cpp 中定义，这里只包含头文件
#include "stb_truetype.h"
#include "stb_rect_pack.h"

#include <fstream>
#include <iostream>

namespace PrismaEngine {

FontAtlas::FontAtlas()
    : m_atlasWidth(2048)
    , m_atlasHeight(2048)
    , m_fontSize(32.0f)
    , m_lineHeight(0.0f)
    , m_loaded(false)
    , m_uploaded(false) {
}

FontAtlas::~FontAtlas() {
}

bool FontAtlas::LoadFromTTF(const std::string& ttfPath, float fontSize, const uint32_t* charRanges) {
    // 读取 TTF 文件
    std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open font file: " << ttfPath << std::endl;
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> ttfBuffer(fileSize);
    file.read(reinterpret_cast<char*>(ttfBuffer.data()), fileSize);
    file.close();

    // 初始化 stb_truetype
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, ttfBuffer.data(), 0)) {
        std::cerr << "Failed to initialize font from: " << ttfPath << std::endl;
        return false;
    }

    m_fontSize = fontSize;

    // 计算缩放因子
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);

    // 获取字体度量
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    m_lineHeight = (ascent - descent + lineGap) * scale;

    // 初始化图集像素缓冲区
    m_pixels.clear();
    m_pixels.resize(m_atlasWidth * m_atlasHeight * 4, 0);  // RGBA8

    // 初始化矩形打包上下文
    std::vector<stbrp_rect> rects;
    std::vector<stbtt_packedchar> packedChars;

    // 统计需要打包的字符数量
    int charCount = 0;
    for (const uint32_t* range = charRanges; range[0] != 0 || range[1] != 0; range += 2) {
        charCount += range[1] - range[0] + 1;
    }

    rects.reserve(charCount);
    packedChars.resize(charCount);

    // 创建打包上下文
    stbrp_context packContext;
    std::vector<stbrp_node> packNodes(m_atlasWidth);
    stbrp_init_target(&packContext, m_atlasWidth, m_atlasHeight, packNodes.data(), m_atlasWidth);

    // 收集需要打包的字符
    int charIndex = 0;
    for (const uint32_t* range = charRanges; range[0] != 0 || range[1] != 0; range += 2) {
        for (char32_t codepoint = range[0]; codepoint <= range[1]; ++codepoint) {
            int ix0, iy0, ix1, iy1;
            stbtt_GetCodepointBitmapBox(&fontInfo, codepoint, scale, scale, &ix0, &iy0, &ix1, &iy1);

            int gw = ix1 - ix0;
            int gh = iy1 - iy0;

            if (gw <= 0 || gh <= 0) {
                // 空字符（如空格）
                rects.push_back({0, 0, 1, 1, 0, 0});
            } else {
                rects.push_back({0, 0, static_cast<stbrp_coord>(gw), static_cast<stbrp_coord>(gh), 0, 0});
            }
            charIndex++;
        }
    }

    // 执行矩形打包
    stbrp_pack_rects(&packContext, rects.data(), charCount);

    // 将字符打包到位图图集
    charIndex = 0;
    for (const uint32_t* range = charRanges; range[0] != 0 || range[1] != 0; range += 2) {
        for (char32_t codepoint = range[0]; codepoint <= range[1]; ++codepoint) {
            stbrp_rect& rect = rects[charIndex];
            stbtt_packedchar& pc = packedChars[charIndex];

            int ix0, iy0, ix1, iy1;
            stbtt_GetCodepointBitmapBox(&fontInfo, codepoint, scale, scale, &ix0, &iy0, &ix1, &iy1);

            int gw = ix1 - ix0;
            int gh = iy1 - iy0;

            // 填充字形信息
            CharGlyph glyph;
            glyph.width = static_cast<float>(gw);
            glyph.height = static_cast<float>(gh);

            if (rect.was_packed && gw > 0 && gh > 0) {
                // 渲染字形到位图图集
                stbtt_MakeCodepointBitmap(
                    &fontInfo,
                    m_pixels.data() + (rect.y * m_atlasWidth + rect.x) * 4,
                    gw, gh, m_atlasWidth * 4,
                    scale, scale,
                    codepoint
                );

                // 将单通道灰度值复制到 RGBA 通道
                for (int y = 0; y < gh; ++y) {
                    for (int x = 0; x < gw; ++x) {
                        size_t srcIdx = (rect.y + y) * m_atlasWidth + (rect.x + x);
                        unsigned char alpha = m_pixels[srcIdx * 4];
                        m_pixels[srcIdx * 4 + 0] = 255;  // R
                        m_pixels[srcIdx * 4 + 1] = 255;  // G
                        m_pixels[srcIdx * 4 + 2] = 255;  // B
                        m_pixels[srcIdx * 4 + 3] = alpha; // A
                    }
                }

                // 计算 UV 坐标
                glyph.u0 = static_cast<float>(rect.x) / m_atlasWidth;
                glyph.v0 = static_cast<float>(rect.y) / m_atlasHeight;
                glyph.u1 = static_cast<float>(rect.x + gw) / m_atlasWidth;
                glyph.v1 = static_cast<float>(rect.y + gh) / m_atlasHeight;

                // 保存打包信息
                pc.x0 = rect.x;
                pc.y0 = rect.y;
                pc.x1 = rect.x + gw;
                pc.y1 = rect.y + gh;
                pc.xoff = ix0;
                pc.yoff = iy0;
                // 使用 stbtt_GetCodepointHMetrics 获取 advance 值
                int advanceWidth;
                stbtt_GetCodepointHMetrics(&fontInfo, codepoint, &advanceWidth, nullptr);
                pc.xadvance = static_cast<int>(advanceWidth * scale);
                pc.xoff2 = ix1;
                pc.yoff2 = iy1;
            } else {
                // 未打包或空字符
                glyph.u0 = glyph.u1 = 0.0f;
                glyph.v0 = glyph.v1 = 0.0f;

                pc.xoff = ix0;
                pc.yoff = iy0;
                // 使用 stbtt_GetCodepointHMetrics 获取 advance 值
                int advanceWidth;
                stbtt_GetCodepointHMetrics(&fontInfo, codepoint, &advanceWidth, nullptr);
                pc.xadvance = static_cast<int>(advanceWidth * scale);
                pc.xoff2 = ix1;
                pc.yoff2 = iy1;
            }

            glyph.xBearing = static_cast<float>(ix0);
            glyph.yBearing = static_cast<float>(-iy1);  // 注意：字形坐标系 Y 向上，屏幕坐标系 Y 向下
            glyph.xAdvance = pc.xadvance;

            m_glyphs[codepoint] = glyph;
            charIndex++;
        }
    }

    m_loaded = true;
    return true;
}

void FontAtlas::UploadToGPU(Graphic::IRenderDevice* device) {
    if (!m_loaded || m_uploaded) {
        return;
    }

    auto factory = device->GetResourceFactory();

    // 创建纹理描述
    Graphic::TextureDesc texDesc = {};
    texDesc.type = Graphic::TextureType::Texture2D;
    texDesc.format = Graphic::TextureFormat::RGBA8_UNorm;
    texDesc.width = m_atlasWidth;
    texDesc.height = m_atlasHeight;
    texDesc.depth = 1;
    texDesc.mipLevels = 1;
    texDesc.arraySize = 1;
    texDesc.allowRenderTarget = false;
    texDesc.allowUnorderedAccess = false;
    texDesc.allowShaderResource = true;
    texDesc.initialData = m_pixels.data();
    texDesc.dataSize = m_pixels.size();

    // 创建纹理
    auto texture = factory->CreateTextureImpl(texDesc);
    if (texture) {
        m_texture = std::move(texture);
        m_uploaded = true;
    } else {
        std::cerr << "Failed to create font atlas texture" << std::endl;
    }
}

const CharGlyph* FontAtlas::GetGlyph(char32_t codepoint) const {
    auto it = m_glyphs.find(codepoint);
    if (it != m_glyphs.end()) {
        return &it->second;
    }

    // 返回空格作为默认
    static const CharGlyph s_emptyGlyph = {0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    return &s_emptyGlyph;
}

} // namespace PrismaEngine
