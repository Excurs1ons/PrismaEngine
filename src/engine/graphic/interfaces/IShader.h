#pragma once

#include "RenderTypes.h"
#include "IResource.h"
#include "ShaderReflection.h"
#include <string>
#include <vector>

namespace Prisma::Graphic {

/// @brief 着色器抽象接口
class IShader : public IResource {
public:
    virtual ~IShader() = default;

    /// @brief 获取着色器类型
    /// @return 着色器类型
    virtual ShaderType GetShaderType() const = 0;

    /// @brief 获取着色器语言
    /// @return 着色器语言
    virtual ShaderLanguage GetLanguage() const = 0;

    /// @brief 获取入口点函数名
    /// @return 入口点函数名
    virtual const std::string& GetEntryPoint() const = 0;

    /// @brief 获取编译目标
    /// @return 编译目标
    virtual const std::string& GetTarget() const = 0;

    /// @brief 获取着色器源码
    /// @return 着色器源码
    virtual const std::string& GetSource() const = 0;

    /// @brief 获取着色器字节码
    /// @return 字节码数据
    virtual const std::vector<uint8_t>& GetBytecode() const = 0;

    /// @brief 获取着色器文件名
    /// @return 文件名
    virtual const std::string& GetFilename() const = 0;

    /// @brief 获取编译时间戳
    /// @return 编译时间戳
    virtual uint64_t GetCompileTimestamp() const = 0;

    /// @brief 获取编译哈希值
    /// @return 编译哈希值
    virtual uint64_t GetCompileHash() const = 0;

    /// @brief 获取编译选项
    /// @return 编译选项
    virtual const ShaderCompileOptions& GetCompileOptions() const = 0;

    /// @brief 获取着色器反射信息
    /// @return 反射信息
    virtual const ShaderReflection& GetReflection() const = 0;

    /// @brief 是否包含反射信息
    /// @return 是否包含
    virtual bool HasReflection() const = 0;

    /// @brief 查找资源信息
    /// @param name 资源名称
    /// @return 资源信息指针，未找到返回nullptr
    virtual const ShaderResource* FindResource(const std::string& name) const = 0;

    /// @brief 根据绑定点查找资源
    /// @param bindPoint 绑定点
    /// @param space 寄存器空间 (Vulkan为Set)
    /// @return 资源信息指针
    virtual const ShaderResource* FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const = 0;

    /// @brief 重新编译着色器
    /// @param options 编译选项
    /// @param errors 错误信息输出
    /// @return 是否成功
    virtual bool Recompile(const ShaderCompileOptions* options, std::string& errors) = 0;

    /// @brief 从源码重新编译
    /// @param source 源代码
    /// @param options 编译选项
    /// @param errors 错误信息
    /// @return 是否成功
    virtual bool RecompileFromSource(const std::string& source, const ShaderCompileOptions* options, std::string& errors) = 0;

    /// @brief 从文件重新加载并编译
    /// @param errors 错误信息
    /// @return 是否成功
    virtual bool ReloadFromFile(std::string& errors) = 0;

    /// @brief 启用/禁用热重载
    /// @param enable 是否启用
    virtual void EnableHotReload(bool enable) = 0;

    /// @brief 检查源文件是否被修改
    /// @return 是否修改
    virtual bool IsFileModified() const = 0;

    /// @brief 是否需要重新加载（修改后未编译）
    /// @return 是否需要
    virtual bool NeedsReload() const = 0;

    /// @brief 获取文件最后修改时间
    /// @return 修改时间戳
    virtual uint64_t GetFileModificationTime() const = 0;

    /// @brief 获取编译日志
    /// @return 编译日志
    virtual const std::string& GetCompileLog() const = 0;

    /// @brief 是否有编译警告
    /// @return 是否有
    virtual bool HasWarnings() const = 0;

    /// @brief 是否有编译错误
    /// @return 是否有
    virtual bool HasErrors() const = 0;

    /// @brief 验证着色器是否有效
    /// @return 是否有效
    virtual bool Validate() = 0;

    /// @brief 反汇编着色器
    /// @return 反汇编代码
    virtual std::string Disassemble() const = 0;

    /// @brief 保存调试信息到文件
    /// @param filename 文件名
    /// @param includeDisassembly 是否包含反汇编
    /// @param includeReflection 是否包含反射
    /// @return 是否成功
    virtual bool DebugSaveToFile(const std::string& filename, bool includeDisassembly, bool includeReflection) const = 0;

    /// @brief 获取依赖文件列表
    /// @return 依赖列表
    virtual const std::vector<std::string>& GetDependencies() const = 0;

    /// @brief 获取包含文件列表
    /// @return 包含列表
    virtual const std::vector<std::string>& GetIncludes() const = 0;

    /// @brief 获取宏定义列表
    /// @return 定义列表
    virtual const std::vector<std::string>& GetDefines() const = 0;
};

} // namespace Prisma::Graphic
