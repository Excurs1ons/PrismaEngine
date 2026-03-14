#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <utility>

namespace Prisma {
    namespace Graphic {

        // 前向声明
        class ITexture;

        /**
         * @brief 纹理区域 - 纹理图集中单个纹理的 UV 坐标
         */
        struct TextureRegion {
            float u0, v0;  // 左上角 UV
            float u1, v1;  // 右下角 UV

            // 纹理信息
            uint32_t width;
            uint32_t height;
            uint32_t offsetX;  // 在图集中的偏移
            uint32_t offsetY;
            std::string name;

            TextureRegion()
                : u0(0), v0(0), u1(1), v1(1), width(0), height(0), offsetX(0), offsetY(0) {}

            TextureRegion(float u0, float v0, float u1, float v1,
                          uint32_t width, uint32_t height,
                          uint32_t offsetX = 0, uint32_t offsetY = 0)
                : u0(u0), v0(v0), u1(u1), v1(v1)
                , width(width), height(height), offsetX(offsetX), offsetY(offsetY) {}

            /**
             * @brief 获取 UV 坐标（vec2 版本）
             */
            glm::vec2 getMin() const { return glm::vec2(u0, v0); }
            glm::vec2 getMax() const { return glm::vec2(u1, v1); }

            /**
             * @brief 计算纹理的宽高比
             */
            float getAspectRatio() const {
                return height > 0 ? static_cast<float>(width) / height : 1.0f;
            }
        };

        /**
         * @brief 纹理图集配置
         */
        struct AtlasConfig {
            uint32_t maxWidth = 4096;      // 图集最大宽度
            uint32_t maxHeight = 4096;     // 图集最大高度
            uint32_t padding = 1;           // 纹理之间的填充（防止溢出）
            bool generateMipmaps = true;    // 是否生成 Mipmap
            bool useLinearFilter = true;    // 是否使用线性过滤
            bool powerOfTwo = true;         // 是否使用 2 的幂次尺寸

            // 纹理格式
            enum class Format {
                RGBA8,          // 8-bit RGBA
                RGB8,           // 8-bit RGB
                RGBA16F,        // 16-bit float RGBA
                RGB16F,         // 16-bit float RGB
                R8,             // 8-bit 单通道
                RG8             // 8-bit 双通道
            };

            Format format = Format::RGBA8;
        };

        /**
         * @brief 纹理图集 - 将多个小纹理合并到一张大纹理
         *
         * 优点：
         * - 减少 draw call（批处理）
         * - 减少纹理切换
         * - 更好的缓存利用率
         *
         * 对应 Minecraft 的方块纹理图集系统
         */
        class TextureAtlas {
        public:
            /**
             * @brief 创建空的纹理图集
             */
            static std::unique_ptr<TextureAtlas> create(const AtlasConfig& config = AtlasConfig());

            virtual ~TextureAtlas() = default;

            /**
             * @brief 添加纹理到图集
             * @param texturePath 纹理文件路径
             * @param name 纹理名称（用于查询）
             * @return 是否成功添加
             */
            virtual bool addTexture(const std::string& texturePath, const std::string& name) = 0;

            /**
             * @brief 批量添加纹理
             */
            virtual size_t addTextures(const std::vector<std::pair<std::string, std::string>>& textures) = 0;

            /**
             * @brief 构建/重建图集
             * @return 是否成功构建
             */
            virtual bool build() = 0;

            /**
             * @brief 获取纹理区域
             * @param name 纹理名称
             * @return 纹理区域，如果不存在返回 nullptr
             */
            virtual const TextureRegion* getRegion(const std::string& name) const = 0;

            /**
             * @brief 检查是否包含纹理
             */
            virtual bool hasRegion(const std::string& name) const = 0;

            /**
             * @brief 获取所有纹理名称
             */
            virtual std::vector<std::string> getRegionNames() const = 0;

            /**
             * @brief 获取图集纹理
             */
            virtual ITexture* getAtlasTexture() const = 0;

            /**
             * @brief 获取纹理数量
             */
            virtual size_t getRegionCount() const = 0;

            /**
             * @brief 获取图集大小
             */
            virtual uint32_t getWidth() const = 0;
            virtual uint32_t getHeight() const = 0;

            /**
             * @brief 清空图集
             */
            virtual void clear() = 0;

            /**
             * @brief 保存图集到文件
             */
            virtual bool saveToFile(const std::string& path) const = 0;

            /**
             * @brief 获取填充率（已使用面积 / 总面积）
             */
            virtual float getFillRate() const = 0;
        };

        /**
         * @brief 方块纹理管理器 - 专门为方块优化的纹理图集
         *
         * 对应 Minecraft 的方块纹理系统
         */
        class BlockTextureManager {
        public:
            /**
             * @brief 方块面方向
             */
            enum class FaceDirection {
                TOP,        // 上方 (+Y)
                BOTTOM,     // 下方 (-Y)
                NORTH,      // 北方 (-Z)
                SOUTH,      // 南方 (+Z)
                WEST,       // 西方 (-X)
                EAST,       // 东方 (+X)
                ALL         // 所有方向
            };

            /**
             * @brief 方块纹理定义
             */
            struct BlockTexture {
                std::string name;           // 纹理名称
                std::string top;           // 上方纹理
                std::string bottom;        // 下方纹理
                std::string north;         // 北方纹理
                std::string south;         // 南方纹理
                std::string west;          // 西方纹理
                std::string east;          // 东方纹理

                // 默认构造
                BlockTexture() = default;

                // 默认构造：所有面使用相同纹理
                BlockTexture(const std::string& textureName)
                    : name(textureName)
                    , top(textureName), bottom(textureName)
                    , north(textureName), south(textureName)
                    , west(textureName), east(textureName) {}

                // 完整构造
                BlockTexture(const std::string& name,
                            const std::string& top, const std::string& bottom,
                            const std::string& north, const std::string& south,
                            const std::string& west, const std::string& east)
                    : name(name), top(top), bottom(bottom)
                    , north(north), south(south), west(west), east(east) {}
            };

            virtual ~BlockTextureManager() = default;

            /**
             * @brief 创建方块纹理管理器
             */
            static std::unique_ptr<BlockTextureManager> create(const AtlasConfig& config = AtlasConfig());

            /**
             * @brief 注册方块纹理
             * @param texture 方块纹理定义
             */
            virtual void registerBlock(const BlockTexture& texture) = 0;

            /**
             * @brief 批量注册方块
             */
            virtual void registerBlocks(const std::vector<BlockTexture>& textures) = 0;

            /**
             * @brief 构建方块纹理图集
             */
            virtual bool buildAtlas() = 0;

            /**
             * @brief 获取方块指定面的 UV 坐标
             * @param blockName 方块名称
             * @param direction 面方向
             * @return UV 坐标，如果不存在返回 (0,0)-(1,1)
             */
            virtual glm::vec4 getFaceUV(const std::string& blockName, FaceDirection direction) const = 0;

            /**
             * @brief 获取方块纹理区域
             */
            virtual const TextureRegion* getFaceRegion(const std::string& blockName, FaceDirection direction) const = 0;

            /**
             * @brief 获取图集纹理
             */
            virtual ITexture* getAtlasTexture() const = 0;

            /**
             * @brief 检查是否已注册方块
             */
            virtual bool hasBlock(const std::string& blockName) const = 0;

            /**
             * @brief 获取已注册方块数量
             */
            virtual size_t getBlockCount() const = 0;

            /**
             * @brief 预定义常用方块纹理
             */
            static void registerDefaultBlocks(BlockTextureManager* manager);

        private:
            /**
             * @brief 内置方块纹理列表
             */
            static const std::vector<BlockTexture> getDefaultBlocks();
        };

        /**
         * @brief 纹理图集构建器 - 实现纹理打包算法
         *
         * 使用贪心算法或 Skyline 算法进行纹理打包
         */
        class TextureAtlasBuilder {
        public:
            /**
             * @brief 打包算法类型
             */
            enum class PackingAlgorithm {
                BASIC,           // 基础行填充
                GREEDY,          // 贪心算法
                SKYLINE,         // Skyline 算法（更高效）
                SHLF,            // Shelf 算法
            };

            /**
             * @brief 构建结果
             */
            struct BuildResult {
                bool success = false;
                uint32_t atlasWidth = 0;
                uint32_t atlasHeight = 0;
                std::unordered_map<std::string, TextureRegion> regions;
                std::vector<uint8_t> atlasData;
                std::string errorMessage;
            };

            virtual ~TextureAtlasBuilder() = default;

            /**
             * @brief 设置打包算法
             */
            virtual void setPackingAlgorithm(PackingAlgorithm algorithm) = 0;

            /**
             * @brief 添加源纹理
             * @param imageData 纹理数据（RGBA8 格式）
             * @param width 宽度
             * @param height 高度
             * @param name 纹理名称
             * @return 是否成功添加
             */
            virtual bool addSourceTexture(const std::vector<uint8_t>& imageData,
                                         uint32_t width, uint32_t height,
                                         const std::string& name) = 0;

            /**
             * @brief 从文件添加纹理
             */
            virtual bool addSourceTexture(const std::string& filePath, const std::string& name) = 0;

            /**
             * @brief 构建图集
             * @param config 图集配置
             * @return 构建结果
             */
            virtual BuildResult build(const AtlasConfig& config) = 0;

            /**
             * @brief 清空所有源纹理
             */
            virtual void clear() = 0;

            /**
             * @brief 获取源纹理数量
             */
            virtual size_t getSourceTextureCount() const = 0;
        };

        /**
         * @brief 纹理图集构建器工厂
         */
        class TextureAtlasBuilderFactory {
        public:
            /**
             * @brief 创建纹理图集构建器
             */
            static std::unique_ptr<TextureAtlasBuilder> create(
                TextureAtlasBuilder::PackingAlgorithm algorithm = TextureAtlasBuilder::PackingAlgorithm::SKYLINE
            );
        };

    } // namespace Graphic
} // namespace Prisma
