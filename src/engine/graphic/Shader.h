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
 * 包装了具体的 RHI Shader 资源，同时也作为引擎资产。
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

    // 解决多重继承歧义 (Asset vs IResource)
    const std::string& GetName() const override { return Prisma::Asset::GetName(); }
    void SetName(const std::string& name) override { Prisma::Asset::SetName(name); }
    ResourceId GetId() const override { return 0; } // 或者按需实现

    // IShader 接口实现 (重定向到内部数据或具体实现)
    virtual ShaderType GetShaderType() const override { return m_ShaderType; }
    virtual ShaderLanguage GetLanguage() const override { return ShaderLanguage::SPIRV; }
    virtual const std::string& GetEntryPoint() const override { static std::string main = "main"; return main; }
    virtual const std::string& GetTarget() const override { static std::string empty; return empty; }
    virtual const std::string& GetSource() const override { static std::string empty; return empty; }
    virtual const std::vector<uint8_t>& GetBytecode() const override { return m_BytecodeBytes; }
    virtual const std::string& GetFilename() const override { return GetPath().string(); }
    virtual uint64_t GetCompileTimestamp() const override { return 0; }
    virtual uint64_t GetCompileHash() const override { return 0; }
    virtual const ShaderCompileOptions& GetCompileOptions() const override { static ShaderCompileOptions opt; return opt; }
    virtual const ShaderReflection& GetReflection() const override { return m_Reflection; }
    virtual bool HasReflection() const override { return !m_Reflection.Resources.empty(); }
    virtual const ShaderResource* FindResource(const std::string& name) const override;
    virtual const ShaderResource* FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const override;
    
    virtual bool Recompile(const ShaderCompileOptions* options, std::string& errors) override { return false; }
    virtual bool RecompileFromSource(const std::string& source, const ShaderCompileOptions* options, std::string& errors) override { return false; }
    virtual bool ReloadFromFile(std::string& errors) override { return false; }
    virtual void EnableHotReload(bool enable) override {}
    virtual bool IsFileModified() const override { return false; }
    virtual bool NeedsReload() const override { return false; }
    virtual uint64_t GetFileModificationTime() const override { return 0; }
    virtual const std::string& GetCompileLog() const override { static std::string empty; return empty; }
    virtual bool HasWarnings() const override { return false; }
    virtual bool HasErrors() const override { return false; }
    virtual bool Validate() override { return m_IsLoaded; }
    virtual std::string Disassemble() const override { return ""; }
    virtual bool DebugSaveToFile(const std::string& filename, bool includeDisassembly, bool includeReflection) const override { return false; }
    virtual const std::vector<std::string>& GetDependencies() const override { static std::vector<std::string> empty; return empty; }
    virtual const std::vector<std::string>& GetIncludes() const override { static std::vector<std::string> empty; return empty; }
    virtual const std::vector<std::string>& GetDefines() const override { static std::vector<std::string> empty; return empty; }

    // 原始访问器 (保留兼容性)
    const std::vector<uint32_t>& GetBytecodeRaw() const { return m_Bytecode; }

    // RHI 资源管理
    void SetRHIResource(std::shared_ptr<IShader> rhiResource) { m_RHIResource = rhiResource; }
    std::shared_ptr<IShader> GetRHIResource() const { return m_RHIResource; }

private:
    std::vector<uint32_t> m_Bytecode;
    std::vector<uint8_t> m_BytecodeBytes;
    ShaderReflection m_Reflection;
    ShaderType m_ShaderType = ShaderType::Vertex;
    
    std::shared_ptr<IShader> m_RHIResource; // 具体的 VulkanShader/DX12Shader
    std::unordered_map<std::string, uint32_t> m_ResourceMap;
};

/**
 * @brief 着色器管理器 (属于 Engine 的子系统的实例)
 */
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
