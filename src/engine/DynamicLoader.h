#pragma once

#include <windows.h>

#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class DynamicLoader {
public:
    DynamicLoader() : m_handle(nullptr), m_tempPath("") {}
    
    ~DynamicLoader() {
        Unload();
    }
    
    bool Load(const std::string& libraryPath) {
        m_handle = LoadLibraryA(libraryPath.c_str());
        if (m_handle == nullptr) {
            DWORD error = GetLastError();
            LOG_ERROR("DynamicLoader", "Failed to load library: {0}, error: {1}", libraryPath.c_str(), error);
            throw std::runtime_error("Failed to load library: " + libraryPath);
        }
        return true;
    }
    
    bool TryLoad(const std::string& libraryPath) {
        std::string tempPath = CopyToTempFile(libraryPath);
        if (tempPath.empty()) {
            LOG_FATAL("DynamicLoader", "Failed to create temp DLL: {0}", libraryPath.c_str());
            return false;
        }
        
        m_tempPath = tempPath;
        
        m_handle = LoadLibraryA(m_tempPath.c_str());
        if (!m_handle) {
            DWORD error = GetLastError();
            LOG_FATAL("DynamicLoader", "Failed to load library: {0}, error: {1}", libraryPath.c_str(), error);
            DeleteFileA(m_tempPath.c_str());
            m_tempPath = "";
            return false;
        }
        return true;
    }
    
    void Unload() {
        if (m_handle) {
            FreeLibrary((HMODULE)m_handle);
            if (!m_tempPath.empty()) {
                DeleteFileA(m_tempPath.c_str());
                m_tempPath = "";
            }
            m_handle = nullptr;
        }
    }
    
    std::string CopyToTempFile(const std::string& sourcePathStr) {
        std::filesystem::path sourcePath(sourcePathStr);

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
    }
    
    template<typename T>
    T GetFunction(const std::string& functionName) {
        if (!m_handle) {
            LOG_ERROR("DynamicLoader", "Library not loaded");
            throw std::runtime_error("Library not loaded");
        }
        
        FARPROC func = GetProcAddress((HMODULE)m_handle, functionName.c_str());
        if (!func) {
            DWORD error = GetLastError();
            LOG_ERROR("DynamicLoader", "Cannot get function: {0}, error: {1}", functionName.c_str(), error);
            throw std::runtime_error("Failed to get function: " + functionName);
        }
        return reinterpret_cast<T>(func);
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
