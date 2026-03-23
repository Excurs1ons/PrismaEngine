#pragma once

#include "Logger.h"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <unistd.h>
    #include <limits.h>
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Prisma {

class DynamicLoader {
public:
    DynamicLoader() : m_handle(nullptr), m_tempPath("") {}
    
    ~DynamicLoader() {
        Unload();
    }
    
    bool Load(const std::string& libraryPath) {
#ifdef _WIN32
        m_handle = LoadLibraryA(libraryPath.c_str());
        if (m_handle == nullptr) {
            DWORD error = GetLastError();
            LOG_ERROR("DynamicLoader", "Failed to load library: {0}, error: {1}", libraryPath.c_str(), error);
            throw std::runtime_error("Failed to load library: " + libraryPath);
        }
#else
        m_handle = dlopen(libraryPath.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (m_handle == nullptr) {
            LOG_ERROR("DynamicLoader", "Failed to load library: {0}, error: {1}", libraryPath.c_str(), dlerror());
            throw std::runtime_error("Failed to load library: " + libraryPath);
        }
#endif
        return true;
    }
    
    bool TryLoad(const std::string& libraryPath) {
        std::string tempPath = CopyToTempFile(libraryPath);
        if (tempPath.empty()) {
            LOG_FATAL("DynamicLoader", "Failed to create temp DLL: {0}", libraryPath.c_str());
            return false;
        }
        
        m_tempPath = tempPath;
        
#ifdef _WIN32
        m_handle = LoadLibraryA(m_tempPath.c_str());
        if (!m_handle) {
            DWORD error = GetLastError();
            LOG_FATAL("DynamicLoader", "Failed to load library: {0}, error: {1}", libraryPath.c_str(), error);
            DeleteFileA(m_tempPath.c_str());
            m_tempPath = "";
            return false;
        }
#else
        m_handle = dlopen(m_tempPath.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!m_handle) {
            LOG_FATAL("DynamicLoader", "Failed to load library: {0}, error: {1}", libraryPath.c_str(), dlerror());
            std::filesystem::remove(m_tempPath);
            m_tempPath = "";
            return false;
        }
#endif
        return true;
    }
    
    void Unload() {
        if (m_handle) {
#ifdef _WIN32
            FreeLibrary((HMODULE)m_handle);
            if (!m_tempPath.empty()) {
                DeleteFileA(m_tempPath.c_str());
                m_tempPath = "";
            }
#else
            dlclose(m_handle);
            if (!m_tempPath.empty()) {
                std::filesystem::remove(m_tempPath);
                m_tempPath = "";
            }
#endif
            m_handle = nullptr;
        }
    }
    
    std::string CopyToTempFile(const std::string& sourcePathStr) {
        std::filesystem::path sourcePath(sourcePathStr);

#ifdef _WIN32
        char exe_path_str[MAX_PATH];
        GetModuleFileNameA(nullptr, exe_path_str, MAX_PATH);
        auto exe_path = std::filesystem::path(exe_path_str).parent_path();
        auto sourceAbsolutePath = exe_path / sourcePath;
        
        if (!std::filesystem::exists(sourceAbsolutePath)) {
            LOG_FATAL("DynamicLoader", "Source file not found: {0}", sourceAbsolutePath.string());
            return "";
        }
        
        char tempPath[MAX_PATH];
        char tempFileName[MAX_PATH];
        
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            LOG_FATAL("DynamicLoader", "Cannot get temp path");
            return "";
        }

        if (GetTempFileNameA(tempPath, "dll_", 0, tempFileName) == 0) {
            LOG_FATAL("DynamicLoader", "Cannot create temp file");
            return "";
        }
        
        DeleteFileA(tempFileName);
        
        std::string newTempFileName(tempFileName);
        newTempFileName += ".dll";
        
        if (!CopyFileA(sourceAbsolutePath.string().c_str(), newTempFileName.c_str(), FALSE)) {
            LOG_FATAL("DynamicLoader", "Cannot copy file: {0} -> {1}", sourceAbsolutePath.string(), newTempFileName);
            return "";
        }
        return newTempFileName;
#else
        char exe_path_str[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", exe_path_str, PATH_MAX);
        if (count == -1) return "";
        std::string exe_path_s(exe_path_str, count);
        auto exe_path = std::filesystem::path(exe_path_s).parent_path();
        auto sourceAbsolutePath = exe_path / sourcePath;

        if (!std::filesystem::exists(sourceAbsolutePath)) {
            LOG_FATAL("DynamicLoader", "Source file not found: {0}", sourceAbsolutePath.string());
            return "";
        }

        std::string newTempFileName = (std::filesystem::temp_directory_path() / ("lib_" + std::to_string(std::rand()) + ".so")).string();
        
        try {
            std::filesystem::copy_file(sourceAbsolutePath, newTempFileName, std::filesystem::copy_options::overwrite_existing);
        } catch (const std::exception& e) {
            LOG_FATAL("DynamicLoader", "Cannot copy file: {0} -> {1}", sourceAbsolutePath.string(), newTempFileName);
            return "";
        }
        return newTempFileName;
#endif
    }
    
    template<typename T>
    T GetFunction(const std::string& functionName) {
        if (!m_handle) {
            LOG_ERROR("DynamicLoader", "Library not loaded");
            throw std::runtime_error("Library not loaded");
        }
        
#ifdef _WIN32
        FARPROC func = GetProcAddress((HMODULE)m_handle, functionName.c_str());
        if (!func) {
            DWORD error = GetLastError();
            LOG_ERROR("DynamicLoader", "Cannot get function: {0}, error: {1}", functionName.c_str(), error);
            throw std::runtime_error("Failed to get function: " + functionName);
        }
        return reinterpret_cast<T>(func);
#else
        void* func = dlsym(m_handle, functionName.c_str());
        if (!func) {
            LOG_ERROR("DynamicLoader", "Cannot get function: {0}, error: {1}", functionName.c_str(), dlerror());
            throw std::runtime_error("Failed to get function: " + functionName);
        }
        return reinterpret_cast<T>(func);
#endif
    }
    
    template<typename T>
    bool TryGetFunction(const std::string& functionName, T& outFunc) {
        if (!m_handle) {
            LOG_FATAL("DynamicLoader", "Library not loaded");
            return false;
        }
        try {
            outFunc = GetFunction<T>(functionName);
        }
        catch (const std::exception& e) {
            LOG_FATAL("DynamicLoader", "Cannot get function: {0}, error: {1}", functionName.c_str(), e.what());
            return false;
        }
        if (!outFunc) {
            LOG_FATAL("DynamicLoader", "Cannot get function: {0}, error: nullptr", functionName.c_str());
            return false;
        }
        return true;
    }
    
    bool IsLoaded() const {
        return m_handle != nullptr;
    }
    
private:
    void* m_handle;
    std::string m_tempPath;
};

} // namespace Prisma
