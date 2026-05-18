#include "ShaderFactory.h"
#include "SpirvReflector.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IShader.h"
#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <vector>

// [fix] glslang 头文件必须在命名空间外部 include，否则 glslang::TShader 等
// 会被嵌套到 Prisma::Graphic::glslang::TShader 中导致链接错误
#ifdef PRISMA_HAS_GLSLANG
// clang-format off
#include <Public/ShaderLang.h>
#include <Include/ResourceLimits.h>
#include <Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
// clang-format on
#endif

namespace Prisma::Graphic {

// ============================================================
// GLSL → SPIR-V Compilation
// ============================================================

#ifdef PRISMA_HAS_GLSLANG

static EShLanguage ToShLanguage(ShaderType t) {
    switch (t) {
        case ShaderType::Vertex:   return EShLangVertex;
        case ShaderType::Pixel:    return EShLangFragment;
        case ShaderType::Compute:  return EShLangCompute;
        case ShaderType::Geometry: return EShLangGeometry;
        case ShaderType::Hull:     return EShLangTessControl;
        case ShaderType::Domain:   return EShLangTessEvaluation;
        default: return EShLangVertex;
    }
}

static std::vector<uint8_t> CompileGLSL(const ShaderDesc& desc) {
    static bool glslangInitialized = false;
    if (!glslangInitialized) {
        if (!glslang::InitializeProcess()) {
            LOG_ERROR("ShaderFactory", "glslang::InitializeProcess() failed");
            return {};
        }
        glslangInitialized = true;
    }

    EShLanguage stage = ToShLanguage(desc.type);
    const char* srcStr = desc.source.c_str();

    glslang::TShader shader(stage);
    shader.setStrings(&srcStr, 1);

    // 显式设置版本和入口点
    shader.setOverrideVersion(450);            // 强制版本 450
    shader.setEntryPoint("main");

    // Vulkan 1.1 目标 + SPIR-V 1.3
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 450);

    // 自动映射绑定
    shader.setAutoMapBindings(true);
    shader.setAutoMapLocations(true);

    // 解析
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    const TBuiltInResource* resources = GetDefaultResources();
    if (!shader.parse(resources, 100, false, messages)) {
        LOG_ERROR("ShaderFactory", "GLSL parse error ({0}): {1}",
                  desc.filename.empty() ? "(inline)" : desc.filename,
                  shader.getInfoLog());
        return {};
    }

    // 链接
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        LOG_ERROR("ShaderFactory", "GLSL link error ({0}): {1}",
                  desc.filename.empty() ? "(inline)" : desc.filename,
                  program.getInfoLog());
        return {};
    }

    // SPIR-V 生成
    glslang::TIntermediate* intermediate = program.getIntermediate(stage);
    if (!intermediate) { return {}; }

    std::vector<unsigned int> spv;
    spv::SpvBuildLogger logger;
    glslang::GlslangToSpv(*intermediate, spv, &logger);

    if (spv.empty()) {
        LOG_ERROR("ShaderFactory", "SPIR-V generation returned empty output");
        return {};
    }

    std::vector<uint8_t> spirv;
    spirv.resize(spv.size() * sizeof(unsigned int));
    memcpy(spirv.data(), spv.data(), spv.size() * sizeof(unsigned int));

    LOG_INFO("ShaderFactory", "Compiled GLSL→SPIR-V ({0} bytes) for {1}",
             spirv.size(), desc.filename.empty() ? "(inline)" : desc.filename);
    return spirv;
}

#else // !PRISMA_HAS_GLSLANG

static std::vector<uint8_t> CompileGLSL(const ShaderDesc& desc) {
    (void)desc;
    LOG_ERROR("ShaderFactory",
              "GLSL→SPIR-V compilation unavailable: rebuild with VULKAN_SDK "
              "containing glslang (find_package(glslang))");
    return {};
}

#endif // PRISMA_HAS_GLSLANG

// ============================================================
// ShaderFactory Implementation
// ============================================================

std::shared_ptr<IShader> ShaderFactory::CreateShader(IRenderDevice* device, const ShaderDesc& desc) {
    if (!device) {
        LOG_ERROR("ShaderFactory", "CreateShader: device is null");
        return nullptr;
    }

    auto* factory = device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("ShaderFactory", "CreateShader: resource factory is null");
        return nullptr;
    }

    std::vector<uint8_t> bytecode;
    ShaderReflection reflection;

    // GLSL → SPIR-V 实时编译
    if (desc.language == ShaderLanguage::GLSL && !desc.source.empty()) {
        bytecode = CompileGLSL(desc);
        if (bytecode.empty()) {
            LOG_ERROR("ShaderFactory",
                      "CreateShader: GLSL→SPIRV failed for {0}",
                      desc.filename.empty() ? "(inline)" : desc.filename);
            return nullptr;
        }
        // 反射 SPIR-V 提取描述符集、Push Constant 信息
        SpirvReflector::Reflect(bytecode, reflection);
    }
    // HLSL / pre-compiled bytecode: passed through as-is

    return factory->CreateShaderImpl(desc, bytecode, reflection);
}

std::shared_ptr<IShader> ShaderFactory::CreateShaderFromFile(IRenderDevice* device,
                                                              const std::string& path,
                                                              ShaderType type) {
    ShaderDesc desc;
    desc.filename = path;
    desc.type = type;

    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) {
        LOG_ERROR("ShaderFactory", "CreateShaderFromFile: cannot open {0}", path);
        return nullptr;
    }
    auto sz = ifs.tellg();
    if (sz <= 0) {
        LOG_ERROR("ShaderFactory", "CreateShaderFromFile: empty file {0}", path);
        return nullptr;
    }
    desc.source.resize(static_cast<size_t>(sz));
    ifs.seekg(0);
    ifs.read(desc.source.data(), static_cast<std::streamsize>(sz));

    // Detect language from file extension
    auto ext = std::filesystem::path(path).extension().string();
    if (ext == ".hlsl" || ext == ".fx") {
        desc.language = ShaderLanguage::HLSL;
    } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp") {
        desc.language = ShaderLanguage::GLSL;
    } else if (ext == ".spv") {
        desc.language = ShaderLanguage::SPIRV;
    }

    return CreateShader(device, desc);
}

} // namespace Prisma::Graphic
