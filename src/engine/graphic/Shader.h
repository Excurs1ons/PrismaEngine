#pragma once

#include "Asset.h"
#include "ISubSystem.h"
#include "interfaces/IShader.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Prisma::Graphic {

/**
 * @brief 现代着色器资产 (Shader Asset)
 */
class ENGINE_API Shader : public Prisma::Asset, public IShader {
public:
    Shader() = default;
    ~Shader() override = default;

    // Asset 接口
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override { return m_IsLoaded; }
    Prisma::AssetType GetType() const override { return Prisma::AssetType::Shader; }

    ResourceType GetResourceType() const override { return ResourceType::Shader; }

    const std::string& GetName() const override { return Prisma::Asset::GetName(); }
    void SetName(const std::string& name) override { Prisma::Asset::SetName(name); }
    ResourceId GetId() const override { return 0; }

    // IShader 接口实现
    virtual ShaderType GetShaderType() const override { return m_ShaderType; }
    virtual ShaderLanguage GetLanguage() const override { return ShaderLanguage::SPIRV; }
    virtual const std::string& GetEntryPoint() const override;
    virtual const std::string& GetTarget() const override;
    virtual const std::string& GetSource() const override;
    virtual const std::vector<uint8_t>& GetBytecode() const override { return m_BytecodeBytes; }
    virtual const std::string& GetFilename() const override { return m_FilenameCache; }
    virtual uint64_t GetCompileTimestamp() const override { return 0; }
    virtual uint64_t GetCompileHash() const override { return 0; }
    virtual const ShaderCompileOptions& GetCompileOptions() const override;
    virtual const ShaderReflection& GetReflection() const override { return m_Reflection; }
    virtual bool HasReflection() const override { return !m_Reflection.Resources.empty(); }
    virtual const ShaderResource* FindResource(const std::string& name) const override;
    virtual const ShaderResource* FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const override;
    
    virtual bool Recompile([[maybe_unused]] const ShaderCompileOptions* options, [[maybe_unused]] std::string& errors) override { return false; }
    virtual bool RecompileFromSource([[maybe_unused]] const std::string& source, [[maybe_unused]] const ShaderCompileOptions* options, [[maybe_unused]] std::string& errors) override { return false; }
    virtual bool ReloadFromFile([[maybe_unused]] std::string& errors) override { return false; }
    virtual void EnableHotReload([[maybe_unused]] bool enable) override {}
    virtual bool IsFileModified() const override { return false; }
    virtual bool NeedsReload() const override { return false; }
    virtual uint64_t GetFileModificationTime() const override { return 0; }
    virtual const std::string& GetCompileLog() const override { static const std::string empty = ""; return empty; }
    virtual bool HasWarnings() const override { return false; }
    virtual bool HasErrors() const override { return false; }
    virtual bool Validate() override { return m_IsLoaded; }
    virtual std::string Disassemble() const override { return ""; }
    virtual bool DebugSaveToFile([[maybe_unused]] const std::string& filename, [[maybe_unused]] bool includeDisassembly, [[maybe_unused]] bool includeReflection) const override { return false; }
    virtual const std::vector<std::string>& GetDependencies() const override { static const std::vector<std::string> empty = {}; return empty; }
    virtual const std::vector<std::string>& GetIncludes() const override { static const std::vector<std::string> empty = {}; return empty; }
    virtual const std::vector<std::string>& GetDefines() const override { static const std::vector<std::string> empty = {}; return empty; }

    const std::vector<uint32_t>& GetBytecodeRaw() const { return m_Bytecode; }

    void SetRHIResource(std::shared_ptr<IShader> rhiResource) { m_RHIResource = rhiResource; }
    std::shared_ptr<IShader> GetRHIResource() const { return m_RHIResource; }

private:
    std::vector<uint32_t> m_Bytecode;
    std::vector<uint8_t> m_BytecodeBytes;
    ShaderReflection m_Reflection;
    ShaderType m_ShaderType = ShaderType::Vertex;
    
    std::string m_FilenameCache;
    
    std::shared_ptr<IShader> m_RHIResource;
    std::unordered_map<std::string, uint32_t> m_ResourceMap;
};

class ShaderLibrary : public Prisma::ISubSystem {
public:
    int Initialize() override { return 0; }
    void Shutdown() override { m_Shaders.clear(); }
    void Update(Prisma::Timestep ts) override;
    const char* GetName() const override { return "ShaderLibrary"; }

    std::shared_ptr<Shader> Load(const std::string& name, const std::filesystem::path& path);
    std::shared_ptr<Shader> Get(const std::string& name);

private:
    float m_accumulatedTime = 0.0f;
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_lastWriteTimes;
};

} // namespace Prisma::Graphic
