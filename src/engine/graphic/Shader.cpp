#include "Shader.h"
#include "DefaultShader.h"
#include "StringUtils.h"
#include "../Logger.h"
#include <filesystem>
#include <fstream>
#include <iostream>

// 平台特定的着色器头文件
#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
#include "adapters/dx12/DX12Shader.h"
#include "adapters/dx12/DX12RenderDevice.h"
#include <d3dcompiler.h>
using Microsoft::WRL::ComPtr;
#endif

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
#include "adapters/vulkan/VulkanShader.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#endif

using namespace Engine;
using namespace StringUtils;

namespace {
    const std::string EXT_CompiledShaderObject = "cso";
    const std::string EXT_HLSL = "hlsl";
    const std::string EXT_SPIRV = "spv";
}

Shader::Shader() {
    // 创建平台特定的着色器实现
    m_impl = CreatePlatformShader();
}

Shader::Shader(std::shared_ptr<PrismaEngine::Graphic::IShader> impl)
    : m_impl(std::move(impl)) {
}

Shader::~Shader() {
    Unload();
}

bool Shader::Load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("Shader", "着色器文件不存在: {0}", path.string());
        return false;
    }

    return LoadShaderFromFile(path);
}

void Shader::Unload() {
    if (m_impl) {
        m_impl.reset();
    }
}

bool Shader::IsLoaded() const {
    return m_impl != nullptr;
}

ResourceType Shader::GetType() const {
    return ResourceType::Shader;
}

// 着色器特定方法
PrismaEngine::Graphic::ShaderType Shader::GetType() const {
    return m_impl ? m_impl->GetShaderType() : PrismaEngine::Graphic::ShaderType::Vertex;
}

PrismaEngine::Graphic::ShaderLanguage Shader::GetLanguage() const {
    return m_impl ? m_impl->GetLanguage() : PrismaEngine::Graphic::ShaderLanguage::HLSL;
}

const std::string& Shader::GetEntryPoint() const {
    static std::string empty;
    return m_impl ? m_impl->GetEntryPoint() : empty;
}

const std::string& Shader::GetTarget() const {
    static std::string empty;
    return m_impl ? m_impl->GetTarget() : empty;
}

const std::vector<uint8_t>& Shader::GetBytecode() const {
    static std::vector<uint8_t> empty;
    return m_impl ? m_impl->GetBytecode() : empty;
}

const std::string& Shader::GetFilename() const {
    static std::string empty;
    return m_impl ? m_impl->GetFilename() : empty;
}

// 反射信息
const PrismaEngine::Graphic::ShaderReflection& Shader::GetReflection() const {
    static PrismaEngine::Graphic::ShaderReflection empty;
    return m_impl ? m_impl->GetReflection() : empty;
}

bool Shader::HasReflection() const {
    return m_impl ? m_impl->HasReflection() : false;
}

// 资源查找
const PrismaEngine::Graphic::ShaderReflection::Resource* Shader::FindResource(const std::string& name) const {
    return m_impl ? m_impl->FindResource(name) : nullptr;
}

const PrismaEngine::Graphic::ShaderReflection::Resource* Shader::FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const {
    return m_impl ? m_impl->FindResourceByBindPoint(bindPoint, space) : nullptr;
}

const PrismaEngine::Graphic::ShaderReflection::ConstantBuffer* Shader::FindConstantBuffer(const std::string& name) const {
    return m_impl ? m_impl->FindConstantBuffer(name) : nullptr;
}

// 输入/输出参数
uint32_t Shader::GetInputParameterCount() const {
    return m_impl ? m_impl->GetInputParameterCount() : 0;
}

const PrismaEngine::Graphic::ShaderReflection::InputParameter& Shader::GetInputParameter(uint32_t index) const {
    static PrismaEngine::Graphic::ShaderReflection::InputParameter empty;
    return m_impl ? m_impl->GetInputParameter(index) : empty;
}

uint32_t Shader::GetOutputParameterCount() const {
    return m_impl ? m_impl->GetOutputParameterCount() : 0;
}

const PrismaEngine::Graphic::ShaderReflection::OutputParameter& Shader::GetOutputParameter(uint32_t index) const {
    static PrismaEngine::Graphic::ShaderReflection::OutputParameter empty;
    return m_impl ? m_impl->GetOutputParameter(index) : empty;
}

// 重新编译
bool Shader::Recompile(const PrismaEngine::Graphic::ShaderCompileOptions* options, std::string* errors) {
    if (!m_impl) {
        if (errors) *errors = "Shader not loaded";
        return false;
    }

    std::string err;
    bool result = m_impl->Recompile(options, err);
    if (errors) *errors = err;
    return result;
}

bool Shader::RecompileFromSource(const std::string& source,
                                const PrismaEngine::Graphic::ShaderCompileOptions* options,
                                std::string* errors) {
    if (!m_impl) {
        if (errors) *errors = "Shader not loaded";
        return false;
    }

    std::string err;
    bool result = m_impl->RecompileFromSource(source, options, err);
    if (errors) *errors = err;
    return result;
}

bool Shader::ReloadFromFile(std::string* errors) {
    if (!m_impl) {
        if (errors) *errors = "Shader not loaded";
        return false;
    }

    std::string err;
    bool result = m_impl->ReloadFromFile(err);
    if (errors) *errors = err;
    return result;
}

// 热重载
void Shader::EnableHotReload(bool enable) {
    if (m_impl) {
        m_impl->EnableHotReload(enable);
    }
}

bool Shader::NeedsReload() const {
    return m_impl ? m_impl->NeedsReload() : false;
}

uint64_t Shader::GetFileModificationTime() const {
    return m_impl ? m_impl->GetFileModificationTime() : 0;
}

// 编译信息
const std::string& Shader::GetCompileLog() const {
    static std::string empty;
    return m_impl ? m_impl->GetCompileLog() : empty;
}

bool Shader::HasWarnings() const {
    return m_impl ? m_impl->HasWarnings() : false;
}

bool Shader::HasErrors() const {
    return m_impl ? m_impl->HasErrors() : false;
}

bool Shader::Validate() const {
    return m_impl ? m_impl->Validate() : false;
}

std::string Shader::Disassemble() const {
    return m_impl ? m_impl->Disassemble() : "";
}

// 调试
bool Shader::DebugSaveToFile(const std::string& filename,
                            bool includeDisassembly,
                            bool includeReflection) const {
    return m_impl ? m_impl->DebugSaveToFile(filename, includeDisassembly, includeReflection) : false;
}

// 依赖信息
const std::vector<std::string>& Shader::GetDependencies() const {
    static std::vector<std::string> empty;
    return m_impl ? m_impl->GetDependencies() : empty;
}

const std::vector<std::string>& Shader::GetIncludes() const {
    static std::vector<std::string> empty;
    return m_impl ? m_impl->GetIncludes() : empty;
}

const std::vector<std::string>& Shader::GetDefines() const {
    static std::vector<std::string> empty;
    return m_impl ? m_impl->GetDefines() : empty;
}

// 尝试加载着色器，失败时回退到默认着色器
bool Shader::LoadWithFallback(const std::filesystem::path& path) {
    if (Load(path)) {
        return true;
    }

    // 加载默认着色器
    LOG_WARN("Shader", "无法加载着色器 {0}，使用默认着色器", path.string());
    return LoadDefaultShader();
}

#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
// DX12特定的兼容方法
bool Shader::CompileFromString(const char* vsSource, const char* psSource) {
    // TODO: 实现从字符串编译DX12着色器
    LOG_ERROR("Shader", "CompileFromString not yet implemented for DX12");
    return false;
}

const std::string& Shader::GetModel() const {
    static std::string model = "ps_5_0";
    return model;
}

void Shader::SetModel(const std::string& model) {
    // TODO: 实现设置编译目标
}

void Shader::SetEntryPoint(const std::string& entryPoint) {
    // TODO: 实现设置入口点
}
#endif

// 获取原生句柄（用于平台特定操作）
void* Shader::GetNativeHandle() const {
    if (!m_impl) {
        return nullptr;
    }

#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
    auto dx12Shader = std::dynamic_pointer_cast<PrismaEngine::Graphic::DX12::DX12Shader>(m_impl);
    if (dx12Shader) {
        return dx12Shader->GetBytecodeData();
    }
#endif

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    auto vulkanShader = std::dynamic_pointer_cast<PrismaEngine::Graphic::Vulkan::VulkanShader>(m_impl);
    if (vulkanShader) {
        return reinterpret_cast<void*>(vulkanShader->GetShaderModule());
    }
#endif

    return nullptr;
}

// 内部辅助方法
bool Shader::LoadShaderFromFile(const std::filesystem::path& path) {
    // 检查文件扩展名
    auto ext = ToLower(path.extension().string());

#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
    if (Equals(ext, EXT_CompiledShaderObject) || Equals(ext, EXT_HLSL)) {
        // 加载DX12着色器
        return LoadDX12Shader(path);
    }
#endif

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    if (Equals(ext, EXT_SPIRV)) {
        // 加载Vulkan着色器
        return LoadVulkanShader(path);
    }
#endif

    LOG_ERROR("Shader", "不支持的着色器文件格式: {0}", ext);
    return false;
}

#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
bool Shader::LoadDX12Shader(const std::filesystem::path& path) {
    // TODO: 实现DX12着色器加载
    // 这里需要创建DX12Shader实例
    LOG_ERROR("Shader", "DX12 shader loading not yet implemented");
    return false;
}
#endif

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
bool Shader::LoadVulkanShader(const std::filesystem::path& path) {
    // TODO: 实现Vulkan着色器加载
    // 这里需要创建VulkanShader实例
    LOG_ERROR("Shader", "Vulkan shader loading not yet implemented");
    return false;
}
#endif

std::shared_ptr<PrismaEngine::Graphic::IShader> Shader::CreatePlatformShader() {
    // 根据编译时宏创建平台特定的着色器实现
#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
    // 创建DX12着色器
    // TODO: 需要DX12RenderDevice实例
    // return std::make_shared<PrismaEngine::Graphic::DX12::DX12Shader>(...);
#endif

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    // 创建Vulkan着色器
    // TODO: 需要VulkanRenderDevice实例
    // return std::make_shared<PrismaEngine::Graphic::Vulkan::VulkanShader>(...);
#endif

    LOG_ERROR("Shader", "No suitable render backend available for shader creation");
    return nullptr;
}

// 工厂函数
std::shared_ptr<Shader> CreateShader(const std::filesystem::path& path) {
    auto shader = std::make_shared<Shader>();
    if (shader->Load(path)) {
        return shader;
    }
    return nullptr;
}

std::shared_ptr<Shader> CreateShader(const std::string& vertexSource, const std::string& pixelSource) {
    auto shader = std::make_shared<Shader>();
#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
    if (shader->CompileFromString(vertexSource.c_str(), pixelSource.c_str())) {
        return shader;
    }
#endif
    LOG_ERROR("Shader", "Failed to create shader from source strings");
    return nullptr;
}

bool Shader::LoadDefaultShader() {
    // 加载默认的着色器
    auto defaultShader = std::make_shared<DefaultShader>();
    m_impl = defaultShader;
    return true;
}