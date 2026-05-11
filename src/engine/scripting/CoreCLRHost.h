#pragma once

#include <string>
#include <cstdint>

namespace Prisma {
namespace Scripting {

/**
 * @brief CoreCLR 运行时宿主
 *
 * 从指定目录加载 self-contained publish 输出的 hostfxr，
 * 初始化 .NET 运行时，获取 C# [UnmanagedCallersOnly] 入口函数指针。
 * 不探测系统路径，不依赖用户安装 .NET SDK/Runtime。
 */
class CoreCLRHost {
public:
    CoreCLRHost() = default;
    ~CoreCLRHost() { Shutdown(); }

    CoreCLRHost(const CoreCLRHost&) = delete;
    CoreCLRHost& operator=(const CoreCLRHost&) = delete;

    /**
     * @brief 从 scriptsDir 加载 hostfxr 并初始化 CoreCLR 运行时
     * @param scriptsDir  包含 hostfxr.dll + 运行时 + 程序集的目录
     *                    （self-contained publish 的输出目录）
     */
    bool Initialize(const std::string& scriptsDir);

    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief 获取 [UnmanagedCallersOnly] 方法的函数指针
     * @param assemblyPath  程序集文件名 (e.g. "GameScripts.dll")
     * @param typeName      完全限定类型名 (e.g. "GameScripts.ScriptEntry")
     * @param methodName    方法名 (e.g. "Bootstrap")
     */
    void* GetFunctionPointer(const std::string& assemblyPath,
                             const std::string& typeName,
                             const std::string& methodName);

    /** @brief scriptsDir 访问 */
    const std::string& GetScriptsDir() const { return m_scriptsDir; }

private:
    void* m_hostfxrLib = nullptr;
    void* m_hostContext = nullptr;
    void* m_loadAssemblyAndGetFn = nullptr;
    bool m_initialized = false;
    std::string m_scriptsDir;
};

} // namespace Scripting
} // namespace Prisma
