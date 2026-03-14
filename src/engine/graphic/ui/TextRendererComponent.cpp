#include "TextRendererComponent.h"
#include "Logger.h"
#include <algorithm>

namespace Prisma {

TextRendererComponent::TextRendererComponent()
    : m_fontSize(24.0f)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_dirty(true)
    , m_textWidth(0.0f)
    , m_textHeight(0.0f)
{
}

TextRendererComponent::~TextRendererComponent()
{
}

void TextRendererComponent::Initialize()
{
}

void TextRendererComponent::Update(Timestep ts)
{
    if (m_dirty) {
        RebuildMesh();
        m_dirty = false;
    }
}

void TextRendererComponent::Shutdown()
{
}

void TextRendererComponent::SetText(const std::string& text)
{
    if (m_text != text) {
        m_text = text;
        m_dirty = true;
    }
}

void TextRendererComponent::SetFont(const std::string& fontPath, float fontSize)
{
    m_fontPath = fontPath;
    m_fontSize = fontSize;
    m_dirty = true;
}

void TextRendererComponent::RebuildMesh()
{
    m_vertices.clear();
    m_indices.clear();

    if (m_text.empty()) {
        return;
    }

    // Basic implementation: create a quad for each character
    float xCursor = 0.0f;
    uint32_t vertexOffset = 0;

    for (char c : m_text) {
        float charWidth = m_fontSize * 0.5f;
        float charHeight = m_fontSize;

        Graphic::Vertex v0, v1, v2, v3;
        v0.position = { xCursor, 0.0f, 0.0f, 1.0f };
        v1.position = { xCursor + charWidth, 0.0f, 0.0f, 1.0f };
        v2.position = { xCursor + charWidth, charHeight, 0.0f, 1.0f };
        v3.position = { xCursor, charHeight, 0.0f, 1.0f };

        v0.color = m_color;
        v1.color = m_color;
        v2.color = m_color;
        v3.color = m_color;

        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
        m_vertices.push_back(v2);
        m_vertices.push_back(v3);

        m_indices.push_back(vertexOffset + 0);
        m_indices.push_back(vertexOffset + 1);
        m_indices.push_back(vertexOffset + 2);
        m_indices.push_back(vertexOffset + 0);
        m_indices.push_back(vertexOffset + 2);
        m_indices.push_back(vertexOffset + 3);

        xCursor += charWidth;
        vertexOffset += 4;
    }

    m_textWidth = xCursor;
    m_textHeight = m_fontSize;
}

} // namespace Prisma
