#pragma once

#include <cstdint>
#include <string>
#include <vector>

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

// 渲染后端类型
enum class RenderBackendType {
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

// 基础描述结构
struct ResourceDesc {
    ResourceType type = ResourceType::Unknown;
    std::string name;
    bool debug = false;
};

// 设备描述
struct DeviceDesc {
    std::string name = "RenderDevice";
    void* windowHandle = nullptr;
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool enableDebug = false;
    bool enableValidation = false;
    uint32_t maxFramesInFlight = 2;
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

/// @brief 着色器编译选项
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


/// @brief 缓冲区描述
    struct BufferDesc : public ResourceDesc {
        BufferType type = BufferType::Vertex;
        uint64_t size = 0;
        BufferUsage usage = BufferUsage::Default;
        const void* initialData = nullptr;
        uint32_t stride = 0;  // 对于结构化缓冲区
    };

/// @brief 着色器描述
    struct ShaderDesc : public ResourceDesc {
        ShaderType type = ShaderType::Vertex;
        ShaderLanguage language = ShaderLanguage::HLSL;
        std::string entryPoint = "main";
        std::string source;
        std::string filename;  // 如果从文件加载
        std::vector<std::string> defines;
        std::string target;    // 如 "vs_5_0", "ps_5_0"
        uint64_t compileTimestamp = 0;
        uint64_t compileHash = 0;
        ShaderCompileOptions compileOptions;
        const std::vector<std::string> &dependencies;
        const std::vector<std::string> &includes;
    };

/// @brief 管线描述
    struct PipelineDesc : public ResourceDesc {
        // 顶点输入布局
        struct VertexAttribute {
            std::string semanticName;
            uint32_t semanticIndex = 0;
            TextureFormat format = TextureFormat::RGBA32_Float;
            uint32_t inputSlot = 0;
            uint32_t alignedByteOffset = 0;
            uint32_t inputSlotClass = 0;  // 0=vertex, 1=instance
            uint32_t instanceDataStepRate = 0;
        };
        std::vector<VertexAttribute> vertexAttributes;

        // 着色器
        std::shared_ptr<IShader> vertexShader;
        std::shared_ptr<IShader> pixelShader;
        std::shared_ptr<IShader> geometryShader;
        std::shared_ptr<IShader> hullShader;
        std::shared_ptr<IShader> domainShader;
        std::shared_ptr<IShader> computeShader;

        // 渲染状态
        struct BlendState {
            bool blendEnable = false;
            bool srcBlendAlpha = false;
            uint32_t writeMask = 0xF;  // RGBA all enabled
        };
        BlendState blendState;

        struct RasterizerState {
            bool cullEnable = true;
            bool frontCounterClockwise = false;
            bool depthClipEnable = true;
            bool scissorEnable = false;
            bool multisampleEnable = false;
            bool antialiasedLineEnable = false;
            uint32_t fillMode = 0;  // 0=solid, 1=wireframe
            uint32_t cullMode = 2;  // 0=none, 1=front, 2=back
            float depthBias = 0.0f;
            float depthBiasClamp = 0.0f;
            float slopeScaledDepthBias = 0.0f;
        };
        RasterizerState rasterizerState;

        struct DepthStencilState {
            bool depthEnable = true;
            bool depthWriteEnable = true;
            bool stencilEnable = false;
            uint8_t depthFunc = 4;  // 4=less
            uint8_t frontStencilFailOp = 1;  // 1=keep
            uint8_t frontStencilDepthFailOp = 1;
            uint8_t frontStencilPassOp = 1;
            uint8_t frontStencilFunc = 8;    // 8=always
            uint8_t backStencilFailOp = 1;
            uint8_t backStencilDepthFailOp = 1;
            uint8_t backStencilPassOp = 1;
            uint8_t backStencilFunc = 8;
            uint8_t stencilReadMask = 0xFF;
            uint8_t stencilWriteMask = 0xFF;
        };
        DepthStencilState depthStencilState;

        // 渲染目标
        uint32_t numRenderTargets = 1;
        TextureFormat renderTargetFormats[8] = {TextureFormat::RGBA8_UNorm};
        TextureFormat depthStencilFormat = TextureFormat::D32_Float;

        // 多重采样
        uint32_t sampleCount = 1;
        uint32_t sampleQuality = 0;

        // 图元拓扑
        uint32_t primitiveTopology = 4;  // 4=trianglelist
    };

/// @brief 管线状态对象描述
    struct PipelineStateDesc : public ResourceDesc {
        PipelineType type = PipelineType::Graphics;

        // 着色器
        std::shared_ptr<IShader> vertexShader;
        std::shared_ptr<IShader> pixelShader;
        std::shared_ptr<IShader> geometryShader;
        std::shared_ptr<IShader> hullShader;
        std::shared_ptr<IShader> domainShader;
        std::shared_ptr<IShader> computeShader;

        // 渲染状态
        PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;

        struct BlendState {
            bool blendEnable = false;
            bool logicOpEnable = false;
            uint32_t writeMask = 0xF;  // RGBA all enabled
            BlendOp blendOp = BlendOp::Add;
            BlendFactorType srcBlend = BlendFactorType::One;
            BlendFactorType destBlend = BlendFactorType::Zero;
            BlendOp blendOpAlpha = BlendOp::Add;
            BlendFactorType srcBlendAlpha = BlendFactorType::One;
            BlendFactorType destBlendAlpha = BlendFactorType::Zero;
        };
        BlendState blendState;

        struct RasterizerState {
            bool cullEnable = true;
            bool frontCounterClockwise = false;
            bool depthClipEnable = true;
            bool depthBiasEnable = false;
            float depthBias = 0.0f;
            float depthBiasClamp = 0.0f;
            float slopeScaledDepthBias = 0.0f;
            FillMode fillMode = FillMode::Solid;
            CullMode cullMode = CullMode::Back;
        };
        RasterizerState rasterizerState;

        struct DepthStencilState {
            bool depthEnable = true;
            bool depthWriteEnable = true;
            bool stencilEnable = false;
            ComparisonFunc depthFunc = ComparisonFunc::Less;
            uint8_t stencilReadMask = 0xFF;
            uint8_t stencilWriteMask = 0xFF;
            StencilOp frontFaceFail = StencilOp::Keep;
            StencilOp frontFaceDepthFail = StencilOp::Keep;
            StencilOp frontFacePass = StencilOp::Keep;
            ComparisonFunc frontFaceFunc = ComparisonFunc::Always;
            StencilOp backFaceFail = StencilOp::Keep;
            StencilOp backFaceDepthFail = StencilOp::Keep;
            StencilOp backFacePass = StencilOp::Keep;
            ComparisonFunc backFaceFunc = ComparisonFunc::Always;
        };
        DepthStencilState depthStencilState;

        // 顶点输入布局
        struct VertexInputAttribute {
            std::string semanticName;
            uint32_t semanticIndex = 0;
            TextureFormat format = TextureFormat::RGBA32_Float;
            uint32_t inputSlot = 0;
            uint32_t alignedByteOffset = 0;
            bool isPerInstance = false;
            uint32_t instanceDataStepRate = 0;
        };
        std::vector<VertexInputAttribute> inputLayout;

        // 渲染目标格式
        uint32_t numRenderTargets = 1;
        TextureFormat renderTargetFormats[8] = {TextureFormat::RGBA8_UNorm};
        TextureFormat depthStencilFormat = TextureFormat::D32_Float;

        // 多重采样
        uint32_t sampleCount = 1;
        uint32_t sampleQuality = 0;

        // 根签名（具体类型取决于后端）
        void* rootSignature = nullptr;
    };

} // namespace PrismaEngine::Graphic