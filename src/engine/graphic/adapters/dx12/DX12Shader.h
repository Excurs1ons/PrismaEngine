#pragma once

#include "interfaces/IResourceManager.h"
#include "interfaces/IShader.h"
#include <string>
#include <vector>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;

/// @brief DirectX12着色器适配器
/// 实现IShader接口，包装编译后的着色器字节码
class DX12Shader : public IShader {
public:
    /// @brief 构造函数
    /// @param device DirectX12渲染设备
    /// @param desc 着色器描述
    /// @param bytecode 编译后的字节码
    /// @param reflection 反射信息
    DX12Shader(DX12RenderDevice* device,
               const ShaderDesc& desc,
               const std::vector<uint8_t>& bytecode,
               const ShaderReflection& reflection);

    /// @brief 析构函数
    ~DX12Shader() override;

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
    bool RecompileFromSource(const std::string& source,
                             const ShaderCompileOptions* options,
                             std::string& errors) override;
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

    bool DebugSaveToFile(const std::string& filename,
                        bool includeDisassembly, bool includeReflection) const override;

    const std::vector<std::string>& GetDependencies() const override;
    const std::vector<std::string>& GetIncludes() const override;
    const std::vector<std::string>& GetDefines() const override;

    // === DirectX12特定方法 ===

    /// @brief 获取着色器字节码指针
    /// @return 字节码数据
    const void* GetBytecodeData() const;

    /// @brief 获取字节码大小
    /// @return 字节码大小
    size_t GetBytecodeSize() const;

private:
    DX12RenderDevice* m_device;
    ShaderDesc m_desc;
    std::vector<uint8_t> m_bytecode;
    ShaderReflection m_reflection;
    std::string m_compileLog;
    bool m_hotReloadEnabled = false;
    uint64_t m_fileModificationTime = 0;
};

} // namespace PrismaEngine::Graphic::DX12