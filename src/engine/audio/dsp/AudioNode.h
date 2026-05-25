#pragma once

#include "AudioBuffer.h"
#include "ParameterBlock.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <queue>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace Prisma::Audio::DSP {

// ============================================================================
// PinDirection - 引脚方向
// ============================================================================
enum class PinDirection : uint8_t {
    Input  = 0,
    Output = 1
};

// ============================================================================
// 前置声明
// ============================================================================
class AudioNode;
class AudioGraph;

// ============================================================================
// AudioPin - 音频引脚，用于节点间连接
// ============================================================================
class AudioPin {
    friend class AudioGraph;

public:
    AudioPin(const std::string& name, PinDirection dir, AudioNode* owner)
        : m_name(name)
        , m_direction(dir)
        , m_owner(owner)
    {
    }

    const std::string& GetName()      const { return m_name; }
    PinDirection       GetDirection() const { return m_direction; }
    AudioNode*         GetOwner()     const { return m_owner; }
    AudioPin*          GetConnection() const { return m_connection; }

    void Connect(AudioPin* other) {
        if (!other) return;
        if (m_direction == other->m_direction) return; // 同向不允许连接

        // 确保方向正确：Output -> Input
        PinDirection outDir = PinDirection::Output;
        PinDirection inDir  = PinDirection::Input;
        AudioPin* output = (m_direction == outDir) ? this : other;
        AudioPin* input  = (m_direction == inDir)  ? this : other;

        // 先断开已有连接
        if (output->m_connection) {
            output->m_connection->m_connection = nullptr;
        }
        if (input->m_connection) {
            input->m_connection->m_connection = nullptr;
        }

        output->m_connection = input;
        input->m_connection  = output;
    }

    void Disconnect() {
        if (m_connection) {
            m_connection->m_connection = nullptr;
            m_connection = nullptr;
        }
    }

    bool IsConnected() const { return m_connection != nullptr; }

    AudioBuffer* GetBuffer() const { return m_buffer; }
    void         SetBuffer(AudioBuffer* buffer) { m_buffer = buffer; }

private:
    std::string  m_name;
    PinDirection m_direction;
    AudioNode*   m_owner      = nullptr;
    AudioPin*    m_connection = nullptr;
    AudioBuffer* m_buffer     = nullptr;
};

// ============================================================================
// AudioNode - 抽象音频处理节点基类
// ============================================================================
class AudioNode : public std::enable_shared_from_this<AudioNode> {
    friend class AudioGraph;

public:
    virtual ~AudioNode() = default;

    // --- 纯虚接口 ---

    /// @brief 处理音频数据
    /// @param output  输出缓冲（节点写入处理结果）
    /// @param ctx     处理上下文（采样率、块大小等）
    virtual void Process(AudioBuffer& output, const AudioProcessContext& ctx) = 0;

    /// @brief 重置节点状态（如包络复位、滤波器清空）
    virtual void Reset() {}

    // --- 引脚管理 ---

    AudioPin* AddInputPin(const std::string& name) {
        auto pin = std::make_unique<AudioPin>(name, PinDirection::Input, this);
        AudioPin* ptr = pin.get();
        m_inputPins.push_back(std::move(pin));
        return ptr;
    }

    AudioPin* AddOutputPin(const std::string& name) {
        auto pin = std::make_unique<AudioPin>(name, PinDirection::Output, this);
        AudioPin* ptr = pin.get();
        m_outputPins.push_back(std::move(pin));
        return ptr;
    }

    AudioPin* GetInputPin(const std::string& name) const {
        for (const auto& pin : m_inputPins) {
            if (pin->GetName() == name) return pin.get();
        }
        return nullptr;
    }

    AudioPin* GetOutputPin(const std::string& name) const {
        for (const auto& pin : m_outputPins) {
            if (pin->GetName() == name) return pin.get();
        }
        return nullptr;
    }

    // --- 参数管理 ---

    void SetParameter(const std::string& name, float value) {
        m_params[name].SetValue(value);
    }

    float GetParameter(const std::string& name) const {
        auto it = m_params.find(name);
        if (it != m_params.end()) {
            return it->second.GetValue();
        }
        return 0.0f;
    }

    // --- 基础属性 ---

    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    uint64_t GetId() const { return m_id; }

protected:
    /// @brief 读取指定输入引脚的缓冲（从上游节点）
    AudioBuffer* ReadInput(const std::string& pinName) {
        auto* pin = GetInputPin(pinName);
        if (pin && pin->IsConnected()) {
            auto* connected = pin->GetConnection();
            return connected->GetBuffer();
        }
        return nullptr;
    }

    std::vector<std::unique_ptr<AudioPin>> m_inputPins;
    std::vector<std::unique_ptr<AudioPin>> m_outputPins;
    std::unordered_map<std::string, ParameterBlock> m_params;
    std::string m_name;
    uint64_t m_id = 0;
};

// ============================================================================
// AudioGraph - 音频处理图，管理节点连接与拓扑排序执行
// ============================================================================
class AudioGraph {
public:
    AudioGraph(uint32_t sampleRate = 48000, uint32_t framesPerBlock = 256) {
        m_context.sampleRate     = sampleRate;
        m_context.framesPerBlock = framesPerBlock;
    }

    ~AudioGraph() = default;

    // --- 节点管理 ---

    std::shared_ptr<AudioNode> CreateNode(std::unique_ptr<AudioNode> node) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto shared = std::shared_ptr<AudioNode>(node.release());
        shared->m_id = m_nextNodeId++;
        m_nodes.push_back(shared);
        m_dirty = true;
        return shared;
    }

    void RemoveNode(std::shared_ptr<AudioNode> node) {
        if (!node) return;
        std::lock_guard<std::mutex> lock(m_mutex);

        // 断开与该节点相关的所有连接
        for (auto& pin : node->m_inputPins) {
            pin->Disconnect();
        }
        for (auto& pin : node->m_outputPins) {
            pin->Disconnect();
        }

        auto it = std::remove(m_nodes.begin(), m_nodes.end(), node);
        if (it != m_nodes.end()) {
            m_nodes.erase(it, m_nodes.end());
            m_dirty = true;
        }
    }

    // --- 连接管理 ---

    bool Connect(std::shared_ptr<AudioNode> source, const std::string& outputPin,
                 std::shared_ptr<AudioNode> target, const std::string& inputPin) {
        if (!source || !target) return false;
        if (source == target) return false;

        std::lock_guard<std::mutex> lock(m_mutex);

        AudioPin* srcPin = source->GetOutputPin(outputPin);
        AudioPin* dstPin = target->GetInputPin(inputPin);

        if (!srcPin || !dstPin) return false;

        // 环路检测：从 target 到 source 是否已有路径
        if (WouldCreateCycle(target.get(), source.get())) {
            return false;
        }

        srcPin->Connect(dstPin);
        m_dirty = true;
        return true;
    }

    bool Disconnect(std::shared_ptr<AudioNode> source, std::shared_ptr<AudioNode> target) {
        if (!source || !target) return false;

        std::lock_guard<std::mutex> lock(m_mutex);

        bool disconnected = false;
        for (auto& srcPin : source->m_outputPins) {
            if (srcPin->IsConnected()) {
                auto* conn = srcPin->GetConnection();
                if (conn->GetOwner() == target.get()) {
                    srcPin->Disconnect();
                    disconnected = true;
                }
            }
        }

        if (disconnected) m_dirty = true;
        return disconnected;
    }

    // --- 图处理 ---

    void Process(AudioBuffer& output) {
        if (m_dirty) {
            TopologicalSort();
        }

        output.Resize(2, m_context.framesPerBlock);
        output.Clear();

        AudioProcessContext ctx = m_context;
        ctx.currentTime = m_context.currentTime;
        ctx.sampleIndex = m_context.sampleIndex;

        // 为所有输出引脚分配/重置缓冲
        for (AudioNode* node : m_sortedNodes) {
            for (auto& pin : node->m_outputPins) {
                if (!pin->GetBuffer()) {
                    pin->SetBuffer(new AudioBuffer(2, m_context.framesPerBlock));
                }
                pin->GetBuffer()->Resize(2, m_context.framesPerBlock);
                pin->GetBuffer()->Clear();
            }
        }

        // 按拓扑顺序处理节点
        for (AudioNode* node : m_sortedNodes) {
            ProcessNode(node, ctx);
        }

        // 将所有叶子节点（无下游连接的输出引脚）混合到最终输出
        for (AudioNode* node : m_sortedNodes) {
            bool isLeaf = true;
            for (auto& pin : node->m_outputPins) {
                if (pin->IsConnected()) {
                    isLeaf = false;
                    break;
                }
            }
            if (isLeaf && !node->m_outputPins.empty()) {
                auto* buf = node->m_outputPins[0]->GetBuffer();
                if (buf) {
                    output.Mix(*buf);
                }
            }
        }

        // 更新上下文
        m_context.sampleIndex += m_context.framesPerBlock;
        m_context.currentTime  = static_cast<double>(m_context.sampleIndex)
                               / static_cast<double>(m_context.sampleRate);
    }

    void Reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& node : m_nodes) {
            node->Reset();
        }
        m_context.sampleIndex = 0;
        m_context.currentTime = 0.0;
    }

    // --- 拓扑排序 ---

    /// @brief 使用 Kahn 算法进行拓扑排序
    void TopologicalSort() {
        m_sortedNodes.clear();

        // 构建入度表
        std::unordered_map<AudioNode*, int> inDegree;
        std::unordered_map<AudioNode*, std::vector<AudioNode*>> adjacency;

        for (const auto& node : m_nodes) {
            inDegree[node.get()] = 0;
            adjacency[node.get()] = {};
        }

        for (const auto& node : m_nodes) {
            for (auto& outPin : node->m_outputPins) {
                if (outPin->IsConnected()) {
                    auto* target = outPin->GetConnection()->GetOwner();
                    adjacency[node.get()].push_back(target);
                    inDegree[target]++;
                }
            }
        }

        // BFS — 入度为零的节点入队
        std::queue<AudioNode*> q;
        for (auto& [node, deg] : inDegree) {
            if (deg == 0) {
                q.push(node);
            }
        }

        while (!q.empty()) {
            AudioNode* node = q.front();
            q.pop();
            m_sortedNodes.push_back(node);

            for (AudioNode* neighbor : adjacency[node]) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    q.push(neighbor);
                }
            }
        }

        // 如果有节点未排序，说明存在环（抛出异常而非静默失败）
        if (m_sortedNodes.size() != m_nodes.size()) {
            m_sortedNodes.clear();
            throw std::runtime_error("AudioGraph: cycle detected during topological sort");
        }

        m_dirty = false;
    }

    // --- 批量更新事务 ---

    void BeginBatchUpdate() {
        m_mutex.lock();
    }

    void EndBatchUpdate() {
        m_dirty = true;
        m_mutex.unlock();
    }

    // --- 上下文访问 ---

    AudioProcessContext GetContext() const { return m_context; }

private:
    // ========================================================================
    // 内部边结构
    // ========================================================================
    struct Edge {
        AudioNode* source;
        AudioNode* target;
    };

    // ========================================================================
    // 构建边列表（基于输出引脚→输入引脚的连接）
    // ========================================================================
    std::vector<Edge> BuildEdgeList() const {
        std::vector<Edge> edges;
        for (const auto& node : m_nodes) {
            for (auto& outPin : node->m_outputPins) {
                if (outPin->IsConnected()) {
                    auto* targetNode = outPin->GetConnection()->GetOwner();
                    edges.push_back({node.get(), targetNode});
                }
            }
        }
        return edges;
    }

    // ========================================================================
    // 环路检测：从 source 出发 DFS，看能否到达 target
    // ========================================================================
    bool WouldCreateCycle(AudioNode* source, AudioNode* target) const {
        if (source == target) return true;

        std::unordered_set<AudioNode*> visited;
        std::queue<AudioNode*> q;
        q.push(source);
        visited.insert(source);

        while (!q.empty()) {
            AudioNode* current = q.front();
            q.pop();

            for (auto& outPin : current->m_outputPins) {
                if (outPin->IsConnected()) {
                    AudioNode* next = outPin->GetConnection()->GetOwner();
                    if (next == target) return true;
                    if (visited.find(next) == visited.end()) {
                        visited.insert(next);
                        q.push(next);
                    }
                }
            }
        }

        return false;
    }

    // ========================================================================
    // 处理单个节点
    // ========================================================================
    void ProcessNode(AudioNode* node, AudioProcessContext& ctx) {
        // 取第一个输出引脚缓冲作为处理输出
        AudioBuffer* outputBuf = nullptr;
        if (!node->m_outputPins.empty()) {
            outputBuf = node->m_outputPins[0]->GetBuffer();
        }

        if (outputBuf) {
            node->Process(*outputBuf, ctx);
        } else {
            // 无输出引脚：用临时缓冲调用 Process（允许节点作"副作用"处理）
            AudioBuffer temp(2, m_context.framesPerBlock);
            node->Process(temp, ctx);
        }
    }

    std::vector<std::shared_ptr<AudioNode>> m_nodes;
    std::vector<AudioNode*>                 m_sortedNodes;
    AudioProcessContext                     m_context;
    std::mutex                              m_mutex;
    bool                                    m_dirty = true;
    uint64_t                                m_nextNodeId = 1;
};

} // namespace Prisma::Audio::DSP
