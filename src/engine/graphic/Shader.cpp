#include "Shader.h"
#include "Logger.h"
#include <fstream>

namespace Prisma::Graphic {

bool Shader::Load(const std::filesystem::path& path) {
    m_Bytecode.clear();
    m_BytecodeBytes.clear();
    m_Reflection.Resources.clear();
    m_ResourceMap.clear();
    SetPath(path);
    m_FilenameCache = path.string();

    LOG_DEBUG("Shader", "正在从以下路径加载着色器字节码: {0}", path.string());
    if (std::filesystem::exists(path)) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            const auto fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            if (fileSize > 0) {
                m_BytecodeBytes.resize(static_cast<size_t>(fileSize));
                if (file.read(reinterpret_cast<char*>(m_BytecodeBytes.data()), fileSize)) {
                    const size_t wordCount = m_BytecodeBytes.size() / sizeof(uint32_t);
                    m_Bytecode.resize(wordCount);
                    std::memcpy(m_Bytecode.data(), m_BytecodeBytes.data(), wordCount * sizeof(uint32_t));
                }
            }
        }
    }
    
    m_IsLoaded = !m_Bytecode.empty();
    return m_IsLoaded;
}

bool Shader::LoadFromMemory(const uint8_t* data, size_t size) {
    m_Bytecode.clear();
    m_BytecodeBytes.clear();
    m_Reflection.Resources.clear();
    m_ResourceMap.clear();

    if (size > 0) {
        m_BytecodeBytes.assign(data, data + size);
        const size_t wordCount = size / sizeof(uint32_t);
        m_Bytecode.resize(wordCount);
        std::memcpy(m_Bytecode.data(), data, wordCount * sizeof(uint32_t));
    }

    m_IsLoaded = !m_Bytecode.empty();
    return m_IsLoaded;
}

void Shader::Unload() {
    m_Bytecode.clear();
    m_BytecodeBytes.clear();
    m_Reflection.Resources.clear();
    m_ResourceMap.clear();
    m_RHIResource = nullptr;
    m_IsLoaded = false;
    m_FilenameCache.clear();
}

const ShaderResource* Shader::FindResource(const std::string& name) const {
    if (m_RHIResource) return m_RHIResource->FindResource(name);
    auto it = m_ResourceMap.find(name);
    return (it != m_ResourceMap.end()) ? &m_Reflection.Resources[it->second] : nullptr;
}

const ShaderResource* Shader::FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const {
    if (m_RHIResource) return m_RHIResource->FindResourceByBindPoint(bindPoint, space);
    for (const auto& res : m_Reflection.Resources) {
        if (res.Binding == bindPoint && res.Set == space) return &res;
    }
    return nullptr;
}

const std::string& Shader::GetEntryPoint() const { static const std::string main = "main"; return main; }
const std::string& Shader::GetTarget() const { static const std::string empty = ""; return empty; }
const std::string& Shader::GetSource() const { static const std::string empty = ""; return empty; }
const ShaderCompileOptions& Shader::GetCompileOptions() const { static const ShaderCompileOptions opt{}; return opt; }

std::shared_ptr<Shader> ShaderLibrary::Load(const std::string& name, const std::filesystem::path& path) {
    auto existing = Get(name);
    if (existing) return existing;
    auto shader = std::make_shared<Shader>();
    shader->SetName(name);
    if (shader->Load(path)) {
        m_Shaders[name] = shader;
        if (std::filesystem::exists(path)) m_lastWriteTimes[name] = std::filesystem::last_write_time(path);
        return shader;
    }
    return nullptr;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name) {
    auto it = m_Shaders.find(name);
    return (it != m_Shaders.end()) ? it->second : nullptr;
}

void ShaderLibrary::Update(Prisma::Timestep ts) {
    m_accumulatedTime += ts.GetSeconds();
    if (m_accumulatedTime >= 5.0f) {
        m_accumulatedTime = 0.0f;
    }
}

} // namespace Prisma::Graphic
