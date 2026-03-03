#include "pch.h"
#include "AsyncLoader.h"
#include "Logger.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <sstream>

namespace PrismaEngine {
    namespace Core {

        // ============================================================================
        // ThreadPool 实现
        // ============================================================================

        namespace {
            class ThreadPoolImpl : public ThreadPool {
            public:
                explicit ThreadPoolImpl(size_t numThreads) {
                    if (numThreads == 0) {
                        numThreads = std::thread::hardware_concurrency();
                        if (numThreads == 0) numThreads = 4;  // 默认值
                    }

                    m_threads.reserve(numThreads);
                    for (size_t i = 0; i < numThreads; ++i) {
                        m_threads.emplace_back(&ThreadPoolImpl::workerThread, this);
                    }

                    LOG_INFO("ThreadPool", "创建线程池，线程数: {}", numThreads);
                }

                ~ThreadPoolImpl() override {
                    shutdownNow();
                }

                uint64_t submit(std::unique_ptr<AsyncTask> task) override {
                    if (!task) {
                        LOG_ERROR("ThreadPool", "提交空任务");
                        return 0;
                    }

                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    uint64_t taskId = task->getId();
                    m_taskQueue.push(std::move(task));
                    m_condition.notify_one();
                    return taskId;
                }

                uint64_t submit(const std::string& name, std::function<void()> func) override {
                    auto task = std::make_unique<FunctionTask>(name, std::move(func));
                    return submit(std::move(task));
                }

                bool waitForTask(uint64_t taskId, uint32_t timeoutMs) override {
                    // 简化实现：轮询任务状态
                    auto start = std::chrono::steady_clock::now();
                    while (true) {
                        {
                            std::lock_guard<std::mutex> lock(m_completedMutex);
                            if (m_completedTasks.find(taskId) != m_completedTasks.end()) {
                                return true;
                            }
                        }

                        if (timeoutMs > 0) {
                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                            if (elapsed >= timeoutMs) {
                                return false;
                            }
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }

                AsyncTask* getTask(uint64_t taskId) override {
                    std::lock_guard<std::mutex> lock(m_completedMutex);
                    auto it = m_completedTasks.find(taskId);
                    if (it != m_completedTasks.end()) {
                        return it->second.get();
                    }
                    return nullptr;
                }

                bool cancelTask(uint64_t taskId) override {
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    // 简化实现：只能取消队列中的任务
                    // 实际实现需要支持取消正在运行的任务
                    return false;
                }

                size_t update(size_t maxCallbacks) override {
                    std::lock_guard<std::mutex> lock(m_completedMutex);
                    size_t processed = 0;

                    while (!m_completedTasks.empty() && processed < maxCallbacks) {
                        // 任务已完成，可以被游戏逻辑处理
                        auto it = m_completedTasks.begin();
                        if (it != m_completedTasks.end()) {
                            m_completedTasks.erase(it);
                            ++processed;
                        }
                    }

                    return processed;
                }

                size_t getThreadCount() const override {
                    return m_threads.size();
                }

                size_t getPendingTaskCount() const override {
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    return m_taskQueue.size();
                }

                void pause() override {
                    m_paused = true;
                }

                void resume() override {
                    m_paused = false;
                    m_condition.notify_all();
                }

                void shutdown(bool waitForTasks) override {
                    {
                        std::lock_guard<std::mutex> lock(m_queueMutex);
                        m_shutdown = true;
                        m_waitForTasks = waitForTasks;
                    }
                    m_condition.notify_all();

                    for (auto& thread : m_threads) {
                        if (thread.joinable()) {
                            thread.join();
                        }
                    }
                    m_threads.clear();
                }

                void shutdownNow() override {
                    {
                        std::lock_guard<std::mutex> lock(m_queueMutex);
                        // 清空任务队列
                        while (!m_taskQueue.empty()) {
                            m_taskQueue.pop();
                        }
                        m_shutdown = true;
                        m_waitForTasks = false;
                    }
                    m_condition.notify_all();

                    for (auto& thread : m_threads) {
                        if (thread.joinable()) {
                            thread.join();
                        }
                    }
                    m_threads.clear();
                }

                bool isShutdown() const override {
                    return m_shutdown;
                }

            private:
                void workerThread() {
                    while (true) {
                        std::unique_ptr<AsyncTask> task;

                        {
                            std::unique_lock<std::mutex> lock(m_queueMutex);
                            m_condition.wait(lock, [this] {
                                return m_shutdown || !m_taskQueue.empty();
                            });

                            if (m_shutdown && !m_waitForTasks) return;
                            if (m_shutdown && m_taskQueue.empty()) return;

                            if (m_paused) {
                                m_condition.wait(lock, [this] { return !m_paused || m_shutdown; });
                                if (m_shutdown && !m_waitForTasks) return;
                            }

                            if (!m_taskQueue.empty()) {
                                task = std::move(m_taskQueue.front());
                                m_taskQueue.pop();
                            }
                        }

                        if (task) {
                            task->execute();

                            std::lock_guard<std::mutex> lock(m_completedMutex);
                            m_completedTasks[task->getId()] = std::move(task);
                        }
                    }
                }

                // 简单的函数任务包装器
                class FunctionTask : public AsyncTask {
                public:
                    FunctionTask(const std::string& name, std::function<void()> func)
                        : m_name(name), m_func(std::move(func)), m_id(generateId()),
                          m_status(TaskStatus::PENDING) {}

                    void execute() override {
                        m_status = TaskStatus::RUNNING;
                        try {
                            if (m_func) m_func();
                            m_status = TaskStatus::COMPLETED;
                        } catch (const std::exception& e) {
                            m_status = TaskStatus::FAILED;
                            m_error = e.what();
                        }
                    }

                    TaskStatus getStatus() const override { return m_status; }
                    float getProgress() const override { return m_status == TaskStatus::COMPLETED ? 1.0f : 0.0f; }
                    void cancel() override { m_status = TaskStatus::CANCELLED; }
                    uint64_t getId() const override { return m_id; }
                    const std::string& getName() const override { return m_name; }
                    bool isCompleted() const override { return m_status == TaskStatus::COMPLETED; }
                    bool isFailed() const override { return m_status == TaskStatus::FAILED; }
                    const std::string& getError() const override { return m_error; }

                private:
                    static uint64_t generateId() {
                        static std::atomic<uint64_t> s_idCounter{0};
                        return s_idCounter.fetch_add(1);
                    }

                    std::string m_name;
                    std::function<void()> m_func;
                    uint64_t m_id;
                    std::atomic<TaskStatus> m_status;
                    std::string m_error;
                };

                std::vector<std::thread> m_threads;
                std::queue<std::unique_ptr<AsyncTask>> m_taskQueue;
                mutable std::mutex m_queueMutex;
                std::condition_variable m_condition;
                std::atomic<bool> m_shutdown{false};
                std::atomic<bool> m_paused{false};
                std::atomic<bool> m_waitForTasks{true};

                std::unordered_map<uint64_t, std::unique_ptr<AsyncTask>> m_completedTasks;
                mutable std::mutex m_completedMutex;
            };
        }

        std::unique_ptr<ThreadPool> ThreadPool::create(size_t numThreads) {
            return std::unique_ptr<ThreadPool>(new ThreadPoolImpl(numThreads));
        }

        // ============================================================================
        // ChunkGenerationSystem 实现
        // ============================================================================

        namespace {
            class ChunkGenerationSystemImpl : public ChunkGenerationSystem {
            public:
                explicit ChunkGenerationSystemImpl(size_t numThreads)
                    : m_threadPool(ThreadPool::create(numThreads))
                    , m_seed(0) {

                    std::ostringstream oss;
                    oss << "ChunkGenerationSystem_" << std::this_thread::get_id();
                    m_name = oss.str();

                    LOG_INFO("ChunkGenerationSystem", "创建区块生成系统，线程数: {}", m_threadPool->getThreadCount());
                }

                ~ChunkGenerationSystemImpl() override {
                    shutdown();
                }

                void setSeed(int32_t seed) override {
                    m_seed = seed;
                }

                void queueChunks(const std::vector<ChunkGenerationTask>& tasks) override {
                    for (const auto& task : tasks) {
                        queueChunk(task.position.x, task.position.z);
                    }
                }

                void queueChunk(int32_t chunkX, int32_t chunkZ) override {
                    ChunkPos pos{chunkX, chunkZ};

                    {
                        std::lock_guard<std::mutex> lock(m_pendingMutex);
                        if (m_pendingChunks.find(pos) != m_pendingChunks.end()) {
                            return;  // 已经在队列中
                        }
                        m_pendingChunks.insert(pos);
                    }

                    std::string taskName = "GenerateChunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ);
                    m_threadPool->submit(taskName, [this, chunkX, chunkZ]() {
                        generateChunk(chunkX, chunkZ);
                    });
                }

                void cancelChunk(int32_t chunkX, int32_t chunkZ) override {
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    m_pendingChunks.erase({chunkX, chunkZ});
                }

                void cancelAll() override {
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    m_pendingChunks.clear();
                }

                void setCompletionCallback(ChunkCallback callback) override {
                    m_completionCallback = std::move(callback);
                }

                size_t update() override {
                    // 回调会在生成完成时立即调用
                    // 这里返回统计信息
                    std::lock_guard<std::mutex> lock(m_statsMutex);
                    size_t completed = m_completedCount;
                    m_completedCount = 0;
                    return completed;
                }

                float getProgress() const override {
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    if (m_totalQueued == 0) return 1.0f;
                    return 1.0f - (static_cast<float>(m_pendingChunks.size()) / m_totalQueued);
                }

                size_t getPendingTaskCount() const override {
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    return m_pendingChunks.size();
                }

                void shutdown() override {
                    cancelAll();
                    m_threadPool->shutdown(true);
                }

            private:
                void generateChunk(int32_t chunkX, int32_t chunkZ) {
                    ChunkData data;
                    data.position = {chunkX, chunkZ};

                    // 简单的地形生成 - 实际应该使用 Perlin 噪声
                    constexpr size_t CHUNK_SIZE = 16;
                    constexpr size_t CHUNK_HEIGHT = 256;
                    constexpr size_t SECTION_SIZE = 16 * 16 * 16;

                    data.blockData.resize(CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT);
                    data.biomeData.resize(CHUNK_SIZE * CHUNK_SIZE);

                    // 简单的平面生成（实际应该使用噪声函数）
                    for (size_t x = 0; x < CHUNK_SIZE; ++x) {
                        for (size_t z = 0; z < CHUNK_SIZE; ++z) {
                            // 计算世界坐标
                            int32_t worldX = chunkX * CHUNK_SIZE + x;
                            int32_t worldZ = chunkZ * CHUNK_SIZE + z;

                            // 简单的地形高度（实际应该使用 Perlin/Simplex 噪声）
                            int32_t height = 64;

                            // 填充方块数据
                            for (int32_t y = 0; y < height; ++y) {
                                size_t index = x + z * CHUNK_SIZE + y * CHUNK_SIZE * CHUNK_SIZE;

                                if (y < height - 4) {
                                    data.blockData[index] = 1;  // 石头
                                } else if (y < height - 1) {
                                    data.blockData[index] = 2;  // 泥土
                                } else {
                                    data.blockData[index] = 3;  // 草方块
                                }
                            }

                            // 生物群系（简化）
                            size_t biomeIndex = x + z * CHUNK_SIZE;
                            data.biomeData[biomeIndex] = 0;  // 平原
                        }
                    }

                    // 从待处理列表中移除
                    {
                        std::lock_guard<std::mutex> lock(m_pendingMutex);
                        m_pendingChunks.erase({chunkX, chunkZ});
                    }

                    // 调用完成回调
                    if (m_completionCallback) {
                        m_completionCallback(data);
                    }

                    // 更新统计
                    {
                        std::lock_guard<std::mutex> lock(m_statsMutex);
                        m_completedCount++;
                    }
                }

                std::unique_ptr<ThreadPool> m_threadPool;
                std::string m_name;
                int32_t m_seed;
                ChunkCallback m_completionCallback;

                struct HashChunkPos {
                    size_t operator()(const ChunkPos& pos) const {
                        return static_cast<size_t>(pos.x) * 31 + static_cast<size_t>(pos.z);
                    }
                };

                std::unordered_set<ChunkPos, HashChunkPos> m_pendingChunks;
                mutable std::mutex m_pendingMutex;

                std::atomic<size_t> m_completedCount{0};
                std::atomic<size_t> m_totalQueued{0};
                mutable std::mutex m_statsMutex;
            };
        }

        std::unique_ptr<ChunkGenerationSystem> ChunkGenerationSystem::create(size_t numThreads) {
            return std::unique_ptr<ChunkGenerationSystem>(new ChunkGenerationSystemImpl(numThreads));
        }

        // ============================================================================
        // ChunkMeshBuildSystem 实现
        // ============================================================================

        namespace {
            class ChunkMeshBuildSystemImpl : public ChunkMeshBuildSystem {
            public:
                explicit ChunkMeshBuildSystemImpl(size_t numThreads)
                    : m_threadPool(ThreadPool::create(numThreads)) {

                    std::ostringstream oss;
                    oss << "ChunkMeshBuildSystem_" << std::this_thread::get_id();
                    m_name = oss.str();

                    LOG_INFO("ChunkMeshBuildSystem", "创建区块网格构建系统，线程数: {}", m_threadPool->getThreadCount());
                }

                ~ChunkMeshBuildSystemImpl() override {
                    shutdown();
                }

                void queueMeshes(const std::vector<MeshBuildTask>& tasks) override {
                    for (const auto& task : tasks) {
                        queueMesh(task.chunkPos, task.chunkData);
                    }
                }

                void queueMesh(const ChunkGenerationSystem::ChunkPos& pos,
                              const ChunkGenerationSystem::ChunkData& data) override {
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    if (m_pendingMeshes.find(pos) != m_pendingMeshes.end()) {
                        return;  // 已经在队列中
                    }
                    m_pendingMeshes.insert(pos);

                    std::string taskName = "BuildMesh_" + std::to_string(pos.x) + "_" + std::to_string(pos.z);
                    m_threadPool->submit(taskName, [this, pos, data]() {
                        buildMesh(pos, data);
                    });
                }

                void rebuildMesh(int32_t chunkX, int32_t chunkZ) override {
                    // 重新构建需要重新获取区块数据
                    // 这里简化为直接从队列中移除，等待下一次构建
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    m_pendingMeshes.erase({chunkX, chunkZ});
                }

                void cancelMesh(int32_t chunkX, int32_t chunkZ) override {
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    m_pendingMeshes.erase({chunkX, chunkZ});
                }

                void setCompletionCallback(MeshCallback callback) override {
                    m_completionCallback = std::move(callback);
                }

                size_t update() override {
                    std::lock_guard<std::mutex> lock(m_statsMutex);
                    size_t completed = m_completedCount;
                    m_completedCount = 0;
                    return completed;
                }

                float getProgress() const override {
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    if (m_totalQueued == 0) return 1.0f;
                    return 1.0f - (static_cast<float>(m_pendingMeshes.size()) / m_totalQueued);
                }

                size_t getPendingTaskCount() const override {
                    std::lock_guard<std::mutex> lock(m_pendingMutex);
                    return m_pendingMeshes.size();
                }

                void shutdown() override {
                    m_threadPool->shutdown(true);
                }

                void setOptimizationOptions(const MeshOptimizationOptions& options) override {
                    m_optimizationOptions = options;
                }

            private:
                void buildMesh(const ChunkGenerationSystem::ChunkPos& pos,
                             const ChunkGenerationSystem::ChunkData& data) {
                    MeshData meshData;

                    // 简化的网格生成
                    constexpr size_t CHUNK_SIZE = 16;
                    constexpr size_t CHUNK_HEIGHT = 256;

                    // 预估顶点数量
                    meshData.reserve(4096, 6144);

                    // 遍历区块中的所有方块
                    for (size_t y = 0; y < CHUNK_HEIGHT; ++y) {
                        for (size_t z = 0; z < CHUNK_SIZE; ++z) {
                            for (size_t x = 0; x < CHUNK_SIZE; ++x) {
                                size_t index = x + z * CHUNK_SIZE + y * CHUNK_SIZE * CHUNK_SIZE;
                                uint8_t blockId = data.blockData[index];

                                if (blockId == 0) continue;  // 空气方块

                                // 检查每个面是否可见
                                addBlockFace(meshData, x, y, z, blockId, data,
                                            CHUNK_SIZE, CHUNK_HEIGHT);
                            }
                        }
                    }

                    meshData.vertexCount = meshData.vertices.size() / 8;  // 8 floats per vertex
                    meshData.indexCount = meshData.indices.size();

                    // 从待处理列表中移除
                    {
                        std::lock_guard<std::mutex> lock(m_pendingMutex);
                        m_pendingMeshes.erase(pos);
                    }

                    // 调用完成回调
                    if (m_completionCallback) {
                        m_completionCallback(pos, meshData);
                    }

                    // 更新统计
                    {
                        std::lock_guard<std::mutex> lock(m_statsMutex);
                        m_completedCount++;
                    }
                }

                void addBlockFace(MeshData& mesh, size_t x, size_t y, size_t z,
                                 uint8_t blockId, const ChunkGenerationSystem::ChunkData& data,
                                 size_t chunkSize, size_t chunkHeight) {
                    // 简化实现：添加所有 6 个面
                    // 实际应该检查相邻方块以进行面剔除

                    constexpr float CUBE_SIZE = 1.0f;
                    float fx = static_cast<float>(x);
                    float fy = static_cast<float>(y);
                    float fz = static_cast<float>(z);

                    // 顶点格式：position(3) + normal(3) + uv(2)
                    // 这里简化为：position(3) + uv(2)

                    // 前面
                    addFace(mesh,
                           glm::vec3(fx, fy, fz + CUBE_SIZE),
                           glm::vec3(fx + CUBE_SIZE, fy, fz + CUBE_SIZE),
                           glm::vec3(fx + CUBE_SIZE, fy + CUBE_SIZE, fz + CUBE_SIZE),
                           glm::vec3(fx, fy + CUBE_SIZE, fz + CUBE_SIZE),
                           0.0f, 0.0f, 1.0f);  // normal

                    // 后面
                    addFace(mesh,
                           glm::vec3(fx + CUBE_SIZE, fy, fz),
                           glm::vec3(fx, fy, fz),
                           glm::vec3(fx, fy + CUBE_SIZE, fz),
                           glm::vec3(fx + CUBE_SIZE, fy + CUBE_SIZE, fz),
                           0.0f, 0.0f, -1.0f);  // normal

                    // 左面
                    addFace(mesh,
                           glm::vec3(fx, fy, fz),
                           glm::vec3(fx, fy, fz + CUBE_SIZE),
                           glm::vec3(fx, fy + CUBE_SIZE, fz + CUBE_SIZE),
                           glm::vec3(fx, fy + CUBE_SIZE, fz),
                           -1.0f, 0.0f, 0.0f);  // normal

                    // 右面
                    addFace(mesh,
                           glm::vec3(fx + CUBE_SIZE, fy, fz + CUBE_SIZE),
                           glm::vec3(fx + CUBE_SIZE, fy, fz),
                           glm::vec3(fx + CUBE_SIZE, fy + CUBE_SIZE, fz),
                           glm::vec3(fx + CUBE_SIZE, fy + CUBE_SIZE, fz + CUBE_SIZE),
                           1.0f, 0.0f, 0.0f);  // normal

                    // 上面
                    addFace(mesh,
                           glm::vec3(fx, fy + CUBE_SIZE, fz + CUBE_SIZE),
                           glm::vec3(fx + CUBE_SIZE, fy + CUBE_SIZE, fz + CUBE_SIZE),
                           glm::vec3(fx + CUBE_SIZE, fy + CUBE_SIZE, fz),
                           glm::vec3(fx, fy + CUBE_SIZE, fz),
                           0.0f, 1.0f, 0.0f);  // normal

                    // 下面
                    addFace(mesh,
                           glm::vec3(fx, fy, fz),
                           glm::vec3(fx + CUBE_SIZE, fy, fz),
                           glm::vec3(fx + CUBE_SIZE, fy, fz + CUBE_SIZE),
                           glm::vec3(fx, fy, fz + CUBE_SIZE),
                           0.0f, -1.0f, 0.0f);  // normal
                }

                void addFace(MeshData& mesh,
                            const glm::vec3& v0, const glm::vec3& v1,
                            const glm::vec3& v2, const glm::vec3& v3,
                            float nx, float ny, float nz) {
                    uint32_t baseIndex = mesh.vertices.size() / 8;

                    // 添加 4 个顶点
                    // v0
                    mesh.vertices.push_back(v0.x);
                    mesh.vertices.push_back(v0.y);
                    mesh.vertices.push_back(v0.z);
                    mesh.vertices.push_back(nx);
                    mesh.vertices.push_back(ny);
                    mesh.vertices.push_back(nz);
                    mesh.vertices.push_back(0.0f);  // u
                    mesh.vertices.push_back(0.0f);  // v

                    // v1
                    mesh.vertices.push_back(v1.x);
                    mesh.vertices.push_back(v1.y);
                    mesh.vertices.push_back(v1.z);
                    mesh.vertices.push_back(nx);
                    mesh.vertices.push_back(ny);
                    mesh.vertices.push_back(nz);
                    mesh.vertices.push_back(1.0f);  // u
                    mesh.vertices.push_back(0.0f);  // v

                    // v2
                    mesh.vertices.push_back(v2.x);
                    mesh.vertices.push_back(v2.y);
                    mesh.vertices.push_back(v2.z);
                    mesh.vertices.push_back(nx);
                    mesh.vertices.push_back(ny);
                    mesh.vertices.push_back(nz);
                    mesh.vertices.push_back(1.0f);  // u
                    mesh.vertices.push_back(1.0f);  // v

                    // v3
                    mesh.vertices.push_back(v3.x);
                    mesh.vertices.push_back(v3.y);
                    mesh.vertices.push_back(v3.z);
                    mesh.vertices.push_back(nx);
                    mesh.vertices.push_back(ny);
                    mesh.vertices.push_back(nz);
                    mesh.vertices.push_back(0.0f);  // u
                    mesh.vertices.push_back(1.0f);  // v

                    // 添加索引（两个三角形）
                    mesh.indices.push_back(baseIndex + 0);
                    mesh.indices.push_back(baseIndex + 1);
                    mesh.indices.push_back(baseIndex + 2);

                    mesh.indices.push_back(baseIndex + 0);
                    mesh.indices.push_back(baseIndex + 2);
                    mesh.indices.push_back(baseIndex + 3);
                }

                std::unique_ptr<ThreadPool> m_threadPool;
                std::string m_name;
                MeshCallback m_completionCallback;
                MeshOptimizationOptions m_optimizationOptions;

                struct HashChunkPos {
                    size_t operator()(const ChunkGenerationSystem::ChunkPos& pos) const {
                        return static_cast<size_t>(pos.x) * 31 + static_cast<size_t>(pos.z);
                    }
                };

                std::unordered_set<ChunkGenerationSystem::ChunkPos, HashChunkPos> m_pendingMeshes;
                mutable std::mutex m_pendingMutex;

                std::atomic<size_t> m_completedCount{0};
                std::atomic<size_t> m_totalQueued{0};
                mutable std::mutex m_statsMutex;
            };
        }

        std::unique_ptr<ChunkMeshBuildSystem> ChunkMeshBuildSystem::create(size_t numThreads) {
            return std::unique_ptr<ChunkMeshBuildSystem>(new ChunkMeshBuildSystemImpl(numThreads));
        }

        // ============================================================================
        // AsyncResourceLoader 实现（简化版本）
        // ============================================================================

        namespace {
            class AsyncResourceLoaderImpl : public AsyncResourceLoader {
            public:
                explicit AsyncResourceLoaderImpl(ThreadPool* threadPool) {
                    if (threadPool) {
                        m_threadPool.reset(threadPool);
                    } else {
                        m_threadPool = ThreadPool::create(4);  // 默认 4 线程
                    }

                    LOG_INFO("AsyncResourceLoader", "创建异步资源加载器，线程数: {}", m_threadPool->getThreadCount());
                }

                ~AsyncResourceLoaderImpl() override {
                    m_threadPool->shutdown(true);
                }

                uint64_t loadTexture(const std::string& filePath,
                                     std::function<void(std::shared_ptr<class ITexture>)> callback) override {
                    // 简化实现：立即返回
                    // 实际应该异步加载纹理
                    return 0;
                }

                uint64_t loadModel(const std::string& filePath,
                                  std::function<void(std::shared_ptr<class IMesh>)> callback) override {
                    // 简化实现
                    return 0;
                }

                std::vector<uint64_t> loadTextures(
                    const std::vector<std::string>& filePaths,
                    std::function<void(size_t index, std::shared_ptr<class ITexture>)> callback) override {
                    // 简化实现
                    return {};
                }

                LoadingProgress getProgress() const override {
                    LoadingProgress progress;
                    return progress;
                }

                void cancelAll() override {
                    m_threadPool->shutdownNow();
                }

                void update() override {
                    m_threadPool->update();
                }

                bool isLoading() const override {
                    return m_threadPool->getPendingTaskCount() > 0;
                }

            private:
                std::unique_ptr<ThreadPool> m_threadPool;
            };
        }

        std::unique_ptr<AsyncResourceLoader> AsyncResourceLoader::create(ThreadPool* threadPool) {
            return std::unique_ptr<AsyncResourceLoader>(new AsyncResourceLoaderImpl(threadPool));
        }

    } // namespace Core
} // namespace PrismaEngine
