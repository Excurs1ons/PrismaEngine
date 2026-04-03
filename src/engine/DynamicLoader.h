#pragma once

#include "Export.h"
#include "Logger.h"
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <filesystem>

namespace Prisma {

/**
 * @brief 动态库加载器
 * 使用 SDL3 实现跨平台 DLL/Shared Object 加载。
 */
class ENGINE_API DynamicLoader {
public:
    DynamicLoader();
    ~DynamicLoader();

    /**
     * @brief 加载动态库
     * @param libraryPath 库文件路径
     * @return 是否加载成功
     */
    bool Load(const std::string& libraryPath);

    /**
     * @brief 尝试加载库（会先复制到临时文件，防止锁定）
     * @param libraryPath 库文件路径
     * @return 是否加载成功
     */
    bool TryLoad(const std::string& libraryPath);

    /**
     * @brief 卸载库
     */
    void Unload();

    /**
     * @brief 获取符号地址
     * @param symbolName 符号名称
     * @return 符号地址
     */
    void* GetSymbol(const std::string& symbolName);

    /**
     * @brief 获取库中指定类型的函数
     * @tparam T 函数类型
     * @param symbolName 符号名称
     * @return 函数指针
     */
    template<typename T>
    T GetFunction(const std::string& symbolName) {
        return reinterpret_cast<T>(GetSymbol(symbolName));
    }

    /**
     * @brief 是否已加载
     * @return 是否已加载
     */
    bool IsLoaded() const;

private:
    std::string CopyToTempFile(const std::string& sourcePath);

    void* m_handle;
    std::string m_tempPath;
};

} // namespace Prisma
