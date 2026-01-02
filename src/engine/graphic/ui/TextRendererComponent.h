#pragma once

#include "Component.h"
#include "Mesh.h"
#include "FontAtlas.h"
#include <string>
#include <vector>
#include <memory>

namespace PrismaEngine {

class IRenderDevice;

// 文本渲染组件
class TextRendererComponent : public Component {
public:
    TextRendererComponent();
    ~TextRendererComponent() override;

    // Component 接口
    void Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;

    // 设置文本内容
    void SetText(const std::string& text);

    // 设置字体
    void SetFont(const std::string& fontPath, float fontSize);

    // 设置颜色
    void SetColor(const PrismaMath::vec4& color) { m_color = color; m_dirty = true; }

    // 获取文本内容
    const std::string& GetText() const { return m_text; }

    // 获取颜色
    const PrismaMath::vec4& GetColor() const { return m_color; }

    // 获取字体图集
    std::shared_ptr<FontAtlas> GetFontAtlas() const { return m_fontAtlas; }

    // 获取顶点数据（用于渲染）
    const std::vector<Vertex>& GetVertices() const { return m_vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_indices; }

    // 是否需要重新构建网格
    bool IsDirty() const { return m_dirty; }
    void ClearDirty() { m_dirty = false; }

    // 获取文本尺寸
    float GetTextWidth() const { return m_textWidth; }
    float GetTextHeight() const { return m_textHeight; }

private:
    // 重建网格（文本变化时调用）
    void RebuildMesh();

    // UTF-8 转 Unicode 码点
    static std::vector<char32_t> UTF8ToCodepoints(const std::string& utf8);

private:
    std::string m_text;
    std::string m_fontPath;
    float m_fontSize;
    PrismaMath::vec4 m_color;
    std::shared_ptr<FontAtlas> m_fontAtlas;
    bool m_dirty;

    // 渲染数据
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // 文本尺寸
    float m_textWidth;
    float m_textHeight;

    // 静态字符范围定义
    static const uint32_t s_charRanges[];
};

} // namespace PrismaEngine
