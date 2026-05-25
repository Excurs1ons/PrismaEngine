#pragma once

#include "Export.h"
#include "Logger.h"
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <filesystem>

namespace Prisma {

/* 动态库加载器 */
class ENGINE_API DynamicLoader {
public:
    DynamicLoader();
    ~DynamicLoader();

    /* 加载动态库 */
    bool Load(const std::string& libraryPath);

    /* 尝试加载库（会先复制到临时文件，防止锁定） */
    bool TryLoad(const std::string& libraryPath);

    // 卸载库
    void Unload();

    /* 获取符号地址 */
    void* GetSymbol(const std::string& symbolName);

    /* 获取库中指定类型的函数 */
    template<typename T>
    T GetFunction(const std::string& symbolName) {
        return reinterpret_cast<T>(GetSymbol(symbolName));
    }

    /* 是否已加载 */
    bool IsLoaded() const;

private:
    std::string CopyToTempFile(const std::string& sourcePath);

    void* m_handle;
    std::string m_tempPath;
};

} // namespace Prisma
