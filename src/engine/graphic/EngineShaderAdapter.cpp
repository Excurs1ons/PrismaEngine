#include "EngineShaderAdapter.h"
#include <fstream>

namespace PrismaEngine::Graphic {

EngineShaderAdapter::EngineShaderAdapter(std::shared_ptr<Engine::Shader> engineShader)
    : m_engineShader(std::move(engineShader)) {
}

ShaderType EngineShaderAdapter::GetShaderType() const {
    // Engine::Shader 不区分具体的着色器类型，返回未知类型
    return ShaderType::Unknown;
}

ShaderLanguage EngineShaderAdapter::GetLanguage() const {
    // Engine::Shader 使用 HLSL
    return ShaderLanguage::HLSL;
}

const std::string& EngineShaderAdapter::GetEntryPoint() const {
    // Engine::Shader 使用固定的入口点
    static const std::string entryPoint = "main";
    return entryPoint;
}

const std::string& EngineShaderAdapter::GetTarget() const {
    // Engine::Shader 使用固定的目标
    static const std::string target = "ps_5_0";
    return target;
}

const std::string& EngineShaderAdapter::GetSource() const {
    // Engine::Shader 不直接存储源代码
    return m_emptyString;
}

const std::vector<uint8_t>& EngineShaderAdapter::GetBytecode() const {
    // Engine::Shader 存储的是 ID3DBlob，需要转换为字节数组
    // 由于接口不匹配，暂时返回空数组
    return m_emptyBytecode;
}

const std::string& EngineShaderAdapter::GetFilename() const {
    if (m_engineShader) {
        return m_engineShader->GetPath().string();
    }
    return m_emptyString;
}

uint64_t EngineShaderAdapter::GetCompileTimestamp() const {
    // Engine::Shader 不存储编译时间戳
    return 0;
}

uint64_t EngineShaderAdapter::GetCompileHash() const {
    // Engine::Shader 不存储编译哈希
    return 0;
}

const ShaderCompileOptions& EngineShaderAdapter::GetCompileOptions() const {
    // Engine::Shader 不存储编译选项
    return m_emptyCompileOptions;
}

const ShaderReflection& EngineShaderAdapter::GetReflection() const {
    // Engine::Shader 不提供反射信息
    return m_emptyReflection;
}

bool EngineShaderAdapter::HasReflection() const {
    // Engine::Shader 不提供反射信息
    return false;
}

const ShaderReflection::Resource* EngineShaderAdapter::FindResource(const std::string& name) const {
    // Engine::Shader 不提供反射信息
    return nullptr;
}

const ShaderReflection::Resource* EngineShaderAdapter::FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const {
    // Engine::Shader 不提供反射信息
    return nullptr;
}

const ShaderReflection::ConstantBuffer* EngineShaderAdapter::FindConstantBuffer(const std::string& name) const {
    // Engine::Shader 不提供反射信息
    return nullptr;
}

uint32_t EngineShaderAdapter::GetInputParameterCount() const {
    // Engine::Shader 不提供反射信息
    return 0;
}

const ShaderReflection::InputParameter& EngineShaderAdapter::GetInputParameter(uint32_t index) const {
    // Engine::Shader 不提供反射信息
    static ShaderReflection::InputParameter emptyParam{};
    return emptyParam;
}

uint32_t EngineShaderAdapter::GetOutputParameterCount() const {
    // Engine::Shader 不提供反射信息
    return 0;
}

const ShaderReflection::OutputParameter& EngineShaderAdapter::GetOutputParameter(uint32_t index) const {
    // Engine::Shader 不提供反射信息
    static ShaderReflection::OutputParameter emptyParam{};
    return emptyParam;
}

bool EngineShaderAdapter::Recompile(const ShaderCompileOptions* options, std::string& errors) {
    // Engine::Shader 不支持重新编译
    errors = "Engine::Shader does not support recompilation";
    return false;
}

bool EngineShaderAdapter::RecompileFromSource(const std::string& source, const ShaderCompileOptions* options, std::string& errors) {
    // Engine::Shader 不支持从源码重新编译
    errors = "Engine::Shader does not support recompilation from source";
    return false;
}

bool EngineShaderAdapter::ReloadFromFile(std::string& errors) {
    // Engine::Shader 不支持重新加载
    errors = "Engine::Shader does not support reloading from file";
    return false;
}

void EngineShaderAdapter::EnableHotReload(bool enable) {
    // Engine::Shader 不支持热重载
}

bool EngineShaderAdapter::IsFileModified() const {
    // Engine::Shader 不跟踪文件修改
    return false;
}

bool EngineShaderAdapter::NeedsReload() const {
    // Engine::Shader 不支持重新加载
    return false;
}

uint64_t EngineShaderAdapter::GetFileModificationTime() const {
    // Engine::Shader 不跟踪文件修改时间
    return 0;
}

const std::string& EngineShaderAdapter::GetCompileLog() const {
    // Engine::Shader 不存储编译日志
    return m_emptyString;
}

bool EngineShaderAdapter::HasWarnings() const {
    // Engine::Shader 不提供警告信息
    return false;
}

bool EngineShaderAdapter::HasErrors() const {
    // Engine::Shader 不提供错误信息
    return false;
}

bool EngineShaderAdapter::Validate() {
    // 检查Engine::Shader是否有效
    return m_engineShader && m_engineShader->IsLoaded();
}

std::string EngineShaderAdapter::Disassemble() const {
    // Engine::Shader 不提供反汇编功能
    return "";
}

bool EngineShaderAdapter::DebugSaveToFile(const std::string& filename, bool includeDisassembly, bool includeReflection) const {
    // Engine::Shader 不提供调试保存功能
    return false;
}

const std::vector<std::string>& EngineShaderAdapter::GetDependencies() const {
    // Engine::Shader 不跟踪依赖
    return m_emptyStringVector;
}

const std::vector<std::string>& EngineShaderAdapter::GetIncludes() const {
    // Engine::Shader 不跟踪包含文件
    return m_emptyStringVector;
}

const std::vector<std::string>& EngineShaderAdapter::GetDefines() const {
    // Engine::Shader 不跟踪宏定义
    return m_emptyStringVector;
}

} // namespace PrismaEngine::Graphic