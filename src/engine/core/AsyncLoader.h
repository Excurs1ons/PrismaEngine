#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <future>
#include <unordered_map>
#include <string>
#include <exception>

namespace PrismaEngine {
    namespace Core {

        /**
         * @brief 异步任务状态
         */
        enum class TaskStatus {
            PENDING,     // 等待执行
            RUNNING,     // 正在执行
            COMPLETED,   // 已完成
            FAILED,      // 失败
            CANCELLED    // 已取消
        };

        /**
         * @brief 异步任务基类
         */
        class AsyncTask {
        public:
            virtual ~AsyncTask() = default;

            /**
             * @brief 执行任务
             */
            virtual void execute() = 0;

            /**
             * @brief 获取任务状态
             */
            virtual TaskStatus getStatus() const = 0;

            /**
             * @brief 获取进度 (0.0 - 1.0)
             */
            virtual float getProgress() const = 0;

            /**
             * @brief 取消任务
             */
            virtual void cancel() = 0;

            /**
             * @brief 获取任务 ID
             */
            virtual uint64_t getId() const = 0;

            /**
             * @brief 获取任务名称
             */
            virtual const std::string& getName() const = 0;

            /**
             * @brief 检查是否完成
             */
            virtual bool isCompleted() const = 0;

            /**
             * @brief 检查是否失败
             */
            virtual bool isFailed() const = 0;

            /**
             * @brief 获取错误信息
             */
            virtual const std::string& getError() const = 0;
        };

        /**
         * @brief 异步任务模板
         */
        template<typename T>
        class TemplatedAsyncTask : public AsyncTask {
        public:
            using ResultType = T;
            using ExecuteFunc = std::function<T()>;
            using ProgressCallback = std::function<void(float)>;

            /**
             * @brief 创建异步任务
             * @param name 任务名称
             * @param func 执行函数
             */
            static std::unique_ptr<TemplatedAsyncTask> create(const std::string& name, ExecuteFunc func) {
                return std::unique_ptr<TemplatedAsyncTask>(new TemplatedAsyncTask(name, std::move(func)));
            }

            /**
             * @brief 执行任务
             */
            void execute() override {
                m_status = TaskStatus::RUNNING;

                try {
                    m_result = m_executeFunc();
                    m_status = TaskStatus::COMPLETED;
                } catch (const std::exception& e) {
                    m_status = TaskStatus::FAILED;
                    m_error = e.what();
                }
            }

            /**
             * @brief 获取任务状态
             */
            TaskStatus getStatus() const override {
                return m_status;
            }

            /**
             * @brief 获取进度
             */
            float getProgress() const override {
                return m_progress.load();
            }

            /**
             * @brief 设置进度
             */
            void setProgress(float progress) {
                m_progress.store(progress);
            }

            /**
             * @brief 取消任务
             */
            void cancel() override {
                m_status = TaskStatus::CANCELLED;
            }

            /**
             * @brief 获取任务 ID
             */
            uint64_t getId() const override {
                return m_id;
            }

            /**
             * @brief 获取任务名称
             */
            const std::string& getName() const override {
                return m_name;
            }

            /**
             * @brief 检查是否完成
             */
            bool isCompleted() const override {
                return m_status == TaskStatus::COMPLETED;
            }

            /**
             * @brief 检查是否失败
             */
            bool isFailed() const override {
                return m_status == TaskStatus::FAILED;
            }

            /**
             * @brief 获取错误信息
             */
            const std::string& getError() const override {
                return m_error;
            }

            /**
             * @brief 获取结果
             */
            const T& getResult() const {
                return m_result;
            }

            /**
             * @brief 设置进度回调
             */
            void setProgressCallback(ProgressCallback callback) {
                m_progressCallback = std::move(callback);
            }

        private:
            TemplatedAsyncTask(const std::string& name, ExecuteFunc func)
                : m_name(name)
                , m_executeFunc(std::move(func))
                , m_id(generateId())
                , m_status(TaskStatus::PENDING) {}

            static uint64_t generateId() {
                static std::atomic<uint64_t> s_idCounter{0};
                return s_idCounter.fetch_add(1);
            }

            std::string m_name;
            ExecuteFunc m_executeFunc;
            uint64_t m_id;
            std::atomic<TaskStatus> m_status{TaskStatus::PENDING};
            std::atomic<float> m_progress{0.0f};
            std::string m_error;
            T m_result;
            ProgressCallback m_progressCallback;
        };

        /**
         * @brief 线程池 - 用于异步任务执行
         */
        class ThreadPool {
        public:
            /**
             * @brief 创建线程池
             * @param numThreads 线程数量（0 表示使用 CPU 核心数）
             */
            static std::unique_ptr<ThreadPool> create(size_t numThreads = 0);

            virtual ~ThreadPool() = default;

            /**
             * @brief 提交任务到线程池
             * @param task 异步任务
             * @return 任务 ID
             */
            virtual uint64_t submit(std::unique_ptr<AsyncTask> task) = 0;

            /**
             * @brief 提交函数任务
             * @param name 任务名称
             * @param func 执行函数
             * @return 任务 ID
             */
            virtual uint64_t submit(const std::string& name, std::function<void()> func) = 0;

            /**
             * @brief 提交带返回值的任务
             * @tparam ResultType 返回值类型
             */
            template<typename ResultType>
            uint64_t submit(const std::string& name, std::function<ResultType()> func) {
                auto task = TemplatedAsyncTask<ResultType>::create(name, std::move(func));
                return submit(std::unique_ptr<AsyncTask>(task.release()));
            }

            /**
             * @brief 等待任务完成
             * @param taskId 任务 ID
             * @param timeoutMs 超时时间（毫秒），0 表示无限等待
             * @return 是否完成
             */
            virtual bool waitForTask(uint64_t taskId, uint32_t timeoutMs = 0) = 0;

            /**
             * @brief 获取任务
             */
            virtual AsyncTask* getTask(uint64_t taskId) = 0;

            /**
             * @brief 取消任务
             */
            virtual bool cancelTask(uint64_t taskId) = 0;

            /**
             * @brief 更新线程池（处理完成的任务）
             * @param maxCallbacks 最大回调数量
             * @return 处理的任务数量
             */
            virtual size_t update(size_t maxCallbacks = 100) = 0;

            /**
             * @brief 获取线程数量
             */
            virtual size_t getThreadCount() const = 0;

            /**
             * @brief 获取待处理任务数量
             */
            virtual size_t getPendingTaskCount() const = 0;

            /**
             * @brief 暂停线程池（停止处理新任务）
             */
            virtual void pause() = 0;

            /**
             * @brief 恢复线程池
             */
            virtual void resume() = 0;

            /**
             * @brief 关闭线程池（等待所有任务完成）
             * @param waitForTasks 是否等待当前任务完成
             */
            virtual void shutdown(bool waitForTasks = true) = 0;

            /**
             * @brief 强制关闭（取消所有任务）
             */
            virtual void shutdownNow() = 0;

            /**
             * @brief 检查是否已关闭
             */
            virtual bool isShutdown() const = 0;
        };

        /**
         * @brief 异步资源加载器
         *
         * 专门用于加载游戏资源（纹理、模型、音频等）
         */
        class AsyncResourceLoader {
        public:
            /**
             * @brief 加载结果
             */
            template<typename T>
            struct LoadResult {
                bool success = false;
                std::string error;
                std::shared_ptr<T> resource;
                uint64_t taskId = 0;

                static LoadResult createSuccess(std::shared_ptr<T> resource, uint64_t taskId) {
                    LoadResult result;
                    result.success = true;
                    result.resource = resource;
                    result.taskId = taskId;
                    return result;
                }

                static LoadResult createFailure(const std::string& error, uint64_t taskId) {
                    LoadResult result;
                    result.success = false;
                    result.error = error;
                    result.taskId = taskId;
                    return result;
                }
            };

            /**
             * @brief 加载回调
             */
            template<typename T>
            using LoadCallback = std::function<void(LoadResult<T>)>;

            virtual ~AsyncResourceLoader() = default;

            /**
             * @brief 创建异步资源加载器
             * @param threadPool 线程池（如果为 null 则自动创建）
             */
            static std::unique_ptr<AsyncResourceLoader> create(ThreadPool* threadPool = nullptr);

            /**
             * @brief 异步加载纹理
             * @param filePath 文件路径
             * @param callback 加载完成回调
             * @return 任务 ID
             */
            virtual uint64_t loadTexture(const std::string& filePath,
                                         std::function<void(std::shared_ptr<class ITexture>)> callback) = 0;

            /**
             * @brief 异步加载模型
             * @param filePath 文件路径
             * @param callback 加载完成回调
             * @return 任务 ID
             */
            virtual uint64_t loadModel(const std::string& filePath,
                                       std::function<void(std::shared_ptr<class IMesh>)> callback) = 0;

            /**
             * @brief 批量加载纹理
             * @param filePaths 文件路径列表
             * @param callback 加载完成回调（每个纹理完成后调用）
             * @return 任务 ID 列表
             */
            virtual std::vector<uint64_t> loadTextures(
                const std::vector<std::string>& filePaths,
                std::function<void(size_t index, std::shared_ptr<class ITexture>)> callback
            ) = 0;

            /**
             * @brief 加载进度
             */
            struct LoadingProgress {
                size_t totalTasks = 0;
                size_t completedTasks = 0;
                size_t failedTasks = 0;
                float progress = 0.0f;
            };

            virtual LoadingProgress getProgress() const = 0;

            /**
             * @brief 取消所有任务
             */
            virtual void cancelAll() = 0;

            /**
             * @brief 更新加载器（处理完成的任务）
             */
            virtual void update() = 0;

            /**
             * @brief 检查是否正在加载
             */
            virtual bool isLoading() const = 0;
        };

        /**
         * @brief 区块生成系统 - 多线程区块数据生成
         *
         * 专门为 PrismaCraft 的区块生成优化
         */
        class ChunkGenerationSystem {
        public:
            /**
             * @brief 区块位置
             */
            struct ChunkPos {
                int32_t x, z;

                bool operator==(const ChunkPos& other) const {
                    return x == other.x && z == other.z;
                }
            };

            /**
             * @brief ChunkPos 哈希函数
             */
            struct HashChunkPos {
                size_t operator()(const ChunkPos& pos) const noexcept {
                    return static_cast<size_t>(pos.x) * 31 + static_cast<size_t>(pos.z);
                }
            };

            /**
             * @brief 区块数据
             */
            struct ChunkData {
                ChunkPos position;
                std::vector<uint8_t> blockData;      // 方块类型数据
                std::vector<uint8_t> metadataData;    // 元数据
                std::vector<uint8_t> biomeData;       // 生物群系数据

                bool isEmpty() const {
                    return blockData.empty();
                }
            };

            /**
             * @brief 区块生成任务
             */
            struct ChunkGenerationTask {
                ChunkPos position;
                int32_t seed;
                uint64_t taskId;
            };

            /**
             * @brief 区块生成回调
             */
            using ChunkCallback = std::function<void(const ChunkData&)>;

            /**
             * @brief 创建区块生成系统
             * @param numThreads 生成线程数（0 表示使用 CPU 核心数）
             */
            static std::unique_ptr<ChunkGenerationSystem> create(size_t numThreads = 0);

            virtual ~ChunkGenerationSystem() = default;

            /**
             * @brief 设置世界种子
             */
            virtual void setSeed(int32_t seed) = 0;

            /**
             * @brief 队列区块生成任务
             * @param tasks 区块任务列表
             */
            virtual void queueChunks(const std::vector<ChunkGenerationTask>& tasks) = 0;

            /**
             * @brief 队列单个区块生成
             */
            virtual void queueChunk(int32_t chunkX, int32_t chunkZ) = 0;

            /**
             * @brief 取消区块生成
             */
            virtual void cancelChunk(int32_t chunkX, int32_t chunkZ) = 0;

            /**
             * @brief 取消所有生成任务
             */
            virtual void cancelAll() = 0;

            /**
             * @brief 设置生成完成回调
             */
            virtual void setCompletionCallback(ChunkCallback callback) = 0;

            /**
             * @brief 更新系统（处理完成的区块）
             * @return 完成的区块数量
             */
            virtual size_t update() = 0;

            /**
             * @brief 获取生成进度
             */
            virtual float getProgress() const = 0;

            /**
             * @brief 获取待处理任务数量
             */
            virtual size_t getPendingTaskCount() const = 0;

            /**
             * @brief 关闭系统
             */
            virtual void shutdown() = 0;
        };

        /**
         * @brief 区块网格构建系统 - 多线程网格生成
         *
         * 专门为 PrismaCraft 的区块网格构建优化
         */
        class ChunkMeshBuildSystem {
        public:
            /**
             * @brief 网格数据
             */
            struct MeshData {
                std::vector<float> vertices;
                std::vector<uint32_t> indices;
                uint32_t vertexCount = 0;
                uint32_t indexCount = 0;

                bool isEmpty() const {
                    return vertices.empty();
                }

                void clear() {
                    vertices.clear();
                    indices.clear();
                    vertexCount = 0;
                    indexCount = 0;
                }

                void reserve(size_t newVertexCount, size_t newIndexCount) {
                    vertices.reserve(newVertexCount * 8);  // 假设每个顶点 8 个 float
                    indices.reserve(newIndexCount);
                }
            };

            /**
             * @brief 网格构建任务
             */
            struct MeshBuildTask {
                ChunkGenerationSystem::ChunkPos chunkPos;
                ChunkGenerationSystem::ChunkData chunkData;
                uint64_t taskId;

                bool operator==(const MeshBuildTask& other) const {
                    return chunkPos.x == other.chunkPos.x && chunkPos.z == other.chunkPos.z;
                }
            };

            /**
             * @brief 网格构建回调
             */
            using MeshCallback = std::function<void(const ChunkGenerationSystem::ChunkPos&, const MeshData&)>;

            /**
             * @brief 创建区块网格构建系统
             * @param numThreads 构建线程数（0 表示使用 CPU 核心数）
             */
            static std::unique_ptr<ChunkMeshBuildSystem> create(size_t numThreads = 0);

            virtual ~ChunkMeshBuildSystem() = default;

            /**
             * @brief 队列网格构建任务
             * @param tasks 任务列表
             */
            virtual void queueMeshes(const std::vector<MeshBuildTask>& tasks) = 0;

            /**
             * @brief 队列单个网格构建
             */
            virtual void queueMesh(const ChunkGenerationSystem::ChunkPos& pos,
                                    const ChunkGenerationSystem::ChunkData& data) = 0;

            /**
             * @brief 重新构建区块网格（当区块数据改变时）
             */
            virtual void rebuildMesh(int32_t chunkX, int32_t chunkZ) = 0;

            /**
             * @brief 取消网格构建
             */
            virtual void cancelMesh(int32_t chunkX, int32_t chunkZ) = 0;

            /**
             * @brief 设置构建完成回调
             */
            virtual void setCompletionCallback(MeshCallback callback) = 0;

            /**
             * @brief 更新系统（处理完成的网格）
             * @return 完成的网格数量
             */
            virtual size_t update() = 0;

            /**
             * @brief 获取构建进度
             */
            virtual float getProgress() const = 0;

            /**
             * @brief 获取待处理任务数量
             */
            virtual size_t getPendingTaskCount() const = 0;

            /**
             * @brief 关闭系统
             */
            virtual void shutdown() = 0;

            /**
             * @brief 设置网格优化选项
             */
            struct MeshOptimizationOptions {
                bool enableFaceCulling = true;        // 启用面剔除
                bool enableGreedyMeshing = true;     // 启用 Greedy Meshing
                bool enableBackfaceCulling = true;   // 启用背面剔除
                bool enableAmbientOcclusion = false;  // 启用环境光遮蔽（高级）
            };

            virtual void setOptimizationOptions(const MeshOptimizationOptions& options) = 0;
        };

        /**
         * @brief 异步加载器 (单例)
         *
         * 引擎范围内统一的异步任务和资源加载入口
         */
        class AsyncLoader {
        public:
            static AsyncLoader& GetInstance();

            bool Initialize(size_t threadCount = 0);
            void Shutdown();
            void Update(size_t maxCallbacks = 100);

            ThreadPool* getThreadPool() { return m_threadPool.get(); }
            AsyncResourceLoader* getResourceLoader() { return m_resourceLoader.get(); }
            ChunkGenerationSystem* getChunkGenerator() { return m_chunkGenerator.get(); }
            ChunkMeshBuildSystem* getMeshBuilder() { return m_meshBuilder.get(); }

        private:
            AsyncLoader();
            ~AsyncLoader();

            std::unique_ptr<ThreadPool> m_threadPool;
            std::unique_ptr<AsyncResourceLoader> m_resourceLoader;
            std::unique_ptr<ChunkGenerationSystem> m_chunkGenerator;
            std::unique_ptr<ChunkMeshBuildSystem> m_meshBuilder;
        };

    } // namespace Core
} // namespace PrismaEngine

