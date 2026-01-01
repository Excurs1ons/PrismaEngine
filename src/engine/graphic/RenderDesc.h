//
// Created by JasonGu on 26-1-1.
//
#pragma once

#ifndef RENDERDESC_H
#define RENDERDESC_H
#include "interfaces/RenderTypes.h"
#include <vector>

namespace PrismaEngine::Graphic {
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
}
#endif //RENDERDESC_H
