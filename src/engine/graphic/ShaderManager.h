#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>

// 添加对新接口的引用
#include "interfaces/RenderTypes.h"
#include "interfaces/IShader.h"  // 添加新的IShader接口

namespace Engine {
namespace Graphic {

// 前向声明
class IShaderProgram;

// 着色器类型
enum class ShaderType : uint32_t {
    Vertex = 0,
    Pixel,
    Geometry,
    Compute,
    Hull,
    Domain,
    Count
};

// 着色器阶段
enum class ShaderStage : uint32_t {
    VS = 0,  // Vertex Shader
    PS,      // Pixel Shader
    GS,      // Geometry Shader
    CS,      // Compute Shader
    HS,      // Hull Shader (Tessellation)
    DS,      // Domain Shader (Tessellation)
    Count
};

// 着色器宏定义
struct ShaderMacro {
    std::string name;
    std::string value;

    ShaderMacro(const std::string& n, const std::string& v)
        : name(n), value(v) {}
};

// 着色器描述
struct ShaderDesc {
    std::string filePath;
    std::string entryPoint;
    PrismaEngine::Graphic::ShaderType type;  // 使用新的ShaderType
    std::vector<ShaderMacro> macros;

    ShaderDesc() : type(PrismaEngine::Graphic::ShaderType::Vertex), entryPoint("main") {}
};

// 着色器程序（Pipeline State Object）
class IShaderProgram {
public:
    virtual ~IShaderProgram() = default;

    // 设置着色器
    virtual void SetShader(ShaderStage stage, std::shared_ptr<PrismaEngine::Graphic::IShader> shader) = 0;

    // 获取着色器
    virtual std::shared_ptr<PrismaEngine::Graphic::IShader> GetShader(ShaderStage stage) const = 0;

    // 链接程序
    virtual bool Link() = 0;

    // 使用程序
    virtual void Bind() = 0;

    // 取消绑定
    virtual void Unbind() = 0;

    // 设置常量缓冲区
    virtual void SetConstantBuffer(const std::string& name, void* buffer, uint32_t size) = 0;

    // 设置纹理
    virtual void SetTexture(const std::string& name, void* texture) = 0;

    // 设置采样器
    virtual void SetSampler(const std::string& name, void* sampler) = 0;

    // 获取常量缓冲区位置
    virtual int GetConstantBufferLocation(const std::string& name) const = 0;

    // 获取纹理位置
    virtual int GetTextureLocation(const std::string& name) const = 0;

    // 获取采样器位置
    virtual int GetSamplerLocation(const std::string& name) const = 0;

    // 检查是否有效
    virtual bool IsValid() const = 0;
};

// 着色色管理器
class ShaderManager {
public:
    static ShaderManager& GetInstance();

    // 加载着色器
    std::shared_ptr<PrismaEngine::Graphic::IShader> LoadShader(const ShaderDesc& desc);

    // 创建着色器程序
    std::shared_ptr<IShaderProgram> CreateShaderProgram();

    // 获取已加载的着色器
    std::shared_ptr<PrismaEngine::Graphic::IShader> GetShader(const std::string& filePath);

    // 重新加载所有着色器
    void ReloadAllShaders();

    // 清理资源
    void Cleanup();

    // 设置着色器搜索路径
    void SetShaderSearchPath(const std::string& path);

    // 预编译所有着色器
    void PrecompileAllShaders(const std::string& shaderDir);

    // 设置当前渲染后端类型
    void SetRenderAPIType(PrismaEngine::Graphic::RenderAPIType type);

    // 获取当前渲染后端类型
    PrismaEngine::Graphic::RenderAPIType GetRenderAPIType() const;

    // 获取着色器缓存统计
    struct ShaderStats {
        uint32_t totalShaders = 0;
        uint32_t compiledShaders = 0;
        uint32_t failedShaders = 0;
        uint32_t totalPrograms = 0;
    };

    ShaderStats GetStats() const;

private:
    ShaderManager() = default;
    ~ShaderManager() = default;
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    // 生成着色器缓存键
    std::string GenerateShaderKey(const ShaderDesc& desc) const;

    // 编译着色器
    std::shared_ptr<PrismaEngine::Graphic::IShader> CompileShader(const ShaderDesc& desc);

    // 线程安全
    mutable std::mutex m_mutex;

    // 着色器缓存
    std::unordered_map<std::string, std::shared_ptr<PrismaEngine::Graphic::IShader>> m_shaders;

    // 着色器程序列表
    std::vector<std::weak_ptr<IShaderProgram>> m_programs;

    // 搜索路径
    std::string m_searchPath = "shaders/";

    // 当前后端类型
    PrismaEngine::Graphic::RenderAPIType m_backendType = PrismaEngine::Graphic::RenderAPIType::DirectX12;

    // 统计信息
    mutable ShaderStats m_stats;
};

// 便利函数
inline ShaderManager& GetShaderManager() {
    return ShaderManager::GetInstance();
}

} // namespace Graphic
} // namespace Engine