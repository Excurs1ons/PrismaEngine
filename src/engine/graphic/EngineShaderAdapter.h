#pragma once

#include "interfaces/IShader.h"
#include "Shader.h"
#include <memory>

namespace PrismaEngine::Graphic {

class EngineShaderAdapter : public IShader {
public:
    explicit EngineShaderAdapter(std::shared_ptr<PrismaEngine::Shader> engineShader);
    
    // IShader接口实现
    ShaderType GetShaderType() const override;
    ShaderLanguage GetLanguage() const override;
    const std::string& GetEntryPoint() const override;
    const std::string& GetTarget() const override;
    const std::string& GetSource() const override;
    const std::vector<uint8_t>& GetBytecode() const override;
    const std::string& GetFilename() const override;
    uint64_t GetCompileTimestamp() const override;
    uint64_t GetCompileHash() const override;
    const ShaderCompileOptions& GetCompileOptions() const override;
    const ShaderReflection& GetReflection() const override;
    bool HasReflection() const override;
    const ShaderReflection::Resource* FindResource(const std::string& name) const override;
    const ShaderReflection::Resource* FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const override;
    const ShaderReflection::ConstantBuffer* FindConstantBuffer(const std::string& name) const override;
    uint32_t GetInputParameterCount() const override;
    const ShaderReflection::InputParameter& GetInputParameter(uint32_t index) const override;
    uint32_t GetOutputParameterCount() const override;
    const ShaderReflection::OutputParameter& GetOutputParameter(uint32_t index) const override;
    bool Recompile(const ShaderCompileOptions* options, std::string& errors) override;
    bool RecompileFromSource(const std::string& source, const ShaderCompileOptions* options, std::string& errors) override;
    bool ReloadFromFile(std::string& errors) override;
    void EnableHotReload(bool enable) override;
    bool IsFileModified() const override;
    bool NeedsReload() const override;
    uint64_t GetFileModificationTime() const override;
    const std::string& GetCompileLog() const override;
    bool HasWarnings() const override;
    bool HasErrors() const override;
    bool Validate() override;
    std::string Disassemble() const override;
    bool DebugSaveToFile(const std::string& filename, bool includeDisassembly, bool includeReflection) const override;
    const std::vector<std::string>& GetDependencies() const override;
    const std::vector<std::string>& GetIncludes() const override;
    const std::vector<std::string>& GetDefines() const override;

private:
    std::shared_ptr<PrismaEngine::Shader> m_engineShader;
    std::string m_emptyString;
    std::vector<uint8_t> m_emptyBytecode;
    ShaderReflection m_emptyReflection;
    ShaderCompileOptions m_emptyCompileOptions;
    std::vector<std::string> m_emptyStringVector;
};

} // namespace PrismaEngine::Graphic