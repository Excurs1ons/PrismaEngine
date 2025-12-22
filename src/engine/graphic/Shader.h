#pragma once
#include "ResourceManager.h"
#include "interfaces/RenderTypes.h"
#include "interfaces/IShader.h"
#include <memory>
#include <string>
#include <filesystem>

// 前置声明特定平台的着色器类
namespace PrismaEngine::Graphic::DX12 {
    class DX12Shader;
}

namespace PrismaEngine::Graphic::Vulkan {
    class VulkanShader;
}

namespace Engine {
class RenderDevice;  // 前置声明

/// @brief 着色器资源类
/// 根据编译时的宏定义自动选择使用DX12Shader或VulkanShader
class Shader : public ResourceBase {
public:
    Shader();
    Shader(std::shared_ptr<PrismaEngine::Graphic::IShader> impl);
    ~Shader();

    // IResource implementation
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override;
    ResourceType GetType() const override;

    // 着色器特定方法
    PrismaEngine::Graphic::ShaderType GetShaderType() const;
    PrismaEngine::Graphic::ShaderLanguage GetLanguage() const;
    const std::string& GetEntryPoint() const;
    const std::string& GetTarget() const;
    const std::vector<uint8_t>& GetBytecode() const;
    const std::string& GetFilename() const;

    // 反射信息
    const PrismaEngine::Graphic::ShaderReflection& GetReflection() const;
    bool HasReflection() const;

    // 资源查找
    const PrismaEngine::Graphic::ShaderReflection::Resource* FindResource(const std::string& name) const;
    const PrismaEngine::Graphic::ShaderReflection::Resource* FindResourceByBindPoint(uint32_t bindPoint, uint32_t space = 0) const;
    const PrismaEngine::Graphic::ShaderReflection::ConstantBuffer* FindConstantBuffer(const std::string& name) const;

    // 输入/输出参数
    uint32_t GetInputParameterCount() const;
    const PrismaEngine::Graphic::ShaderReflection::InputParameter& GetInputParameter(uint32_t index) const;
    uint32_t GetOutputParameterCount() const;
    const PrismaEngine::Graphic::ShaderReflection::OutputParameter& GetOutputParameter(uint32_t index) const;

    // 重新编译
    bool Recompile(const PrismaEngine::Graphic::ShaderCompileOptions* options = nullptr, std::string* errors = nullptr);
    bool RecompileFromSource(const std::string& source,
                            const PrismaEngine::Graphic::ShaderCompileOptions* options = nullptr,
                            std::string* errors = nullptr);
    bool ReloadFromFile(std::string* errors = nullptr);

    // 热重载
    void EnableHotReload(bool enable);
    bool NeedsReload() const;
    uint64_t GetFileModificationTime() const;

    // 编译信息
    const std::string& GetCompileLog() const;
    bool HasWarnings() const;
    bool HasErrors() const;
    bool Validate() const;
    std::string Disassemble() const;

    // 调试
    bool DebugSaveToFile(const std::string& filename,
                        bool includeDisassembly = false,
                        bool includeReflection = false) const;

    // 依赖信息
    const std::vector<std::string>& GetDependencies() const;
    const std::vector<std::string>& GetIncludes() const;
    const std::vector<std::string>& GetDefines() const;

    // 尝试加载着色器，失败时回退到默认着色器
    bool LoadWithFallback(const std::filesystem::path& path);

    // 兼容旧接口的方法（在DX12时使用）
#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
    // 这些方法仅在DX12时可用
    bool CompileFromString(const char* vsSource, const char* psSource);
    const std::string& GetModel() const;
    void SetModel(const std::string& model);
    void SetEntryPoint(const std::string& entryPoint);
#endif

    // 获取平台特定的着色器实现
    std::shared_ptr<PrismaEngine::Graphic::IShader> GetImplementation() const { return m_impl; }

    // 获取原生句柄（用于平台特定操作）
    const void* GetNativeHandle() const;

private:
    std::shared_ptr<PrismaEngine::Graphic::IShader> m_impl;

    // 内部辅助方法
    bool LoadShaderFromFile(const std::filesystem::path& path);
    std::shared_ptr<PrismaEngine::Graphic::IShader> CreatePlatformShader();
    bool LoadDefaultShader();

    // 平台特定的加载方法
#if defined(PRISMA_ENABLE_RENDER_DX12) || (defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_FORCE_GLM))
    bool LoadDX12Shader(const std::filesystem::path& path);
#endif

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    bool LoadVulkanShader(const std::filesystem::path& path);
#endif
};

// 工厂函数
std::shared_ptr<Shader> CreateShader(const std::filesystem::path& path);
std::shared_ptr<Shader> CreateShader(const std::string& vertexSource, const std::string& pixelSource);

}  // namespace Engine