#pragma once
// 嵌入资源 - 以字节数组形式存储的必要资源

namespace PrismaEngine {
namespace Resource {

// 默认顶点着色器 (编译后的字节码)
static const unsigned char DEFAULT_VERTEX_SHADER_BYTES[] = {
    // TODO: 这里应该放入预编译的着色器字节码
    // 现在先放占位符
    0x44, 0x58, 0x42, 0x43, // "DXBC" 标识
    0x00, 0x00, 0x00, 0x00, // 占位符
};

static const size_t DEFAULT_VERTEX_SHADER_SIZE = sizeof(DEFAULT_VERTEX_SHADER_BYTES);

// 默认像素着色器 (编译后的字节码)
static const unsigned char DEFAULT_PIXEL_SHADER_BYTES[] = {
    // TODO: 这里应该放入预编译的着色器字节码
    // 现在先放占位符
    0x44, 0x58, 0x42, 0x43, // "DXBC" 标识
    0x00, 0x00, 0x00, 0x00, // 占位符
};

static const size_t DEFAULT_PIXEL_SHADER_SIZE = sizeof(DEFAULT_PIXEL_SHADER_BYTES);

// 默认纹理 (2x2 白色纹理)
static const unsigned char DEFAULT_TEXTURE_BYTES[] = {
    // DDS 头部
    0x44, 0x44, 0x53, 0x20, 0x7C, 0x00, 0x00, 0x00, // "DDS ", 124 bytes
    0x07, 0x10, 0x08, 0x00, // DDS_HEADER

    // 纹理数据 (2x2 白色 RGBA)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const size_t DEFAULT_TEXTURE_SIZE = sizeof(DEFAULT_TEXTURE_BYTES);

// 默认网格数据 (三角形)
struct DefaultMeshData {
    // 顶点数据 (位置 + 颜色)
    struct Vertex {
        float x, y, z;
        float r, g, b, a;
    };

    static const Vertex VERTICES[3];
    static const unsigned short INDICES[3];
};

const DefaultMeshData::Vertex DefaultMeshData::VERTICES[3] = {
    { 0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f },  // 红色顶点
    {-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f },  // 绿色顶点
    { 0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }   // 蓝色顶点
};

const unsigned short DefaultMeshData::INDICES[3] = { 0, 1, 2 };

} // namespace Resource
} // namespace Engine