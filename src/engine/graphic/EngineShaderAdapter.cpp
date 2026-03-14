#include "EngineShaderAdapter.h"
#include "Logger.h"

namespace Prisma::Graphic {

EngineShaderAdapter::EngineShaderAdapter(std::shared_ptr<Shader> engineShader)
    : m_engineShader(std::move(engineShader)) {
}

ShaderType EngineShaderAdapter::GetShaderType() const {
    return ShaderType::Unknown;
}

ShaderLanguage EngineShaderAdapter::GetLanguage() const {
    return ShaderLanguage::SPIRV;
}

const std::string& EngineShaderAdapter::GetEntryPoint() const {
    return m_emptyString;
}

const std::string& EngineShaderAdapter::GetTarget() const {
    return m_emptyString;
}

const std::string& EngineShaderAdapter::GetSource() const {
    return m_emptyString;
}

const std::vector<uint8_t>& EngineShaderAdapter::GetBytecode() const {
    if (m_engineShader) {
        // This is a bit of a hack, but SPIR-V bytecode is usually what we have
        return reinterpret_cast<const std::vector<uint8_t>&>(m_engineShader->GetBytecode());
    }
    return m_emptyBytecode;
}

const std::string& EngineShaderAdapter::GetFilename() const {
    return m_emptyString;
}

uint64_t EngineShaderAdapter::GetCompileTimestamp() const {
    return 0;
}

uint64_t EngineShaderAdapter::GetCompileHash() const {
    return 0;
}

const ShaderCompileOptions& EngineShaderAdapter::GetCompileOptions() const {
    return m_emptyCompileOptions;
}

const ShaderReflection& EngineShaderAdapter::GetReflection() const {
    if (m_engineShader) {
        return m_engineShader->GetReflection();
    }
    return m_emptyReflection;
}

bool EngineShaderAdapter::HasReflection() const {
    return m_engineShader != nullptr;
}

const ShaderResource* EngineShaderAdapter::FindResource(const std::string& name) const {
    if (m_engineShader) {
        return m_engineShader->FindResource(name);
    }
    return nullptr;
}

const ShaderResource* EngineShaderAdapter::FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const {
    // Not implemented in engine shader yet
    return nullptr;
}

bool EngineShaderAdapter::Recompile(const ShaderCompileOptions* options, std::string& errors) {
    return false;
}

bool EngineShaderAdapter::RecompileFromSource(const std::string& source, const ShaderCompileOptions* options, std::string& errors) {
    return false;
}

bool EngineShaderAdapter::ReloadFromFile(std::string& errors) {
    return false;
}

void EngineShaderAdapter::EnableHotReload(bool enable) {
}

bool EngineShaderAdapter::IsFileModified() const {
    return false;
}

bool EngineShaderAdapter::NeedsReload() const {
    return false;
}

uint64_t EngineShaderAdapter::GetFileModificationTime() const {
    return 0;
}

const std::string& EngineShaderAdapter::GetCompileLog() const {
    return m_emptyString;
}

bool EngineShaderAdapter::HasWarnings() const {
    return false;
}

bool EngineShaderAdapter::HasErrors() const {
    return false;
}

bool EngineShaderAdapter::Validate() {
    return true;
}

std::string EngineShaderAdapter::Disassemble() const {
    return "";
}

bool EngineShaderAdapter::DebugSaveToFile(const std::string& filename, bool includeDisassembly, bool includeReflection) const {
    return false;
}

const std::vector<std::string>& EngineShaderAdapter::GetDependencies() const {
    return m_emptyStringVector;
}

const std::vector<std::string>& EngineShaderAdapter::GetIncludes() const {
    return m_emptyStringVector;
}

const std::vector<std::string>& EngineShaderAdapter::GetDefines() const {
    return m_emptyStringVector;
}

} // namespace Prisma::Graphic
