#pragma once

#include "RenderTypes.h"
#include <string>
#include <vector>
#include <string_view>

namespace PrismaEngine::Graphic {

/// @brief 着色器反射信息
struct ShaderReflection {
    struct Resource {
        std::string name;
        uint32_t bindPoint;
        uint32_t bindCount;
        uint32_t space;
        ShaderType shaderStage;
        std::string type;  // "Texture2D", "Buffer", "Sampler", etc.
    };

    struct ConstantBuffer {
        std::string name;
        uint32_t size;
        uint32_t bindPoint;
        uint32_t space;
        struct Variable {
            std::string name;
            uint32_t offset;
            uint32_t size;
            std::string type;
        };
        std::vector<Variable> variables;
    };

    struct InputParameter {
        std::string semanticName;
        uint32_t semanticIndex;
        uint32_t registerIndex;
        TextureFormat format;
        uint32_t componentMask;  // Bit mask for components
    };

    struct OutputParameter {
        std::string semanticName;
        uint32_t semanticIndex;
        uint32_t registerIndex;
        TextureFormat format;
        uint32_t componentMask;
    };

    ShaderType shaderType;
    std::string entryPoint;
    std::string target;
    std::vector<Resource> resources;
    std::vector<ConstantBuffer> constantBuffers;
    std::vector<InputParameter> inputs;
    std::vector<OutputParameter> outputs;
    std::vector<std::string> defines;
};

/// @brief 着色器编译选项
struct ShaderCompileOptions {
    bool debug = false;
    bool optimize = true;
    bool skipValidation = false;
    bool enable16BitTypes = false;
    bool allResourcesBound = false;
    bool avoidFlowControl = false;
    bool preferFlowControl = false;
    bool enableStrictness = false;
    bool ieeeStrictness = false;
    bool warningsAsErrors = false;
    bool resourcesMayAlias = false;
    int optimizationLevel = 3;  // 0-3, higher is more optimization
    uint32_t flags = 0;  // 编译标志位
    std::vector<std::string> additionalDefines;
    std::string additionalIncludePath;
    std::string additionalArguments;
    std::vector<std::string> dependencies;  // 依赖的其他着色器
    std::vector<std::string> includeDirectories;  // 包含目录
};

/// @brief 着色器抽象接口
class IShader {
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

    // === 反射信息 ===

    /// @brief 获取着色器反射信息
    /// @return 反射信息
    virtual const ShaderReflection& GetReflection() const = 0;

    /// @brief 检查是否包含反射信息
    /// @return 是否包含反射信息
    virtual bool HasReflection() const = 0;

    /// @brief 根据名称查找资源绑定信息
    /// @param name 资源名称
    /// @return 资源信息指针，如果未找到返回nullptr
    virtual const ShaderReflection::Resource* FindResource(const std::string& name) const = 0;

    /// @brief 根据绑定点查找资源
    /// @param bindPoint 绑定点
    /// @param space 寄存器空间
    /// @return 资源信息指针，如果未找到返回nullptr
    virtual const ShaderReflection::Resource* FindResourceByBindPoint(uint32_t bindPoint, uint32_t space = 0) const = 0;

    /// @brief 根据名称查找常量缓冲区
    /// @param name 常量缓冲区名称
    /// @return 常量缓冲区信息指针，如果未找到返回nullptr
    virtual const ShaderReflection::ConstantBuffer* FindConstantBuffer(const std::string& name) const = 0;

    /// @brief 获取输入参数数量
    /// @return 输入参数数量
    virtual uint32_t GetInputParameterCount() const = 0;

    /// @brief 获取输入参数
    /// @param index 参数索引
    /// @return 输入参数信息
    virtual const ShaderReflection::InputParameter& GetInputParameter(uint32_t index) const = 0;

    /// @brief 获取输出参数数量
    /// @return 输出参数数量
    virtual uint32_t GetOutputParameterCount() const = 0;

    /// @brief 获取输出参数
    /// @param index 参数索引
    /// @return 输出参数信息
    virtual const ShaderReflection::OutputParameter& GetOutputParameter(uint32_t index) const = 0;

    // === 编译和重新编译 ===

    /// @brief 重新编译着色器
    /// @param options 编译选项
    /// @param[out] errors 编译错误信息
    /// @return 是否编译成功
    virtual bool Recompile(const ShaderCompileOptions* options, std::string& errors) = 0;

    /// @brief 从源码重新编译
    /// @param source 新的源码
    /// @param options 编译选项
    /// @param[out] errors 编译错误信息
    /// @return 是否编译成功
    virtual bool RecompileFromSource(const std::string& source,
                                     const ShaderCompileOptions* options,
                                     std::string& errors) = 0;

    /// @brief 从文件重新加载
    /// @param[out] errors 错误信息
    /// @return 是否加载成功
    virtual bool ReloadFromFile(std::string& errors) = 0;

    // === 热重载 ===

    /// @brief 启用热重载
    /// @param enable 是否启用
    virtual void EnableHotReload(bool enable) = 0;

    /// @brief 检查文件是否已修改
    /// @return 是否已修改
    virtual bool IsFileModified() const = 0;

    /// @brief 检查是否需要重新加载
    /// @return 是否需要重新加载
    virtual bool NeedsReload() const = 0;

    /// @brief 获取文件修改时间
    /// @return 修改时间戳
    virtual uint64_t GetFileModificationTime() const = 0;

    // === 调试功能 ===

    /// @brief 获取编译日志
    /// @return 编译日志
    virtual const std::string& GetCompileLog() const = 0;

    /// @brief 检查是否有编译警告
    /// @return 是否有警告
    virtual bool HasWarnings() const = 0;

    /// @brief 检查是否有编译错误
    /// @return 是否有错误
    virtual bool HasErrors() const = 0;

    /// @brief 验证着色器
    /// @return 是否有效
    virtual bool Validate() = 0;

    /// @brief 生成着色器反汇编代码
    /// @return 反汇编代码
    virtual std::string Disassemble() const = 0;

    /// @brief 调试着色器到文件
    /// @param filename 文件名
    /// @param includeDisassembly 是否包含反汇编代码
    /// @param includeReflection 是否包含反射信息
    /// @return 是否成功
    virtual bool DebugSaveToFile(const std::string& filename,
                                bool includeDisassembly = true,
                                bool includeReflection = true) const = 0;

    // === 依赖关系 ===

    /// @brief 获取依赖的着色器文件
    /// @return 依赖文件列表
    virtual const std::vector<std::string>& GetDependencies() const = 0;

    /// @brief 获取包含的文件
    /// @return 包含文件列表
    virtual const std::vector<std::string>& GetIncludes() const = 0;

    /// @brief 获取定义的宏
    /// @return 宏定义列表
    virtual const std::vector<std::string>& GetDefines() const = 0;
};

} // namespace PrismaEngine::Graphic