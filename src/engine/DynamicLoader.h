#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class DynamicLoader {
public:
    DynamicLoader() : m_handle(nullptr)
#ifdef _WIN32
    , m_tempPath("")
#endif
    {}
    
    ~DynamicLoader() {
        Unload();
    }
    
    bool Load(const std::string& libraryPath) {
#ifdef _WIN32
        m_handle = LoadLibraryA(libraryPath.c_str());
        if (m_handle == nullptr) {
            DWORD error = GetLastError();
            LOG_ERROR("DynamicLoader", "无法加载动态库: {0},错误码: {1}", libraryPath.c_str(), error);
            throw std::runtime_error("Failed to load library: " + libraryPath + ", error code: " + std::to_string(error));
        }
#else
        m_handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
        if (!m_handle) {
            LOG_ERROR("DynamicLoader", "无法加载动态库: {0},错误码: {1}", libraryPath.c_str(), dlerror());
            throw std::runtime_error("Failed to load library: " + libraryPath + ", error: " + std::string(dlerror()));
        }
#endif
        return true;
    }
    
    // 新增TryLoad方法，加载失败时返回false而不是抛出异常
    bool TryLoad(const std::string& libraryPath) {
#ifdef _WIN32
        // 为了解决LNK1168错误，我们将DLL文件复制到临时位置再加载
        // 这样原始DLL文件就不会被锁定，允许在开发过程中重新编译
        std::string tempPath = CopyToTempFile(libraryPath);
        if (tempPath.empty()) {
            LOG_FATAL("DynamicLoader", "无法创建临时DLL文件: {0}", libraryPath.c_str());
            return false;
        }
        
        // 保存临时文件路径以便稍后清理
        m_tempPath = tempPath;
        
        // 从临时位置加载DLL
        m_handle = LoadLibraryA(m_tempPath.c_str());
        if (!m_handle) {
            DWORD error = GetLastError();
            LOG_FATAL("DynamicLoader", "无法加载动态库: {0},错误码: {1}", libraryPath.c_str(), error);
            // 清理临时文件
            DeleteFileA(m_tempPath.c_str());
            m_tempPath = "";
            return false;
        }
#else
        m_handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
        if (!m_handle) {
            LOG_FATAL("DynamicLoader", "无法加载动态库: {0},错误码: {1}", libraryPath.c_str(), dlerror());
            return false;
        }
#endif
        return true;
    }
    
    void Unload() {
        if (m_handle) {
#ifdef _WIN32
            FreeLibrary((HMODULE)m_handle);
            // 清理临时文件
            if (!m_tempPath.empty()) {
                DeleteFileA(m_tempPath.c_str());
                m_tempPath = "";
            }
#else
            dlclose(m_handle);
#endif
            m_handle = nullptr;
        }
    }
    
#ifdef _WIN32
    // 将DLL文件复制到临时位置
    std::string CopyToTempFile(const std::string& sourcePathStr) {

        // 判断是否是绝对路径
        std::filesystem::path sourcePath(sourcePathStr);

        char exe_path_str[MAX_PATH];
        GetModuleFileNameA(nullptr, exe_path_str, MAX_PATH);
        auto exe_path      = std::filesystem::path(exe_path_str).parent_path();
        auto sourceAbsolutePath = exe_path / sourcePath;
        LOG_INFO("DynamicLoader", "源文件绝对路径: {0}", sourceAbsolutePath.string());
        
        // 判断文件是否存在
        if (!std::filesystem::exists(sourceAbsolutePath)) {
            LOG_FATAL("DynamicLoader", "源文件不存在: {0}", sourceAbsolutePath.string());
            return "";
        }
        // 创建临时文件
        char tempPath[MAX_PATH];
        char tempFileName[MAX_PATH];
        
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_FATAL("DynamicLoader", "无法获取临时文件路径");
            return "";
        }
        LOG_INFO("DynamicLoader", "临时文件路径: {0}", tempPath);

        if (GetTempFileNameA(tempPath, "dll_", 0, tempFileName) == 0) {
            LOG_FATAL("DynamicLoader", "无法创建临时文件");
            return "";
        }
        LOG_INFO("DynamicLoader", "临时文件已创建: {0}", tempFileName);

        // 删除临时文件，因为我们想要使用相同的名称但有不同的扩展名
        if (!DeleteFileA(tempFileName)) {
            LOG_FATAL("DynamicLoader", "无法删除临时文件");
        }
        LOG_INFO("DynamicLoader", "临时文件已删除: {0}", tempFileName);
        
        // 构造新的临时文件名，保持.dll扩展名
        std::string newTempFileName(tempFileName);
        newTempFileName += ".dll";
        
        // 复制文件
        if (!CopyFileA(sourceAbsolutePath.string().c_str(), newTempFileName.c_str(), FALSE)) {
            LOG_FATAL(
                "DynamicLoader", "无法复制文件到临时位置：{0} -> {1}", sourceAbsolutePath.string(), newTempFileName);
            return "";
        }
        LOG_INFO("DynamicLoader", "文件已复制到临时位置: {0}", newTempFileName);
        return newTempFileName;
    }
#endif
    
    template<typename T>
    T GetFunction(const std::string& functionName) {
        if (!m_handle) {
            LOG_ERROR("DynamicLoader", "动态库未加载");
            throw std::runtime_error("Library not loaded");
        }
        
#ifdef _WIN32
        FARPROC func = GetProcAddress((HMODULE)m_handle, functionName.c_str());
        if (!func) {
            DWORD error = GetLastError();
            LOG_ERROR("DynamicLoader", "无法获取函数: {0},错误码: {1}", functionName.c_str(), error);
            throw std::runtime_error("Failed to get function: " + functionName + ", error code: " + std::to_string(error));
        }
        return reinterpret_cast<T>(func);
#else
        void* func = dlsym(m_handle, functionName.c_str());
        if (!func) {
            LOG_ERROR("DynamicLoader", "无法获取函数: {0},错误码: {1}", functionName.c_str(), dlerror());
            throw std::runtime_error("Failed to get function: " + functionName + ", error: " + std::string(dlerror()));
        }
        return reinterpret_cast<T>(func);
#endif
    }
    
    // 新增TryGetFunction方法，获取函数失败时返回false而不是抛出异常
    template<typename T>
    bool TryGetFunction(const std::string& functionName, T& outFunc) {
        if (!m_handle) {
            LOG_FATAL("DynamicLoader", "动态库未加载");
            return false;
        }
        try {
            outFunc = GetFunction<T>(functionName);
        }
        catch (const std::exception& e) {
            LOG_FATAL("DynamicLoader", "无法获取函数: {0},错误码: {1}", functionName.c_str(), e.what());
            return false;
        }
        if (!outFunc) {
            LOG_FATAL("DynamicLoader", "无法获取函数: {0},错误码: {1}", functionName.c_str(), "nullptr");
            return false;
        }
        return true;
    }
    
    bool IsLoaded() const {
        return m_handle != nullptr;
    }
    
private:
    void* m_handle;
#ifdef _WIN32
    std::string m_tempPath;  // 临时DLL文件的路径
#endif
};