#include "TextureManager.h"
#include "RenderBackend.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Engine {
namespace Graphic {

// 加载纹理数据的辅助函数
bool LoadImageData(const std::string& filePath, std::vector<uint8_t>& data,
                   uint32_t& width, uint32_t& height, uint32_t& channels);

TextureManager& TextureManager::GetInstance()
{
    static TextureManager instance;
    return instance;
}

std::shared_ptr<ITexture> TextureManager::CreateTexture(const TextureDesc& desc)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 获取渲染后端
    auto backend = RenderSystem::GetInstance()->GetRenderBackend();
    if (!backend) {
        LOG_ERROR("TextureManager", "无效的渲染后端");
        return nullptr;
    }

    // 创建纹理
    auto texture = backend->CreateTexture(desc);
    if (texture) {
        m_stats.totalTextures++;
        m_stats.memoryUsage += CalculateTextureMemory(desc) / (1024 * 1024);
        LOG_DEBUG("TextureManager", "创建纹理: {0}x{1}, 格式: {2}",
                  desc.width, desc.height, static_cast<uint32_t>(desc.format));
    }

    return texture;
}

std::shared_ptr<ITexture> TextureManager::LoadTexture(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查缓存
    auto it = m_textures.find(filePath);
    if (it != m_textures.end() && it->second) {
        return it->second;
    }

    // 构建完整路径
    std::string fullPath = m_searchPath + filePath;

    // 加载图像数据
    std::vector<uint8_t> imageData;
    uint32_t width, height, channels;
    if (!LoadImageData(fullPath, imageData, width, height, channels)) {
        LOG_ERROR("TextureManager", "加载纹理失败: {0}", fullPath);
        return nullptr;
    }

    // 确定格式
    TextureFormat format;
    switch (channels) {
    case 1: format = TextureFormat::R8; break;
    case 2: format = TextureFormat::RG8; break;
    case 3: format = TextureFormat::RGB8; break;
    case 4: format = TextureFormat::RGBA8; break;
    default:
        LOG_ERROR("TextureManager", "不支持的通道数: {0}", channels);
        return nullptr;
    }

    // 创建纹理描述
    TextureDesc desc;
    desc.type = TextureType::Texture2D;
    desc.format = format;
    desc.width = width;
    desc.height = height;
    desc.generateMips = true;

    // 创建纹理
    auto texture = CreateTexture(desc);
    if (!texture) {
        return nullptr;
    }

    // 更新纹理数据
    if (!texture->UpdateData(imageData.data(), static_cast<uint32_t>(imageData.size()))) {
        LOG_ERROR("TextureManager", "更新纹理数据失败: {0}", filePath);
        return nullptr;
    }

    // 生成Mipmap
    if (desc.generateMips) {
        texture->GenerateMips();
    }

    // 缓存纹理
    m_textures[filePath] = texture;
    m_stats.loadedTextures++;

    LOG_INFO("TextureManager", "成功加载纹理: {0} ({1}x{2})", filePath, width, height);
    return texture;
}

std::shared_ptr<ITexture> TextureManager::LoadCubeMap(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 对于立方体贴图，文件路径可能包含通配符或特定格式
    // 这里简化处理，假设 filePath 是 6 个面的路径列表

    // TODO: 实现立方体贴图加载
    LOG_WARNING("TextureManager", "立方体贴图加载尚未实现: {0}", filePath);
    return nullptr;
}

std::shared_ptr<ITexture> TextureManager::CreateRenderTarget(uint32_t width, uint32_t height,
                                                            TextureFormat format)
{
    TextureDesc desc;
    desc.type = TextureType::Texture2D;
    desc.format = format;
    desc.width = width;
    desc.height = height;
    desc.renderTarget = true;
    desc.generateMips = false;

    auto texture = CreateTexture(desc);
    if (texture) {
        m_stats.renderTargets++;
    }

    return texture;
}

std::shared_ptr<ITexture> TextureManager::CreateDepthBuffer(uint32_t width, uint32_t height,
                                                           TextureFormat format)
{
    TextureDesc desc;
    desc.type = TextureType::Texture2D;
    desc.format = format;
    desc.width = width;
    desc.height = height;
    desc.depthStencil = true;
    desc.generateMips = false;

    return CreateTexture(desc);
}

std::shared_ptr<ISampler> TextureManager::CreateSampler(const SamplerDesc& desc)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 生成哈希键
    size_t key = std::hash<std::string>{}(std::string(reinterpret_cast<const char*>(&desc), sizeof(desc)));

    // 检查缓存
    auto it = m_samplers.find(key);
    if (it != m_samplers.end()) {
        return it->second;
    }

    // 获取渲染后端
    auto backend = RenderSystem::GetInstance()->GetRenderBackend();
    if (!backend) {
        LOG_ERROR("TextureManager", "无效的渲染后端");
        return nullptr;
    }

    // 创建采样器
    auto sampler = backend->CreateSampler(desc);
    if (sampler) {
        m_samplers[key] = sampler;
        m_stats.totalSamplers++;
    }

    return sampler;
}

std::shared_ptr<ITexture> TextureManager::GetTexture(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_textures.find(filePath);
    return (it != m_textures.end()) ? it->second : nullptr;
}

std::shared_ptr<ITexture> TextureManager::GetWhiteTexture()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_whiteTexture) {
        CreateDefaultTextures();
    }
    return m_whiteTexture;
}

std::shared_ptr<ITexture> TextureManager::GetBlackTexture()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_blackTexture) {
        CreateDefaultTextures();
    }
    return m_blackTexture;
}

std::shared_ptr<ITexture> TextureManager::GetNormalTexture()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_normalTexture) {
        CreateDefaultTextures();
    }
    return m_normalTexture;
}

void TextureManager::ReleaseTexture(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_textures.find(filePath);
    if (it != m_textures.end()) {
        // 更新统计
        if (it->second) {
            m_stats.memoryUsage -= CalculateTextureMemory(it->second->GetDesc()) / (1024 * 1024);
            if (it->second->GetDesc().renderTarget) {
                m_stats.renderTargets--;
            }
        }

        m_textures.erase(it);
        LOG_DEBUG("TextureManager", "释放纹理: {0}", filePath);
    }
}

void TextureManager::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 清理所有资源
    m_textures.clear();
    m_samplers.clear();
    m_whiteTexture.reset();
    m_blackTexture.reset();
    m_normalTexture.reset();
    m_defaultSampler.reset();

    // 重置统计
    m_stats = {};

    LOG_INFO("TextureManager", "资源清理完成");
}

void TextureManager::SetTextureSearchPath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_searchPath = path;
    LOG_INFO("TextureManager", "设置纹理搜索路径: {0}", path);
}

void TextureManager::PreloadTextures(const std::string& directory)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_INFO("TextureManager", "预加载纹理目录: {0}", directory);

    std::filesystem::path searchPath = std::filesystem::path(m_searchPath) / directory;

    // 遍历所有支持的纹理文件
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // 检查支持的格式
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
                ext == ".bmp" || ext == ".tga" || ext == ".dds" ||
                ext == ".hdr" || ext == ".tiff" || ext == ".webp") {

                std::string relativePath = std::filesystem::relative(entry.path(), m_searchPath).string();
                LoadTexture(relativePath);
            }
        }
    }
}

TextureManager::TextureStats TextureManager::GetStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void TextureManager::CreateDefaultTextures()
{
    // 创建白色纹理
    if (!m_whiteTexture) {
        TextureDesc desc;
        desc.type = TextureType::Texture2D;
        desc.format = TextureFormat::RGBA8;
        desc.width = 1;
        desc.height = 1;

        m_whiteTexture = CreateTexture(desc);
        if (m_whiteTexture) {
            uint8_t whitePixel[] = {255, 255, 255, 255};
            m_whiteTexture->UpdateData(whitePixel, 4);
        }
    }

    // 创建黑色纹理
    if (!m_blackTexture) {
        TextureDesc desc;
        desc.type = TextureType::Texture2D;
        desc.format = TextureFormat::RGBA8;
        desc.width = 1;
        desc.height = 1;

        m_blackTexture = CreateTexture(desc);
        if (m_blackTexture) {
            uint8_t blackPixel[] = {0, 0, 0, 255};
            m_blackTexture->UpdateData(blackPixel, 4);
        }
    }

    // 创建法线纹理（中性法线）
    if (!m_normalTexture) {
        TextureDesc desc;
        desc.type = TextureType::Texture2D;
        desc.format = TextureFormat::RGBA8;
        desc.width = 1;
        desc.height = 1;

        m_normalTexture = CreateTexture(desc);
        if (m_normalTexture) {
            uint8_t normalPixel[] = {128, 128, 255, 255};
            m_normalTexture->UpdateData(normalPixel, 4);
        }
    }

    // 创建默认采样器
    if (!m_defaultSampler) {
        SamplerDesc desc;
        desc.filter = TextureFilter::Linear;
        desc.addressU = desc.addressV = desc.addressW = TextureAddressMode::Wrap;
        m_defaultSampler = CreateSampler(desc);
    }
}

uint32_t TextureManager::CalculateTextureMemory(const TextureDesc& desc) const
{
    // 计算每个像素的字节数
    uint32_t bytesPerPixel = 0;
    switch (desc.format) {
    case TextureFormat::R8: bytesPerPixel = 1; break;
    case TextureFormat::RG8: bytesPerPixel = 2; break;
    case TextureFormat::RGB8: bytesPerPixel = 3; break;
    case TextureFormat::RGBA8: bytesPerPixel = 4; break;
    case TextureFormat::R16: bytesPerPixel = 2; break;
    case TextureFormat::RG16: bytesPerPixel = 4; break;
    case TextureFormat::RGB16: bytesPerPixel = 6; break;
    case TextureFormat::RGBA16: bytesPerPixel = 8; break;
    case TextureFormat::R16F: bytesPerPixel = 2; break;
    case TextureFormat::RG16F: bytesPerPixel = 4; break;
    case TextureFormat::RGB16F: bytesPerPixel = 6; break;
    case TextureFormat::RGBA16F: bytesPerPixel = 8; break;
    case TextureFormat::R32F: bytesPerPixel = 4; break;
    case TextureFormat::RG32F: bytesPerPixel = 8; break;
    case TextureFormat::RGB32F: bytesPerPixel = 12; break;
    case TextureFormat::RGBA32F: bytesPerPixel = 16; break;
    case TextureFormat::D16: bytesPerPixel = 2; break;
    case TextureFormat::D24: bytesPerPixel = 3; break;
    case TextureFormat::D32: bytesPerPixel = 4; break;
    case TextureFormat::D24S8: bytesPerPixel = 4; break;
    case TextureFormat::D32S8: bytesPerPixel = 8; break;
    default:
        // 压缩格式需要特殊处理
        if (desc.format >= TextureFormat::BC1 && desc.format <= TextureFormat::BC5) {
            bytesPerPixel = 1; // 4x4 块平均
        } else if (desc.format >= TextureFormat::BC6H && desc.format <= TextureFormat::BC7) {
            bytesPerPixel = 1; // 4x4 块平均
        }
        break;
    }

    // 计算Mipmap级别的总大小
    uint32_t totalSize = 0;
    uint32_t width = desc.width;
    uint32_t height = desc.height;
    uint32_t levels = desc.mipLevels > 0 ? desc.mipLevels : 1;

    for (uint32_t level = 0; level < levels; ++level) {
        totalSize += width * height * bytesPerPixel;
        width = std::max(width / 2, 1u);
        height = std::max(height / 2, 1u);
    }

    return totalSize * desc.arraySize;
}

// 临时实现 - 实际项目中应使用stb_image等库
bool LoadImageData(const std::string& filePath, std::vector<uint8_t>& data,
                   uint32_t& width, uint32_t& height, uint32_t& channels)
{
    // TODO: 实现实际的图像加载
    // 这里只是占位符
    LOG_WARNING("TextureManager", "图像加载功能尚未实现: {0}", filePath);
    return false;
}

} // namespace Graphic
} // namespace Engine