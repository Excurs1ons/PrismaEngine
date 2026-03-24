#include "DynamicLoader.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <unistd.h>
    #include <limits.h>
#endif

namespace Prisma {

DynamicLoader::DynamicLoader() : m_handle(nullptr), m_tempPath("") {}

DynamicLoader::~DynamicLoader() {
    Unload();
}

bool DynamicLoader::Load(const std::string& libraryPath) {
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

bool DynamicLoader::TryLoad(const std::string& libraryPath) {
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

void DynamicLoader::Unload() {
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

std::string DynamicLoader::CopyToTempFile(const std::string& sourcePathStr) {
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

bool DynamicLoader::IsLoaded() const {
    return m_handle != nullptr;
}

} // namespace Prisma
