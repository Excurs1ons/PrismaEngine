#include "Shader.h"
#include "Logger.h"
#include <fstream>

namespace Prisma::Graphic {

bool Shader::Load(const std::filesystem::path& path) {
    m_Bytecode.clear();
    m_Reflection.Resources.clear();
    m_ResourceMap.clear();
    SetPath(path);

    // 1. 加载 SPIR-V 字节码 (这里简化处理，实际应读取文件)
    LOG_INFO("Shader", "Loading shader bytecode from: {0}", path.string());
    if (std::filesystem::exists(path)) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            const auto fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            if (fileSize > 0) {
                std::vector<char> bytes(static_cast<size_t>(fileSize));
                if (file.read(bytes.data(), fileSize)) {
                    const size_t wordCount = bytes.size() / sizeof(uint32_t);
                    m_Bytecode.resize(wordCount);
                    std::memcpy(m_Bytecode.data(), bytes.data(), wordCount * sizeof(uint32_t));
                }
            }
        }
    }
    
    // 2. 模拟反射信息填充 (实际应使用 spirv-cross 等工具自动生成)
    // 假设这是一个基础的 PBR Shader
    ShaderResource albedo;
    albedo.Name = "u_Albedo";
    albedo.ResourceType = ShaderResource::Type::Sampler2D;
    albedo.Set = 1;
    albedo.Binding = 0;
    m_Reflection.Resources.push_back(albedo);
    
    ShaderResource params;
    params.Name = "u_MaterialParams";
    params.ResourceType = ShaderResource::Type::UniformBuffer;
    params.Set = 1;
    params.Binding = 1;
    params.Size = sizeof(float) * 4; // metallic, roughness, etc.
    m_Reflection.Resources.push_back(params);

    // 3. 构建快速查找映射
    for (uint32_t i = 0; i < (uint32_t)m_Reflection.Resources.size(); ++i) {
        m_ResourceMap[m_Reflection.Resources[i].Name] = i;
    }

    m_IsLoaded = true;
    return true;
}

void Shader::Unload() {
    m_Bytecode.clear();
    m_Reflection.Resources.clear();
    m_ResourceMap.clear();
    m_IsLoaded = false;
}

const ShaderResource* Shader::FindResource(const std::string& name) const {
    auto it = m_ResourceMap.find(name);
    if (it != m_ResourceMap.end()) {
        return &m_Reflection.Resources[it->second];
    }
    return nullptr;
}

// === ShaderLibrary 实现 ===

std::shared_ptr<Shader> ShaderLibrary::Load(const std::string& name, const std::filesystem::path& path) {
    if (m_Shaders.find(name) != m_Shaders.end()) {
        LOG_WARNING("Shader", "Shader already exists in library: {0}", name);
        return m_Shaders[name];
    }

    auto shader = std::make_shared<Shader>();
    shader->SetName(name);
    if (shader->Load(path)) {
        m_Shaders[name] = shader;
        if (std::filesystem::exists(path)) {
            m_lastWriteTimes[name] = std::filesystem::last_write_time(path);
        }
        return shader;
    }

    LOG_ERROR("Shader", "Failed to load shader: {0}", path.string());
    return nullptr;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name) {
    if (m_Shaders.find(name) == m_Shaders.end()) {
        LOG_ERROR("Shader", "Shader not found in library: {0}", name);
        return nullptr;
    }
    return m_Shaders[name];
}

void ShaderLibrary::Update(Prisma::Timestep ts) {
    m_accumulatedTime += ts.GetSeconds();

    // 每 5 秒检查一次着色器热重载
    if (m_accumulatedTime >= 5.0f) {
        for (auto& [name, shader] : m_Shaders) {
            const auto& path = shader->GetPath();
            if (path.empty() || !std::filesystem::exists(path)) continue;

            auto currentWriteTime = std::filesystem::last_write_time(path);
            if (currentWriteTime > m_lastWriteTimes[name]) {
                LOG_INFO("Shader", "Detected change in shader: {0}. Reloading...", name);
                shader->Unload();
                if (shader->Load(path)) {
                    m_lastWriteTimes[name] = currentWriteTime;
                    LOG_INFO("Shader", "Shader reloaded successfully.");
                } else {
                    LOG_ERROR("Shader", "Failed to reload shader: {0}", name);
                }
            }
        }
        m_accumulatedTime = 0.0f;
    }
}

} // namespace Prisma::Graphic
