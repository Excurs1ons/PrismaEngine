#pragma once

#include "RenderTypes.h"
#include "IResource.h"
#include "ShaderReflection.h"
#include <string>
#include <vector>

namespace Prisma::Graphic {

// 着色器抽象接口
class IShader : public IResource {
public:
    virtual ~IShader() = default;

    // 获取着色器类型
    virtual ShaderType GetShaderType() const = 0;

    // 获取着色器语言
    virtual ShaderLanguage GetLanguage() const = 0;

    // 获取入口点函数名
    virtual const std::string& GetEntryPoint() const = 0;

    // 获取编译目标
    virtual const std::string& GetTarget() const = 0;

    // 获取着色器源码
    virtual const std::string& GetSource() const = 0;

    // 获取着色器字节码
    virtual const std::vector<uint8_t>& GetBytecode() const = 0;

    // 获取着色器文件名
    virtual const std::string& GetFilename() const = 0;

    // 获取编译时间戳
    virtual uint64_t GetCompileTimestamp() const = 0;

    // 获取编译哈希值
    virtual uint64_t GetCompileHash() const = 0;

    // 获取编译选项
    virtual const ShaderCompileOptions& GetCompileOptions() const = 0;

    // 获取着色器反射信息
    virtual const ShaderReflection& GetReflection() const = 0;

    // 是否包含反射信息
    virtual bool HasReflection() const = 0;

    // 查找资源信息
    virtual const ShaderResource* FindResource(const std::string& name) const = 0;

    // 根据绑定点查找资源
    virtual const ShaderResource* FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const = 0;

    // 重新编译着色器
    virtual bool Recompile(const ShaderCompileOptions* options, std::string& errors) = 0;

    // 从源码重新编译
    virtual bool RecompileFromSource(const std::string& source, const ShaderCompileOptions* options, std::string& errors) = 0;

    // 从文件重新加载并编译
    virtual bool ReloadFromFile(std::string& errors) = 0;

    // 启用/禁用热重载
    virtual void EnableHotReload(bool enable) = 0;

    // 检查源文件是否被修改
    virtual bool IsFileModified() const = 0;

    // 是否需要重新加载（修改后未编译）
    virtual bool NeedsReload() const = 0;

    // 获取文件最后修改时间
    virtual uint64_t GetFileModificationTime() const = 0;

    // 获取编译日志
    virtual const std::string& GetCompileLog() const = 0;

    // 是否有编译警告
    virtual bool HasWarnings() const = 0;

    // 是否有编译错误
    virtual bool HasErrors() const = 0;

    // 验证着色器是否有效
    virtual bool Validate() = 0;

    // 反汇编着色器
    virtual std::string Disassemble() const = 0;

    // 保存调试信息到文件
    virtual bool DebugSaveToFile(const std::string& filename, bool includeDisassembly, bool includeReflection) const = 0;

    // 获取依赖文件列表
    virtual const std::vector<std::string>& GetDependencies() const = 0;

    // 获取包含文件列表
    virtual const std::vector<std::string>& GetIncludes() const = 0;

    // 获取宏定义列表
    virtual const std::vector<std::string>& GetDefines() const = 0;
};

} // namespace Prisma::Graphic
