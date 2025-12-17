#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IResource;
class ITexture;
class IBuffer;
class IShader;
class IPipeline;
class IFence;

// 资源ID类型
using ResourceId = uint64_t;

// 资源类型枚举
enum class ResourceType {
    Unknown = 0,
    Texture,
    Buffer,
    Shader,
    Pipeline,
    RenderTarget,
    DepthStencil,
    Sampler
};

// 缓冲区类型
enum class BufferType {
    Vertex,
    Index,
    Constant,
    Structured,
    Raw,
    IndirectArgument
};

// 缓冲区使用标记
enum class BufferUsage {
    Default = 0,
    Immutable = 1 << 0,
    Dynamic = 1 << 1,
    Staging = 1 << 2,
    Upload = 1 << 3,
    Readback = 1 << 4,
    UnorderedAccess = 1 << 5,
    ShaderResource = 1 << 6
};
inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<int>(a) | static_cast<int>(b));
}
inline bool HasFlag(BufferUsage usage, BufferUsage flag) {
    return (static_cast<int>(usage) & static_cast<int>(flag)) != 0;
}

// 纹理类型
enum class TextureType {
    Texture2D,
    Texture3D,
    TextureCube,
    Texture2DArray,
    TextureCubeArray
};

// 纹理格式
enum class TextureFormat {
    Unknown,

    // 8位 formats
    R8_UNorm,
    R8_SNorm,
    R8_UInt,
    R8_SInt,

    // 16位 formats
    R16_UNorm,
    R16_SNorm,
    R16_Float,
    R16_UInt,
    R16_SInt,
    RG16_UNorm,
    RG16_SNorm,
    RG16_Float,
    RG16_UInt,
    RG16_SInt,

    // 32位 formats
    R32_Float,
    R32_UInt,
    R32_SInt,
    RG32_Float,
    RG32_UInt,
    RG32_SInt,
    RGB32_Float,
    RGB32_UInt,
    RGB32_SInt,
    RGBA32_Float,
    RGBA32_UInt,
    RGBA32_SInt,

    // Packed formats
    RGB8_UNorm,
    RGBA8_UNorm,
    RGBA8_SNorm,
    RGBA8_UInt,
    RGBA8_SInt,

    // Depth-stencil formats
    D16_UNorm,
    D24_UNorm_S8_UInt,
    D32_Float,
    D32_Float_S8_UInt,

    // Compressed formats
    BC1_UNorm,      // DXT1
    BC1_SRGB,       // DXT1 sRGB
    BC2_UNorm,      // DXT3
    BC2_SRGB,       // DXT3 sRGB
    BC3_UNorm,      // DXT5
    BC3_SRGB,       // DXT5 sRGB
    BC4_UNorm,      // BC4
    BC4_SNorm,      // BC4 SNorm
    BC5_UNorm,      // BC5
    BC5_SNorm,      // BC5 SNorm
    BC7_UNorm,      // BC7
    BC7_SRGB,       // BC7 sRGB
};

// 着色器类型
enum class ShaderType {
    Vertex,
    Pixel,
    Geometry,
    Hull,
    Domain,
    Compute
};

// 着色器语言
enum class ShaderLanguage {
    HLSL,
    GLSL,
    SPIRV
};

// 命令缓冲区类型
enum class CommandBufferType {
    Graphics,
    Compute,
    Copy
};

// 围栏状态
enum class FenceState {
    Idle,
    InFlight,
    Completed
};

// 裁剪矩形
struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

// 视口
struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

// 颜色
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
};

// 基础描述结构
struct ResourceDesc {
    ResourceType type = ResourceType::Unknown;
    std::string name;
    bool debug = false;
};

// 设备描述
struct DeviceDesc {
    std::string name = "RenderDevice";
    bool enableDebug = false;
    bool enableValidation = false;
    uint32_t maxFramesInFlight = 2;
};

} // namespace PrismaEngine::Graphic