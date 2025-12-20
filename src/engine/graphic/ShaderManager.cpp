#include "ShaderManager.h"
#include "RenderBackend.h"
#include "Logger.h"
#include "ShaderFactory.h"  // 添加对ShaderFactory的引用
#include "EngineShaderAdapter.h"  // 添加对适配器的引用
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Engine {
namespace Graphic {

ShaderManager& ShaderManager::GetInstance()
{
    static ShaderManager instance;
    return instance;
}

std::shared_ptr<PrismaEngine::Graphic::IShader> ShaderManager::LoadShader(const ShaderDesc& desc)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 生成缓存键
    std::string key = GenerateShaderKey(desc);

    // 检查是否已加载
    auto it = m_shaders.find(key);
    if (it != m_shaders.end()) {
        return it->second;
    }

    // 编译着色器
    auto shader = CompileShader(desc);
    if (shader) {
        m_shaders[key] = shader;
        m_stats.compiledShaders++;
        LOG_INFO("ShaderManager", "成功加载着色器: {0}", desc.filePath);
    } else {
        m_stats.failedShaders++;
        LOG_ERROR("ShaderManager", "加载着色器失败: {0}", desc.filePath);
    }

    m_stats.totalShaders++;
    return shader;
}

std::shared_ptr<IShaderProgram> ShaderManager::CreateShaderProgram()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 获取渲染后端并创建程序
    auto backend = RenderSystem::GetInstance()->GetRenderBackend();
    if (!backend) {
        LOG_ERROR("ShaderManager", "无效的渲染后端");
        return nullptr;
    }

    auto program = backend->CreateShaderProgram();
    if (program) {
        m_programs.push_back(program);
        m_stats.totalPrograms++;
    }

    return program;
}

std::shared_ptr<PrismaEngine::Graphic::IShader> ShaderManager::GetShader(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 简单查找
    for (const auto& pair : m_shaders) {
        const auto& desc = pair.second->GetDesc();
        if (desc.filePath == filePath) {
            return pair.second;
        }
    }

    return nullptr;
}

void ShaderManager::ReloadAllShaders()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_INFO("ShaderManager", "重新加载所有着色器...");

    uint32_t reloaded = 0;
    uint32_t failed = 0;

    // 保存所有描述符
    std::vector<std::pair<std::string, ShaderDesc>> shaderDescs;
    for (const auto& pair : m_shaders) {
        shaderDescs.emplace_back(pair.first, pair.second->GetDesc());
    }

    // 清除缓存
    m_shaders.clear();

    // 重新编译
    for (const auto& pair : shaderDescs) {
        auto shader = CompileShader(pair.second);
        if (shader) {
            m_shaders[pair.first] = shader;
            reloaded++;
        } else {
            failed++;
        }
    }

    // 重新链接所有程序
    for (auto weakProgram : m_programs) {
        if (auto program = weakProgram.lock()) {
            program->Link();
        }
    }

    LOG_INFO("ShaderManager", "重新加载完成: 成功 {0}, 失败 {1}", reloaded, failed);
}

void ShaderManager::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 清理着色器
    m_shaders.clear();

    // 清理程序（弱引用会自动失效）
    m_programs.clear();

    // 重置统计
    m_stats = {};

    LOG_INFO("ShaderManager", "资源清理完成");
}

void ShaderManager::SetShaderSearchPath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_searchPath = path;
    LOG_INFO("ShaderManager", "设置着色器搜索路径: {0}", path);
}

void ShaderManager::PrecompileAllShaders(const std::string& shaderDir)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_INFO("ShaderManager", "预编译着色器目录: {0}", shaderDir);

    std::filesystem::path searchPath = std::filesystem::path(m_searchPath) / shaderDir;

    // 遍历所有着色器文件
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::string filePath = entry.path().string();

            // 根据扩展名确定着色器类型
            ShaderType type;
            if (ext == ".vert" || ext == ".vs") {
                type = ShaderType::Vertex;
            } else if (ext == ".frag" || ext == ".ps" || ext == ".pixel") {
                type = ShaderType::Pixel;
            } else if (ext == ".geom" || ext == ".gs") {
                type = ShaderType::Geometry;
            } else if (ext == ".comp" || ext == ".cs") {
                type = ShaderType::Compute;
            } else {
                continue;
            }

            // 创建着色器描述
            ShaderDesc desc;
            desc.filePath = filePath;
            desc.type = type;

            // 加载并预编译
            LoadShader(desc);
        }
    }
}

void ShaderManager::SetRenderBackendType(PrismaEngine::Graphic::RenderBackendType type) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_backendType = type;
    LOG_INFO("ShaderManager", "设置渲染后端类型: {0}", static_cast<int>(type));
}

PrismaEngine::Graphic::RenderBackendType ShaderManager::GetRenderBackendType() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_backendType;
}

ShaderManager::ShaderStats ShaderManager::GetStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

std::string ShaderManager::GenerateShaderKey(const ShaderDesc& desc) const
{
    std::stringstream ss;
    ss << desc.filePath << "|" << desc.entryPoint << "|" << static_cast<uint32_t>(desc.type);

    // 添加宏定义
    for (const auto& macro : desc.macros) {
        ss << "|" << macro.name << "=" << macro.value;
    }

    return ss.str();
}

std::shared_ptr<PrismaEngine::Graphic::IShader> ShaderManager::CompileShader(const ShaderDesc& desc)
{
    // 构造新的ShaderDesc
    PrismaEngine::Graphic::ShaderDesc newDesc;
    newDesc.filename = m_searchPath + desc.filePath;
    newDesc.entryPoint = desc.entryPoint;
    newDesc.type = static_cast<PrismaEngine::Graphic::ShaderType>(static_cast<int>(desc.type));
    newDesc.language = PrismaEngine::Graphic::ShaderLanguage::HLSL; // 默认使用HLSL
    newDesc.target = "ps_5_0"; // 默认目标
    
    // 使用ShaderFactory创建着色器
    auto newShader = PrismaEngine::Graphic::ShaderFactory::CreateShaderFromFile(
        m_backendType,
        newDesc.filename,
        newDesc);
    
    if (newShader) {
        return newShader;
    }
    
    // 如果新的着色器系统失败，则回落到旧的Engine::Shader系统
    LOG_WARNING("ShaderManager", "新着色器系统失败，回落到旧系统: {0}", desc.filePath);
    
    // 创建旧的Engine::Shader
    auto engineShader = std::make_shared<Shader>();
    if (engineShader->LoadWithFallback(std::filesystem::path(m_searchPath + desc.filePath))) {
        // 使用适配器包装旧的着色器
        return std::make_shared<PrismaEngine::Graphic::EngineShaderAdapter>(engineShader);
    }
    
    LOG_ERROR("ShaderManager", "无法加载着色器: {0}", desc.filePath);
    return nullptr;
}

} // namespace Graphic
} // namespace Engine