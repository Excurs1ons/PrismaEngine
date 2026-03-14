#pragma once
#include <cstddef>

namespace Prisma {
namespace Resource {

static const unsigned char DEFAULT_VERTEX_SHADER_BYTES[] = {
    0x00, 0x00, 0x00, 0x00
};

static const size_t DEFAULT_VERTEX_SHADER_SIZE = sizeof(DEFAULT_VERTEX_SHADER_BYTES);

static const unsigned char DEFAULT_PIXEL_SHADER_BYTES[] = {
    0x00, 0x00, 0x00, 0x00
};

static const size_t DEFAULT_PIXEL_SHADER_SIZE = sizeof(DEFAULT_PIXEL_SHADER_BYTES);

static const unsigned char DEFAULT_TEXTURE_BYTES[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const size_t DEFAULT_TEXTURE_SIZE = sizeof(DEFAULT_TEXTURE_BYTES);

struct DefaultMeshData {
    struct Vertex {
        float x, y, z;
        float r, g, b, a;
    };

    static const Vertex VERTICES[3];
    static const unsigned short INDICES[3];
};

const DefaultMeshData::Vertex DefaultMeshData::VERTICES[3] = {
    { 0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f },
    {-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f },
    { 0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }
};

const unsigned short DefaultMeshData::INDICES[3] = { 0, 1, 2 };

} // namespace Resource
} // namespace Prisma