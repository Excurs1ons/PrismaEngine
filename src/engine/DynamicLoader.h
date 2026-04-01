#pragma once

#include "Logger.h"
#include "Export.h"
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace Prisma {

class ENGINE_API DynamicLoader {
public:
    DynamicLoader();
    ~DynamicLoader();
    
    bool Load(const std::string& libraryPath);
    bool TryLoad(const std::string& libraryPath);
    void Unload();
    bool IsLoaded() const;
    
    std::string CopyToTempFile(const std::string& sourcePathStr);
    
    template<typename T>
    T GetFunction(const std::string& functionName) {
        if (!m_handle) {
            LOG_ERROR("DynamicLoader", "库未加载");
            throw std::runtime_error("Library not loaded");
        }
        
#ifdef _WIN32
        FARPROC func = GetProcAddress((HMODULE)m_handle, functionName.c_str());
        if (!func) {
            DWORD error = GetLastError();
            LOG_ERROR("DynamicLoader", "无法获取函数: {0}，错误码: {1}", functionName.c_str(), error);
            throw std::runtime_error("Failed to get function: " + functionName);
        }
        return reinterpret_cast<T>(func);
#else
        void* func = dlsym(m_handle, functionName.c_str());
        if (!func) {
            LOG_ERROR("DynamicLoader", "无法获取函数: {0}，错误: {1}", functionName.c_str(), dlerror());
            throw std::runtime_error("Failed to get function: " + functionName);
        }
        return reinterpret_cast<T>(func);
#endif
    }
    
    template<typename T>
    bool TryGetFunction(const std::string& functionName, T& outFunc) {
        if (!m_handle) {
            LOG_FATAL("DynamicLoader", "库未加载");
            return false;
        }
        try {
            outFunc = GetFunction<T>(functionName);
        }
        catch (const std::exception& e) {
            LOG_FATAL("DynamicLoader", "无法获取函数: {0}，错误: {1}", functionName.c_str(), e.what());
            return false;
        }
        if (!outFunc) {
            LOG_FATAL("DynamicLoader", "无法获取函数: {0}，错误: 空指针", functionName.c_str());
            return false;
        }
        return true;
    }
    
private:
    void* m_handle;
    std::string m_tempPath;
};

} // namespace Prisma
