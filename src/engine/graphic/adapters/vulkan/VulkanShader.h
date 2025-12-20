#pragma once

#include "interfaces/IShader.h"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace PrismaEngine::Graphic::Vulkan {

// 前置声明
class VulkanRenderDevice;

/// @brief Vulkan着色器适配器
/// 实现IShader接口，包装编译后的Vulkan SPIR-V字节码
class VulkanShader : public IShader {
public:
    /// @brief 构造函数
    /// @param device Vulkan渲染设备
    /// @param desc 着色器描述
    /// @param spirv SPIR-V字节码
    /// @param reflection 反射信息
    VulkanShader(VulkanRenderDevice* device,
                 const ShaderDesc& desc,
                 const std::vector<uint32_t>& spirv,
                 const ShaderReflection& reflection);

    /// @brief 析构函数
    ~VulkanShader() override;

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

    // === Vulkan特定方法 ===

    /// @brief 获取Vulkan着色器模块
    /// @return VkShaderModule句柄
    VkShaderModule GetShaderModule() const { return m_shaderModule; }

    /// @brief 获取SPIR-V字节码
    /// @return SPIR-V字节码
    const std::vector<uint32_t>& GetSpirv() const { return m_spirv; }

    /// @brief 获取Vulkan着色器阶段标志
    /// @return VkShaderStageFlagBits
    VkShaderStageFlagBits GetVkShaderStage() const;


private:
    VulkanRenderDevice* m_device;
    ShaderDesc m_desc;
    std::vector<uint32_t> m_spirv;
    std::vector<uint8_t> m_bytecode;  // 兼容IShader接口
    ShaderReflection m_reflection;
    std::string m_compileLog;
    bool m_hotReloadEnabled = false;
    uint64_t m_fileModificationTime = 0;
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;

    /// @brief 创建Vulkan着色器模块
    /// @return 是否成功
    bool CreateShaderModule();

    /// @brief 销毁着色器模块
    void DestroyShaderModule();
};

} // namespace PrismaEngine::Graphic::Vulkan