#include "TextRendererComponent.h"
#include "interfaces/IRenderDevice.h"
#include <iostream>

namespace PrismaEngine {

// 默认字符范围：ASCII (0-127) + 常用中文 (0x4E00-0x9FFF)
const uint32_t TextRendererComponent::s_charRanges[] = {
    0,      127,    // ASCII
    0x4E00, 0x9FFF, // CJK Unified Ideographs
    0,      0       // 结束标记
};

TextRendererComponent::TextRendererComponent()
    : m_text("Hello World")
    , m_fontSize(32.0f)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_dirty(true)
    , m_textWidth(0.0f)
    , m_textHeight(0.0f) {
}

TextRendererComponent::~TextRendererComponent() {
}

void TextRendererComponent::Initialize() {
    // 如果没有设置字体，使用默认字体
    if (m_fontPath.empty()) {
        // TODO: 设置默认字体路径
        m_fontPath = "fonts/default.ttf";
    }

    // 创建字体图集
    m_fontAtlas = std::make_shared<FontAtlas>();
    if (!m_fontAtlas->LoadFromTTF(m_fontPath, m_fontSize, s_charRanges)) {
        std::cerr << "Failed to load font: " << m_fontPath << std::endl;
    }

    // 初始构建网格
    RebuildMesh();
}

void TextRendererComponent::Update(float deltaTime) {
    // 文本内容变化时重新构建网格
    if (m_dirty) {
        RebuildMesh();
        m_dirty = false;
    }
}

void TextRendererComponent::Shutdown() {
    m_vertices.clear();
    m_indices.clear();
    m_fontAtlas.reset();
}

void TextRendererComponent::SetText(const std::string& text) {
    if (m_text != text) {
        m_text = text;
        m_dirty = true;
    }
}

void TextRendererComponent::SetFont(const std::string& fontPath, float fontSize) {
    if (m_fontPath != fontPath || m_fontSize != fontSize) {
        m_fontPath = fontPath;
        m_fontSize = fontSize;

        // 重新加载字体
        m_fontAtlas = std::make_shared<FontAtlas>();
        if (m_fontAtlas->LoadFromTTF(m_fontPath, m_fontSize, s_charRanges)) {
            m_dirty = true;
        }
    }
}

void TextRendererComponent::RebuildMesh() {
    m_vertices.clear();
    m_indices.clear();

    if (!m_fontAtlas || !m_fontAtlas->IsLoaded()) {
        return;
    }

    // 转换 UTF-8 到 Unicode 码点
    std::vector<char32_t> codepoints = UTF8ToCodepoints(m_text);

    float x = 0.0f;
    float y = 0.0f;
    float lineHeight = m_fontAtlas->GetLineHeight();
    float maxWidth = 0.0f;
    float currentLineWidth = 0.0f;

    // 为每个字符生成四边形
    for (char32_t codepoint : codepoints) {
        // 处理换行符
        if (codepoint == '\n') {
            x = 0.0f;
            y += lineHeight;
            if (currentLineWidth > maxWidth) {
                maxWidth = currentLineWidth;
            }
            currentLineWidth = 0.0f;
            continue;
        }

        // 获取字形信息
        const CharGlyph* glyph = m_fontAtlas->GetGlyph(codepoint);
        if (!glyph) {
            continue;
        }

        // 计算字符位置
        float x0 = x + glyph->xBearing;
        float y0 = y + glyph->yBearing + m_fontSize;  // 调整 Y 坐标
        float x1 = x0 + glyph->width;
        float y1 = y0 + glyph->height;

        // 添加四个顶点
        Vertex v0, v1, v2, v3;

        // 位置 (左上, 右上, 左下, 右下)
        v0.position = Vector4(x0, y0, 0.0f,0.0f);
        v1.position = Vector4(x1, y0, 0.0f,0.0f);
        v2.position = Vector4(x0, y1, 0.0f,0.0f);
        v3.position = Vector4(x1, y1, 0.0f,0.0f);

        // UV 坐标
        v0.texCoord = PrismaMath::vec4(glyph->u0, glyph->v0, 0.0f, 0.0f);
        v1.texCoord = PrismaMath::vec4(glyph->u1, glyph->v0, 0.0f, 0.0f);
        v2.texCoord = PrismaMath::vec4(glyph->u0, glyph->v1, 0.0f, 0.0f);
        v3.texCoord = PrismaMath::vec4(glyph->u1, glyph->v1, 0.0f, 0.0f);

        // 颜色
        v0.color = m_color;
        v1.color = m_color;
        v2.color = m_color;
        v3.color = m_color;

        // 法线（不需要，设为 0）
        v0.normal = v1.normal = v2.normal = v3.normal = PrismaMath::vec4(0.0f, 0.0f, 1.0f, 0.0f);

        // 切线（不需要，设为 0）
        v0.tangent = v1.tangent = v2.tangent = v3.tangent = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        // 添加顶点
        uint32_t baseIndex = static_cast<uint32_t>(m_vertices.size());
        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
        m_vertices.push_back(v2);
        m_vertices.push_back(v3);

        // 添加索引（两个三角形组成一个四边形）
        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 1);
        m_indices.push_back(baseIndex + 1);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 3);

        // 移动光标
        x += glyph->xAdvance;
        currentLineWidth = x;
    }

    // 更新文本尺寸
    m_textWidth = (maxWidth > currentLineWidth) ? maxWidth : currentLineWidth;
    m_textHeight = y + lineHeight;
}

std::vector<char32_t> TextRendererComponent::UTF8ToCodepoints(const std::string& utf8) {
    std::vector<char32_t> codepoints;
    size_t i = 0;
    size_t len = utf8.length();

    while (i < len) {
        char32_t codepoint = 0;
        unsigned char c = static_cast<unsigned char>(utf8[i]);

        if (c < 0x80) {
            // 1-byte sequence
            codepoint = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence
            if (i + 1 >= len) break;
            codepoint = ((c & 0x1F) << 6) | (utf8[i + 1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence
            if (i + 2 >= len) break;
            codepoint = ((c & 0x0F) << 12) | ((utf8[i + 1] & 0x3F) << 6) | (utf8[i + 2] & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence
            if (i + 3 >= len) break;
            codepoint = ((c & 0x07) << 18) | ((utf8[i + 1] & 0x3F) << 12) |
                        ((utf8[i + 2] & 0x3F) << 6) | (utf8[i + 3] & 0x3F);
            i += 4;
        } else {
            // 无效的 UTF-8 序列
            i += 1;
            continue;
        }

        codepoints.push_back(codepoint);
    }

    return codepoints;
}

} // namespace PrismaEngine
