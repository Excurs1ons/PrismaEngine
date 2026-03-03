#include "pch.h"
#include "TextureAtlas.h"
#include "../resource/TextureAsset.h"
#include "interfaces/ITexture.h"
#include "../Logger.h"
#include <algorithm>
#include <stack>
#include <unordered_set>

namespace PrismaEngine {
    namespace Graphic {

        // ============================================================================
        // 内部辅助类
        // ============================================================================

        namespace {
            /**
             * @brief 源纹理数据
             */
            struct SourceTexture {
                std::vector<uint8_t> data;
                uint32_t width;
                uint32_t height;
                uint32_t channels;
                std::string name;

                SourceTexture(const std::vector<uint8_t>& d, uint32_t w, uint32_t h, uint32_t c, const std::string& n)
                    : data(d), width(w), height(h), channels(c), name(n) {}
            };

            /**
             * @brief Skyline 算法节点
             */
            struct SkylineNode {
                int x;
                int y;
                int width;

                SkylineNode(int x = 0, int y = 0, int width = 0)
                    : x(x), y(y), width(width) {}
            };

            /**
             * @brief Skyline 算法实现（最高效的矩形打包算法）
             */
            class SkylinePacker {
            public:
                SkylinePacker(int atlasWidth, int atlasHeight, int padding)
                    : m_atlasWidth(atlasWidth)
                    , m_atlasHeight(atlasHeight)
                    , m_padding(padding) {
                    m_skyline.emplace_back(0, 0, atlasWidth);
                }

                /**
                 * @brief 插入矩形
                 * @return 如果成功，返回 (x, y)，否则返回 (-1, -1)
                 */
                std::pair<int, int> insert(int width, int height) {
                    int bestIndex = -1;
                    int bestHeight = INT_MAX;
                    int bestWidth = INT_MAX;
                    int bestX = -1;
                    int bestY = -1;

                    // 尝试在每个位置插入
                    for (size_t i = 0; i < m_skyline.size(); ++i) {
                        int x = m_skyline[i].x;
                        int y = fitRectangle(i, width, height);

                        if (y >= 0) {
                            // 找到合适位置
                            if (y < bestHeight || (y == bestHeight && x < bestX)) {
                                bestIndex = static_cast<int>(i);
                                bestHeight = y;
                                bestX = x;
                                bestY = y;
                                bestWidth = width;
                            }
                        }
                    }

                    if (bestIndex == -1) {
                        return {-1, -1};  // 无法插入
                    }

                    // 更新 Skyline
                    insertSkyline(bestIndex, bestX, bestY, bestWidth, height);

                    return {bestX, bestY};
                }

                /**
                 * @brief 检查矩形是否可以放在指定位置
                 */
                int fitRectangle(size_t skylineIndex, int width, int height) {
                    int x = m_skyline[skylineIndex].x;

                    if (x + width > m_atlasWidth) {
                        return -1;  // 超出右边界
                    }

                    int widthLeft = width;
                    size_t i = skylineIndex;
                    int y = m_skyline[i].y;

                    while (widthLeft > 0) {
                        y = std::max(y, m_skyline[i].y);

                        if (y + height > m_atlasHeight) {
                            return -1;  // 超出上边界
                        }

                        widthLeft -= m_skyline[i].width;
                        ++i;

                        if (i >= m_skyline.size()) {
                            return -1;
                        }
                    }

                    return y;
                }

                /**
                 * @brief 插入新节点到 Skyline
                 */
                void insertSkyline(size_t index, int x, int y, int width, int height) {
                    // 添加新节点
                    m_skyline.insert(m_skyline.begin() + index, SkylineNode(x, y + height, width));

                    // 合并相邻的同高度节点
                    for (size_t i = index + 1; i < m_skyline.size(); ++i) {
                        if (m_skyline[i].y == m_skyline[i - 1].y) {
                            m_skyline[i - 1].width += m_skyline[i].width;
                            m_skyline.erase(m_skyline.begin() + i);
                            --i;
                        }
                    }
                }

            private:
                int m_atlasWidth;
                int m_atlasHeight;
                int m_padding;
                std::vector<SkylineNode> m_skyline;
            };

        } // anonymous namespace

        // ============================================================================
        // TextureAtlas 实现
        // ============================================================================

        namespace {
            class TextureAtlasImpl : public TextureAtlas {
            public:
                explicit TextureAtlasImpl(const AtlasConfig& config)
                    : m_config(config) {
                    LOG_INFO("TextureAtlas", "创建纹理图集，最大尺寸: {}x{}", config.maxWidth, config.maxHeight);
                }

                ~TextureAtlasImpl() override = default;

                bool addTexture(const std::string& texturePath, const std::string& name) override {
                    // 加载纹理
                    PrismaEngine::TextureAsset asset;
                    if (!asset.Load(texturePath)) {
                        LOG_ERROR("TextureAtlas", "加载纹理失败: {}", texturePath);
                        return false;
                    }

                    // 存储源纹理数据
                    m_sourceTextures.emplace(
                        name,
                        SourceTexture(
                            asset.GetData(),
                            asset.GetWidth(),
                            asset.GetHeight(),
                            asset.GetChannels(),
                            name
                        )
                    );

                    m_needsRebuild = true;
                    return true;
                }

                size_t addTextures(const std::vector<std::pair<std::string, std::string>>& textures) override {
                    size_t count = 0;
                    for (const auto& [path, name] : textures) {
                        if (addTexture(path, name)) {
                            ++count;
                        }
                    }
                    return count;
                }

                bool build() override {
                    if (m_sourceTextures.empty()) {
                        LOG_WARNING("TextureAtlas", "没有纹理可构建");
                        return false;
                    }

                    // 使用 Skyline 打包算法
                    TextureAtlasBuilder* builder = TextureAtlasBuilderFactory::create(
                        TextureAtlasBuilder::PackingAlgorithm::SKYLINE
                    ).release();

                    // 添加所有源纹理
                    for (const auto& [name, source] : m_sourceTextures) {
                        builder->addSourceTexture(source.data, source.width, source.height, name);
                    }

                    // 构建图集
                    auto result = builder->build(m_config);

                    if (!result.success) {
                        LOG_ERROR("TextureAtlas", "构建图集失败: {}", result.errorMessage);
                        delete builder;
                        return false;
                    }

                    // 保存结果
                    m_regions = std::move(result.regions);
                    m_atlasData = std::move(result.atlasData);
                    m_width = result.atlasWidth;
                    m_height = result.atlasHeight;
                    m_needsRebuild = false;

                    LOG_INFO("TextureAtlas", "构建图集成功: {}x{}, {} 个纹理, 填充率: {:.1f}%",
                             m_width, m_height, m_regions.size(), getFillRate() * 100.0f);

                    delete builder;
                    return true;
                }

                const TextureRegion* getRegion(const std::string& name) const override {
                    auto it = m_regions.find(name);
                    if (it != m_regions.end()) {
                        return &it->second;
                    }
                    return nullptr;
                }

                bool hasRegion(const std::string& name) const override {
                    return m_regions.find(name) != m_regions.end();
                }

                std::vector<std::string> getRegionNames() const override {
                    std::vector<std::string> names;
                    names.reserve(m_regions.size());
                    for (const auto& [name, _] : m_regions) {
                        names.push_back(name);
                    }
                    return names;
                }

                ITexture* getAtlasTexture() const override {
                    // 简化实现：返回 nullptr
                    // 实际应该创建 ITexture 接口的实现
                    return nullptr;
                }

                size_t getRegionCount() const override {
                    return m_regions.size();
                }

                uint32_t getWidth() const override {
                    return m_width;
                }

                uint32_t getHeight() const override {
                    return m_height;
                }

                void clear() override {
                    m_sourceTextures.clear();
                    m_regions.clear();
                    m_atlasData.clear();
                    m_width = 0;
                    m_height = 0;
                    m_needsRebuild = true;
                }

                bool saveToFile(const std::string& path) const override {
                    // 简化实现：需要 STB 库保存图片
                    LOG_WARNING("TextureAtlas", "保存图集功能暂未实现: {}", path);
                    return false;
                }

                float getFillRate() const override {
                    if (m_width == 0 || m_height == 0) return 0.0f;

                    size_t usedArea = 0;
                    for (const auto& [name, region] : m_regions) {
                        usedArea += region.width * region.height;
                    }

                    size_t totalArea = m_width * m_height;
                    return totalArea > 0 ? static_cast<float>(usedArea) / totalArea : 0.0f;
                }

            private:
                AtlasConfig m_config;
                std::unordered_map<std::string, SourceTexture> m_sourceTextures;
                std::unordered_map<std::string, TextureRegion> m_regions;
                std::vector<uint8_t> m_atlasData;
                uint32_t m_width = 0;
                uint32_t m_height = 0;
                bool m_needsRebuild = false;
            };
        }

        std::unique_ptr<TextureAtlas> TextureAtlas::create(const AtlasConfig& config) {
            return std::unique_ptr<TextureAtlas>(new TextureAtlasImpl(config));
        }

        // ============================================================================
        // BlockTextureManager 实现
        // ============================================================================

        namespace {
            class BlockTextureManagerImpl : public BlockTextureManager {
            public:
                explicit BlockTextureManagerImpl(const AtlasConfig& config)
                    : m_atlas(TextureAtlas::create(config)) {
                    LOG_INFO("BlockTextureManager", "创建方块纹理管理器");
                }

                ~BlockTextureManagerImpl() override = default;

                void registerBlock(const BlockTexture& texture) override {
                    m_blocks[texture.name] = texture;

                    // 添加所有面的纹理到图集
                    std::vector<std::pair<std::string, std::string>> texturesToLoad;

                    if (m_loadedTextures.find(texture.top) == m_loadedTextures.end()) {
                        texturesToLoad.push_back({"", texture.top});
                        m_loadedTextures.insert(texture.top);
                    }
                    if (m_loadedTextures.find(texture.bottom) == m_loadedTextures.end()) {
                        texturesToLoad.push_back({"", texture.bottom});
                        m_loadedTextures.insert(texture.bottom);
                    }
                    if (m_loadedTextures.find(texture.north) == m_loadedTextures.end()) {
                        texturesToLoad.push_back({"", texture.north});
                        m_loadedTextures.insert(texture.north);
                    }
                    if (m_loadedTextures.find(texture.south) == m_loadedTextures.end()) {
                        texturesToLoad.push_back({"", texture.south});
                        m_loadedTextures.insert(texture.south);
                    }
                    if (m_loadedTextures.find(texture.west) == m_loadedTextures.end()) {
                        texturesToLoad.push_back({"", texture.west});
                        m_loadedTextures.insert(texture.west);
                    }
                    if (m_loadedTextures.find(texture.east) == m_loadedTextures.end()) {
                        texturesToLoad.push_back({"", texture.east});
                        m_loadedTextures.insert(texture.east);
                    }

                    // 注意：实际应该从文件路径加载，这里简化为添加占位符
                }

                void registerBlocks(const std::vector<BlockTexture>& textures) override {
                    for (const auto& texture : textures) {
                        registerBlock(texture);
                    }
                }

                bool buildAtlas() override {
                    return m_atlas->build();
                }

                glm::vec4 getFaceUV(const std::string& blockName, FaceDirection direction) const override {
                    const TextureRegion* region = getFaceRegion(blockName, direction);
                    if (region) {
                        return glm::vec4(region->u0, region->v0, region->u1, region->v1);
                    }
                    return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);  // 默认 UV
                }

                const TextureRegion* getFaceRegion(const std::string& blockName, FaceDirection direction) const override {
                    auto it = m_blocks.find(blockName);
                    if (it == m_blocks.end()) {
                        return nullptr;
                    }

                    const BlockTexture& block = it->second;
                    std::string textureName;

                    switch (direction) {
                        case FaceDirection::TOP:    textureName = block.top; break;
                        case FaceDirection::BOTTOM: textureName = block.bottom; break;
                        case FaceDirection::NORTH:  textureName = block.north; break;
                        case FaceDirection::SOUTH:  textureName = block.south; break;
                        case FaceDirection::WEST:   textureName = block.west; break;
                        case FaceDirection::EAST:   textureName = block.east; break;
                        case FaceDirection::ALL:    textureName = block.top; break;
                        default:                    textureName = block.top; break;
                    }

                    return m_atlas->getRegion(textureName);
                }

                ITexture* getAtlasTexture() const override {
                    return m_atlas->getAtlasTexture();
                }

                bool hasBlock(const std::string& blockName) const override {
                    return m_blocks.find(blockName) != m_blocks.end();
                }

                size_t getBlockCount() const override {
                    return m_blocks.size();
                }

                static void registerDefaultBlocks(BlockTextureManager* manager) {
                    manager->registerBlocks(getDefaultBlocks());
                }

            private:
                static const std::vector<BlockTexture> getDefaultBlocks() {
                    return {
                        BlockTexture("grass_block_top", "grass_block_top", "dirt", "grass_side", "grass_side", "grass_side", "grass_side"),
                        BlockTexture("dirt"),
                        BlockTexture("stone"),
                        BlockTexture("cobblestone"),
                        BlockTexture("oak_log", "oak_log_top", "oak_log_top", "oak_log", "oak_log", "oak_log", "oak_log"),
                        BlockTexture("oak_leaves"),
                        BlockTexture("oak_planks"),
                        BlockTexture("crafting_table_top", "crafting_table_top", "crafting_table_bottom", "crafting_table_side", "crafting_table_side", "crafting_table_front", "crafting_table_side"),
                        BlockTexture("furnace_top", "furnace_top", "furnace_bottom", "furnace_side", "furnace_front", "furnace_side", "furnace_side"),
                    };
                }

                std::unique_ptr<TextureAtlas> m_atlas;
                std::unordered_map<std::string, BlockTexture> m_blocks;
                std::unordered_set<std::string> m_loadedTextures;
            };
        }

        std::unique_ptr<BlockTextureManager> BlockTextureManager::create(const AtlasConfig& config) {
            return std::unique_ptr<BlockTextureManager>(new BlockTextureManagerImpl(config));
        }

        // ============================================================================
        // TextureAtlasBuilder 实现
        // ============================================================================

        namespace {
            class TextureAtlasBuilderImpl : public TextureAtlasBuilder {
            public:
                TextureAtlasBuilderImpl(PackingAlgorithm algorithm)
                    : m_algorithm(algorithm) {
                    LOG_INFO("TextureAtlasBuilder", "创建纹理图集构建器，算法: {}",
                             algorithm == PackingAlgorithm::SKYLINE ? "Skyline" : "Basic");
                }

                ~TextureAtlasBuilderImpl() override = default;

                void setPackingAlgorithm(PackingAlgorithm algorithm) override {
                    m_algorithm = algorithm;
                }

                bool addSourceTexture(const std::vector<uint8_t>& imageData,
                                     uint32_t width, uint32_t height,
                                     const std::string& name) override {
                    m_sourceTextures.emplace(name, SourceTexture(imageData, width, height, 4, name));
                    return true;
                }

                bool addSourceTexture(const std::string& filePath, const std::string& name) override {
                    PrismaEngine::TextureAsset asset;
                    if (!asset.Load(filePath)) {
                        LOG_ERROR("TextureAtlasBuilder", "加载纹理失败: {}", filePath);
                        return false;
                    }

                    return addSourceTexture(asset.GetData(), asset.GetWidth(), asset.GetHeight(), name);
                }

                BuildResult build(const AtlasConfig& config) override {
                    BuildResult result;

                    if (m_sourceTextures.empty()) {
                        result.errorMessage = "没有源纹理";
                        return result;
                    }

                    // 尝试不同的图集尺寸
                    std::vector<uint32_t> sizesToTry = {1024, 2048, 4096, 8192};

                    for (uint32_t atlasSize : sizesToTry) {
                        if (atlasSize > config.maxWidth || atlasSize > config.maxHeight) {
                            continue;
                        }

                        result = tryBuild(atlasSize, atlasSize, config);
                        if (result.success) {
                            LOG_INFO("TextureAtlasBuilder", "图集构建成功: {}x", atlasSize);
                            return result;
                        }
                    }

                    result.errorMessage = "无法找到合适的图集尺寸";
                    return result;
                }

                void clear() override {
                    m_sourceTextures.clear();
                }

                size_t getSourceTextureCount() const override {
                    return m_sourceTextures.size();
                }

            private:
                BuildResult tryBuild(uint32_t atlasWidth, uint32_t atlasHeight, const AtlasConfig& config) {
                    BuildResult result;
                    result.atlasWidth = atlasWidth;
                    result.atlasHeight = atlasHeight;

                    // 创建图集数据
                    result.atlasData.resize(atlasWidth * atlasHeight * 4);
                    std::fill(result.atlasData.begin(), result.atlasData.end(), 0);

                    // 使用 Skyline 打包器
                    SkylinePacker packer(atlasWidth, atlasHeight, config.padding);

                    // 打包所有纹理
                    for (auto& [name, source] : m_sourceTextures) {
                        auto [x, y] = packer.insert(source.width, source.height);

                        if (x < 0 || y < 0) {
                            // 无法放入
                            result.success = false;
                            result.errorMessage = "图集空间不足";
                            return result;
                        }

                        // 复制纹理数据到图集
                        copyTextureToAtlas(result.atlasData, atlasWidth, source.data, source.width, source.height, x, y);

                        // 创建纹理区域
                        float u0 = static_cast<float>(x) / atlasWidth;
                        float v0 = static_cast<float>(y) / atlasHeight;
                        float u1 = static_cast<float>(x + source.width) / atlasWidth;
                        float v1 = static_cast<float>(y + source.height) / atlasHeight;

                        result.regions[name] = TextureRegion(u0, v0, u1, v1, source.width, source.height, x, y);
                        result.regions[name].name = name;
                    }

                    result.success = true;
                    return result;
                }

                void copyTextureToAtlas(std::vector<uint8_t>& atlasData, uint32_t atlasWidth,
                                       const std::vector<uint8_t>& textureData, uint32_t texWidth, uint32_t texHeight,
                                       uint32_t destX, uint32_t destY) {
                    for (uint32_t y = 0; y < texHeight; ++y) {
                        for (uint32_t x = 0; x < texWidth; ++x) {
                            uint32_t atlasIndex = ((destY + y) * atlasWidth + (destX + x)) * 4;
                            uint32_t texIndex = (y * texWidth + x) * 4;

                            atlasData[atlasIndex + 0] = textureData[texIndex + 0];  // R
                            atlasData[atlasIndex + 1] = textureData[texIndex + 1];  // G
                            atlasData[atlasIndex + 2] = textureData[texIndex + 2];  // B
                            atlasData[atlasIndex + 3] = textureData[texIndex + 3];  // A
                        }
                    }
                }

                PackingAlgorithm m_algorithm;
                std::unordered_map<std::string, SourceTexture> m_sourceTextures;
            };
        }

        std::unique_ptr<TextureAtlasBuilder> TextureAtlasBuilderFactory::create(TextureAtlasBuilder::PackingAlgorithm algorithm) {
            return std::unique_ptr<TextureAtlasBuilder>(new TextureAtlasBuilderImpl(algorithm));
        }

    } // namespace Graphic
} // namespace PrismaEngine
