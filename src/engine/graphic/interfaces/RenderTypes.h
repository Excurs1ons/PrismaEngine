#pragma once

#include "math/MathTypes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Prisma::Graphic {
using namespace Prisma;
// 前置声明
class IResource;
class ITexture;
class IBuffer;
class IPipeline;
class IFence;

struct Vertex {
    Vertex(const Vector4& inPosition, const Vector4& inColor, const Vector4& inUV)
        : position(inPosition), color(inColor), uv(inUV), normal(Vector4(0, 0, 0, 0)), texCoord(Vector4(0, 0, 0, 0)), tangent(Vector4(0, 0, 0, 0)) {}

    Vertex(const Vector4& inPosition, const Vector4& inColor, const Vector4& inUV, const Vector4& inNormal)
        : position(inPosition), color(inColor), uv(inUV), normal(inNormal), texCoord(Vector4(0, 0, 0, 0)), tangent(Vector4(0, 0, 0, 0)) {}

    Vertex() {
        position = Vector4(0, 0, 0,0);
        color = Vector4(1, 1, 1, 1);
        uv = Vector4(0, 0,0,0);
        normal = Vector4(0, 0, 0, 0);
        texCoord = Vector4(0, 0, 0, 0);
        tangent = Vector4(0, 0, 0, 0);
    }

    Vertex(const Vector4& inPosition, const Vector4& inColor, const Vector4& inUV, const Vector4& inNormal, const Vector4& inTexCoord, const Vector4& inTangent) {
        position = inPosition;
        color = inColor;
        uv = inUV;
        normal = inNormal;
        texCoord = inTexCoord;
        tangent = inTangent;
    }
    Vector4 position;
    Vector4 color;
    Vector4 uv;
    Vector4 normal;       // 法线
    Vector4 texCoord;     // 纹理坐标
    Vector4 tangent;      // 切线
    constexpr static uint32_t GetVertexStride() { return sizeof(Vertex); }
};

// 2D 渲染简化顶点（posUV(vec4) + color(vec4) = 32 bytes）
// posUV.xy = 屏幕位置, posUV.zw = 纹理坐标
// 聚合体，可直接 brace-init: { Vector4(px,py,u,v), colorVec4 }
struct Vertex2D {
    Vector4 posUV;
    Vector4 color;
    constexpr static uint32_t GetVertexStride() { return sizeof(Vertex2D); }
};


// 简单的包围盒结构
struct BoundingBox {
    Vector3 minBounds;
    Vector3 maxBounds;

    BoundingBox() {
        minBounds = Vector3(0, 0, 0);
        maxBounds = Vector3(0, 0, 0);
    }
    BoundingBox(const Vector3& minVal, const Vector3& maxVal) {
        minBounds = minVal;
        maxBounds = maxVal;
    }

    // 扩展包围盒以包含点
    void Encapsulate(const Vector3& point) {
        if (point.x < minBounds.x) minBounds.x = point.x;
        if (point.y < minBounds.y) minBounds.y = point.y;
        if (point.z < minBounds.z) minBounds.z = point.z;
        if (point.x > maxBounds.x) maxBounds.x = point.x;
        if (point.y > maxBounds.y) maxBounds.y = point.y;
        if (point.z > maxBounds.z) maxBounds.z = point.z;
    }

    // 合并另一个包围盒
    void Merge(const BoundingBox& other) {
        Encapsulate(other.minBounds);
        Encapsulate(other.maxBounds);
    }

    // 获取中心点
    [[nodiscard]] Vector3 GetCenter() const {
        return (minBounds + maxBounds) * 0.5f;
    }

    // 获取尺寸
    [[nodiscard]] Vector3 GetSize() const {
        return maxBounds - minBounds;
    }
};

// 子网格 GPU 资源缓冲区
struct SubMeshBuffer {
    std::string name;
    uint32_t materialIndex;
    uint32_t baseVertex;
    uint32_t baseIndex;
    uint32_t indexCount;
    uint32_t vertexCount;
    std::shared_ptr<IBuffer> vertexBuffer;
    std::shared_ptr<IBuffer> indexBuffer;
    bool use16BitIndices;
};

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

// 渲染 API 类型
enum class RenderAPIType {
    None,
    DirectX12,
    Vulkan,
    OpenGL
};

// 缓冲区类型
enum class BufferType {
    Unknown = 0,
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
    Texture1D,
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
    RG8_UNorm,
    RG8_SNorm,

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
    RGBA16_UNorm,
    RGBA16_SNorm,
    RGBA16_Float,
    RGBA16_UInt,
    RGBA16_SInt,

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
    RGBA8_UNorm_sRGB,
    RGBA8_SNorm,
    RGBA8_UInt,
    RGBA8_SInt,
    BGRA8_UNorm,
    BGRA8_UNorm_sRGB,

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
    Compute,
    VertexAndPixel, // 同时推送到 Vertex 和 Pixel 阶段，对应 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    Unknown
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

// 呈现模式 (VSync 模式)
enum class PresentMode {
    Immediate,   // 立即呈现 (关闭垂直同步，可能导致画面撕裂)
    VSync,       // 等待垂直同步 (开启垂直同步，限制帧率为显示器刷新率)
    Mailbox,     // 邮箱模式 (三重缓冲，开启垂直同步但允许低延迟，如果支持)
    Adaptive     // 自适应垂直同步 (根据性能自动切换)
};

// 基础描述结构
struct ResourceDesc {
    ResourceType type = ResourceType::Unknown;
    std::string name;
    bool debug = false;
};

// 光线追踪模式
enum class RTMode : uint8_t {
    None = 0,        // No RT extensions required
    RayQuery = 1,    // VK_KHR_acceleration_structure + VK_KHR_ray_query
    HardwareRT = 2   // VK_KHR_acceleration_structure + VK_KHR_ray_tracing_pipeline + VK_KHR_deferred_host_operations
};

} // namespace Prisma::Graphic

// 渲染模式（引擎全局枚举，各管线通过 GetMode() 返回）
namespace Prisma {
enum class RenderMode : uint8_t {
    Mode2D = 0,
    Mode3D_Forward = 1,
    Mode3D_ForwardPlus = 2,
    Mode3D_Deferred = 3,
    Mode3D_DeferredPlus = 4,
    Mode3D_PathTracing = 5,
    Mode3D_ClusteredForward = 6,
    Mode3D_NPR = 8,
    Mode3D_MobileLumen = 9,
    SRP = 7
};
} // namespace Prisma

namespace Prisma::Graphic {

// 路径追踪计算模式
enum class PathTraceMode : uint8_t {
    Flat = 0,       ///< 暴力遍历所有三角形
    BVH = 1,        ///< BVH 加速遍历
    HardwareRT = 2, ///< 硬件光线追踪
    RayQuery = 3    ///< Ray Query 软光追
};

// 设备描述
struct DeviceDesc {
    std::string name = "RenderDevice";
    void* windowHandle = nullptr;
    uint32_t width = 1920;
    uint32_t height = 1080;
    PresentMode presentMode = PresentMode::VSync;
    bool enableDebug = false;
    bool enableValidation = false;
    uint32_t maxFramesInFlight = 2;
    bool headless = false;
    RTMode rtMode = RTMode::None;  // 光线追踪模式（None=关闭, RayQuery=软光追, HardwareRT=硬件光追）
};

// 纹理过滤模式
enum class TextureFilter : uint32_t {
    Point,
    Linear,
    Anisotropic,
    ComparisonPoint,
    ComparisonLinear,
    ComparisonAnisotropic,
    MinPointMagLinearMipPoint,
    MinPointMagLinearMipLinear,
    MinLinearMagPointMipPoint,
    MinLinearMagPointMipLinear,
    MinMagPointMipLinear,
    MinLinearMagMipPoint
};

// 纹理寻址模式
enum class TextureAddressMode : uint32_t {
    Wrap,
    Mirror,
    Clamp,
    Border,
    MirrorOnce
};

// 纹理比较函数
enum class TextureComparisonFunc : uint32_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

// 采样器描述
struct SamplerDesc {
    TextureFilter filter = TextureFilter::Linear;
    TextureAddressMode addressU = TextureAddressMode::Wrap;
    TextureAddressMode addressV = TextureAddressMode::Wrap;
    TextureAddressMode addressW = TextureAddressMode::Wrap;
    float mipLODBias = 0.0f;
    uint32_t maxAnisotropy = 16;
    TextureComparisonFunc comparisonFunc = TextureComparisonFunc::Always;
    float borderColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float minLOD = 0.0f;
    float maxLOD = 3.402823466e+38f; // FLT_MAX
};

// 混合操作
enum class BlendOp {
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max
};

// 混合因子
enum class BlendFactorType {
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstAlpha,
    InvDstAlpha,
    DstColor,
    InvDstColor,
    SrcAlphaSat,
    BlendFactor,
    InvBlendFactor,
    Src1Color,
    InvSrc1Color,
    Src1Alpha,
    InvSrc1Alpha
};

// 填充模式
enum class FillMode {
    Wireframe,
    Solid
};

// 裁剪模式
enum class CullMode {
    None,
    Front,
    Back
};

// 比较函数
enum class ComparisonFunc {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

// 图元拓扑
enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    PatchList_1ControlPoints,
    PatchList_2ControlPoints,
    PatchList_3ControlPoints,
    PatchList_4ControlPoints,
    PatchList_5ControlPoints,
    PatchList_6ControlPoints,
    PatchList_7ControlPoints,
    PatchList_8ControlPoints,
    PatchList_9ControlPoints,
    PatchList_10ControlPoints,
    PatchList_11ControlPoints,
    PatchList_12ControlPoints,
    PatchList_13ControlPoints,
    PatchList_14ControlPoints,
    PatchList_15ControlPoints,
    PatchList_16ControlPoints,
    PatchList_17ControlPoints,
    PatchList_18ControlPoints,
    PatchList_19ControlPoints,
    PatchList_20ControlPoints,
    PatchList_21ControlPoints,
    PatchList_22ControlPoints,
    PatchList_23ControlPoints,
    PatchList_24ControlPoints,
    PatchList_25ControlPoints,
    PatchList_26ControlPoints,
    PatchList_27ControlPoints,
    PatchList_28ControlPoints,
    PatchList_29ControlPoints,
    PatchList_30ControlPoints,
    PatchList_31ControlPoints,
    PatchList_32ControlPoints
};

// 管线类型
enum class PipelineType {
    Graphics,
    Compute
};

// 优化等级
enum class OptimizationLevel {
    None,
    Size,
    Speed,
    Full
};

// 着色器编译标志
enum class ShaderCompileFlag : uint32_t {
    None = 0,
    Debug = 1 << 0,
    SkipOptimization = 1 << 1,
    Strict = 1 << 2,
    WarningsAsErrors = 1 << 3
};

// 着色器编译选项
    struct ShaderCompileOptions {
        bool debug = false;
        bool optimize = true;
        bool skipValidation = false;
        bool enable16BitTypes = false;
        bool allResourcesBound = false;
        bool avoidFlowControl = false;
        bool preferFlowControl = false;
        bool enableStrictness = false;
        bool ieeeStrictness = false;
        bool warningsAsErrors = false;
        bool resourcesMayAlias = false;
        int optimizationLevel = 3;  // 0-3, higher is more optimization
        uint32_t flags = 0;  // 编译标志位
        std::vector<std::string> additionalDefines;
        std::string additionalIncludePath;
        std::string additionalArguments;
        std::vector<std::string> dependencies;  // 依赖的其他着色器
        std::vector<std::string> includeDirectories;  // 包含目录
    };

// 模板操作
enum class StencilOp {
    Keep,
    Zero,
    Replace,
    IncrementSat,
    DecrementSat,
    Invert,
    Increment,
    Decrement
};

    // 光源结构 (严格 16 字节对齐，3x vec4)
    struct Light {
        Prisma::Vector4 position;  // xyz: position, w: padding
        Prisma::Vector4 color;     // rgb: color * intensity, w: range
        Prisma::Vector4 direction; // xyz: direction, w: type (0=dir, 1=point)
    };


} // namespace Prisma::Graphic