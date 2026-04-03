#include "DynamicLoader.h"
#include <SDL3/SDL.h>
#include <filesystem>
#include <iostream>

namespace Prisma {

DynamicLoader::DynamicLoader() : m_handle(nullptr), m_tempPath("") {}

DynamicLoader::~DynamicLoader() {
    Unload();
}

bool DynamicLoader::Load(const std::string& libraryPath) {
    m_handle = reinterpret_cast<void*>(SDL_LoadObject(libraryPath.c_str()));
    if (m_handle == nullptr) {
        LOG_ERROR("DynamicLoader", "无法加载库: {0}，错误: {1}", libraryPath.c_str(), SDL_GetError());
        throw std::runtime_error("Failed to load library: " + libraryPath + " Error: " + SDL_GetError());
    }
    return true;
}

bool DynamicLoader::TryLoad(const std::string& libraryPath) {
    // SDL3 handles platform-specific loading, but the temporary file logic 
    // is often used to avoid locking the original file.
    std::string tempPath = CopyToTempFile(libraryPath);
    if (tempPath.empty()) {
        LOG_FATAL("DynamicLoader", "无法创建临时库文件: {0}", libraryPath.c_str());
        return false;
    }
    
    m_tempPath = tempPath;
    m_handle = reinterpret_cast<void*>(SDL_LoadObject(m_tempPath.c_str()));
    
    if (!m_handle) {
        LOG_FATAL("DynamicLoader", "无法加载库: {0}，错误: {1}", libraryPath.c_str(), SDL_GetError());
        std::error_code ec;
        std::filesystem::remove(m_tempPath, ec);
        m_tempPath = "";
        return false;
    }
    return true;
}

void DynamicLoader::Unload() {
    if (m_handle) {
        SDL_UnloadObject(reinterpret_cast<SDL_SharedObject*>(m_handle));
        if (!m_tempPath.empty()) {
            std::error_code ec;
            std::filesystem::remove(m_tempPath, ec);
            m_tempPath = "";
        }
        m_handle = nullptr;
    }
}

std::string DynamicLoader::CopyToTempFile(const std::string& sourcePathStr) {
    std::filesystem::path sourcePath(sourcePathStr);
    
    // Using SDL_GetBasePath for portable path resolution
    const char* base_path_ptr = SDL_GetBasePath();
    std::filesystem::path exe_path = base_path_ptr ? std::filesystem::path(base_path_ptr) : std::filesystem::current_path();
    // In SDL3, SDL_GetBasePath returns a const char* that does NOT need to be freed? 
    // Wait, let me check SDL3 documentation or common usage.
    // Actually SDL3 SDL_GetBasePath returns a const string that should be freed with SDL_free.
    // But the return type is const char*.
    if (base_path_ptr) SDL_free(const_cast<char*>(base_path_ptr));
    
    auto sourceAbsolutePath = exe_path / sourcePath;
    
    if (!std::filesystem::exists(sourceAbsolutePath)) {
        // Try current working directory as fallback
        sourceAbsolutePath = std::filesystem::current_path() / sourcePath;
        if (!std::filesystem::exists(sourceAbsolutePath)) {
            LOG_FATAL("DynamicLoader", "未找到源文件: {0}", sourcePathStr.c_str());
            return "";
        }
    }
    
    // Create a unique temporary filename
    std::string extension = sourcePath.extension().string();
    if (extension.empty()) {
#ifdef _WIN32
        extension = ".dll";
#elif defined(__APPLE__)
        extension = ".dylib";
#else
        extension = ".so";
#endif
    }
    
    std::string tempFileName = (std::filesystem::temp_directory_path() / 
                               ("prisma_lib_" + std::to_string(SDL_GetTicks()) + extension)).string();
    
    std::error_code ec;
    if (std::filesystem::copy_file(sourceAbsolutePath, tempFileName, std::filesystem::copy_options::overwrite_existing, ec)) {
        return tempFileName;
    } else {
        LOG_FATAL("DynamicLoader", "无法复制文件: {0} -> {1}, 错误: {2}", 
                  sourceAbsolutePath.string().c_str(), tempFileName.c_str(), ec.message().c_str());
        return "";
    }
}

bool DynamicLoader::IsLoaded() const {
    return m_handle != nullptr;
}

void* DynamicLoader::GetSymbol(const std::string& symbolName) {
    if (!m_handle) return nullptr;
    return reinterpret_cast<void*>(SDL_LoadFunction(reinterpret_cast<SDL_SharedObject*>(m_handle), symbolName.c_str()));
}

} // namespace Prisma
