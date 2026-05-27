#pragma once

#include "Export.h"
#include "Blackboard.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <cstdint>

namespace Prisma {
namespace AI {

// ═══════════════════════════════════════════════════════════════════
// 前置声明
// ═══════════════════════════════════════════════════════════════════

class Blackboard;

// ═══════════════════════════════════════════════════════════════════
// BTNode — 行为树节点基类
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTNode {
public:
    enum class Status : uint8_t {
        Success,
        Failure,
        Running
    };

    virtual ~BTNode() = default;

    /**
     * @brief 执行节点逻辑
     * @param blackboard 共享黑板数据
     * @param dt 帧时间差（秒）
     * @return 执行状态：Success / Failure / Running
     */
    virtual Status Execute(Blackboard& blackboard, double dt) = 0;

    /** 重置节点状态（用于复用） */
    virtual void Reset() {}

    /** 获取节点名称 */
    virtual const char* GetName() const { return "BTNode"; }
};

// ═══════════════════════════════════════════════════════════════════
// BTTask — 叶子动作节点基类
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTTask : public BTNode {
public:
    ~BTTask() override = default;
    const char* GetName() const override { return "Task"; }
};

// ═══════════════════════════════════════════════════════════════════
// BTComposite — 组合节点基类
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTComposite : public BTNode {
public:
    explicit BTComposite(std::string name = "Composite")
        : m_name(std::move(name)) {}

    BTComposite(const BTComposite&) = delete;
    BTComposite& operator=(const BTComposite&) = delete;

    ~BTComposite() override = default;

    /** 添加子节点 */
    void AddChild(std::unique_ptr<BTNode> child) {
        m_children.push_back(std::move(child));
    }

    /** 获取子节点数量 */
    size_t GetChildCount() const { return m_children.size(); }

    /** 获取指定子节点 */
    BTNode* GetChild(size_t index) const {
        if (index < m_children.size())
            return m_children[index].get();
        return nullptr;
    }

    /** 重置所有子节点 */
    void Reset() override {
        m_currentChildIndex = 0;
        for (auto& child : m_children) {
            child->Reset();
        }
    }

    const char* GetName() const override { return m_name.c_str(); }

protected:
    std::string m_name;
    std::vector<std::unique_ptr<BTNode>> m_children;
    size_t m_currentChildIndex = 0;
};

// ═══════════════════════════════════════════════════════════════════
// BTSequence — 序列节点（顺序执行，遇失败则中止）
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTSequence : public BTComposite {
public:
    explicit BTSequence(std::string name = "Sequence")
        : BTComposite(std::move(name)) {}

    Status Execute(Blackboard& blackboard, double dt) override {
        // 从上次中断处继续执行
        while (m_currentChildIndex < m_children.size()) {
            Status status = m_children[m_currentChildIndex]->Execute(blackboard, dt);

            if (status == Status::Running) {
                return Status::Running; // 挂起，下次从该子节点继续
            }

            if (status == Status::Failure) {
                m_currentChildIndex = 0; // 重置，下次从头开始
                return Status::Failure;
            }

            // Success: 继续下一个子节点
            m_currentChildIndex++;
        }

        // 所有子节点执行成功
        m_currentChildIndex = 0;
        return Status::Success;
    }

    const char* GetName() const override { return m_name.c_str(); }
};

// ═══════════════════════════════════════════════════════════════════
// BTSelector — 选择节点（顺序尝试，遇成功则中止）
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTSelector : public BTComposite {
public:
    explicit BTSelector(std::string name = "Selector")
        : BTComposite(std::move(name)) {}

    Status Execute(Blackboard& blackboard, double dt) override {
        while (m_currentChildIndex < m_children.size()) {
            Status status = m_children[m_currentChildIndex]->Execute(blackboard, dt);

            if (status == Status::Running) {
                return Status::Running;
            }

            if (status == Status::Success) {
                m_currentChildIndex = 0;
                return Status::Success;
            }

            // Failure: 尝试下一个子节点
            m_currentChildIndex++;
        }

        // 所有子节点均失败
        m_currentChildIndex = 0;
        return Status::Failure;
    }

    const char* GetName() const override { return m_name.c_str(); }
};

// ═══════════════════════════════════════════════════════════════════
// BTParallel — 并行节点（同时执行所有子节点）
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTParallel : public BTComposite {
public:
    enum class Policy : uint8_t {
        All,    // 等待所有子节点完成
        One     // 任一子节点完成即返回
    };

    explicit BTParallel(std::string name = "Parallel", Policy policy = Policy::All)
        : BTComposite(std::move(name)), m_policy(policy) {}

    Status Execute(Blackboard& blackboard, double dt) override {
        if (m_children.empty()) return Status::Success;

        // 确保运行状态向量大小正确
        if (m_childStatus.size() != m_children.size()) {
            m_childStatus.assign(m_children.size(), Status::Running);
        }

        size_t completedCount = 0;
        size_t successCount = 0;
        size_t failureCount = 0;

        for (size_t i = 0; i < m_children.size(); ++i) {
            if (m_childStatus[i] == Status::Running) {
                m_childStatus[i] = m_children[i]->Execute(blackboard, dt);
            }

            if (m_childStatus[i] != Status::Running) {
                completedCount++;
                if (m_childStatus[i] == Status::Success) successCount++;
                else failureCount++;
            }
        }

        // Policy::One: 任一完成即返回
        if (m_policy == Policy::One) {
            if (completedCount > 0) {
                ResetChildStatus();
                return successCount > 0 ? Status::Success : Status::Failure;
            }
            return Status::Running;
        }

        // Policy::All: 等待所有完成
        if (completedCount == m_children.size()) {
            ResetChildStatus();
            return failureCount == 0 ? Status::Success : Status::Failure;
        }

        return Status::Running;
    }

    void Reset() override {
        BTComposite::Reset();
        ResetChildStatus();
    }

    const char* GetName() const override { return m_name.c_str(); }

private:
    Policy m_policy = Policy::All;
    std::vector<Status> m_childStatus;

    void ResetChildStatus() {
        m_childStatus.clear();
    }
};

// ═══════════════════════════════════════════════════════════════════
// BTDecorator — 装饰节点基类
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTDecorator : public BTNode {
public:
    explicit BTDecorator(std::string name = "Decorator")
        : m_name(std::move(name)) {}

    ~BTDecorator() override = default;

    /** 设置子节点 */
    void SetChild(std::unique_ptr<BTNode> child) {
        m_child = std::move(child);
    }

    /** 获取子节点 */
    BTNode* GetChild() const { return m_child.get(); }

    /** 检查是否有子节点 */
    bool HasChild() const { return m_child != nullptr; }

    const char* GetName() const override { return m_name.c_str(); }

protected:
    std::string m_name;
    std::unique_ptr<BTNode> m_child;
};

// ═══════════════════════════════════════════════════════════════════
// BTDecorator_Invert — 取反装饰器
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTDecorator_Invert : public BTDecorator {
public:
    BTDecorator_Invert() : BTDecorator("Invert") {}

    Status Execute(Blackboard& blackboard, double dt) override {
        if (!m_child) return Status::Failure;
        Status childStatus = m_child->Execute(blackboard, dt);

        if (childStatus == Status::Running) return Status::Running;
        return (childStatus == Status::Success) ? Status::Failure : Status::Success;
    }
};

// ═══════════════════════════════════════════════════════════════════
// BTDecorator_RepeatUntilFail — 重复直到失败
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTDecorator_RepeatUntilFail : public BTDecorator {
public:
    BTDecorator_RepeatUntilFail() : BTDecorator("RepeatUntilFail") {}

    Status Execute(Blackboard& blackboard, double dt) override {
        if (!m_child) return Status::Failure;

        Status status = m_child->Execute(blackboard, dt);

        if (status == Status::Running) return Status::Running;

        if (status == Status::Failure) {
            m_child->Reset();
            return Status::Success; // 循环终止，返回成功
        }

        // Success: 重置子节点并继续循环
        m_child->Reset();
        return Status::Running; // 继续循环
    }
};

// ═══════════════════════════════════════════════════════════════════
// BTDecorator_Cooldown — 冷却装饰器
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTDecorator_Cooldown : public BTDecorator {
public:
    explicit BTDecorator_Cooldown(double cooldownSeconds)
        : BTDecorator("Cooldown"), m_cooldown(cooldownSeconds) {}

    Status Execute(Blackboard& blackboard, double dt) override {
        if (!m_child) return Status::Failure;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - m_lastRunTime).count();

        if (elapsed < m_cooldown) {
            return Status::Failure; // 冷却中
        }

        m_lastRunTime = now;
        return m_child->Execute(blackboard, dt);
    }

    void Reset() override {
        BTDecorator::Reset();
        m_lastRunTime = std::chrono::steady_clock::time_point();
    }

private:
    double m_cooldown = 0.0;
    std::chrono::steady_clock::time_point m_lastRunTime;
};

// ═══════════════════════════════════════════════════════════════════
// BTDecorator_Timer — 定时执行装饰器
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API BTDecorator_Timer : public BTDecorator {
public:
    explicit BTDecorator_Timer(double intervalSeconds)
        : BTDecorator("Timer"), m_interval(intervalSeconds) {}

    Status Execute(Blackboard& blackboard, double dt) override {
        if (!m_child) return Status::Failure;

        // 累积时间
        m_accumulator += dt;

        if (m_accumulator < m_interval) {
            return Status::Failure; // 未到时间
        }

        m_accumulator = 0.0;
        return m_child->Execute(blackboard, dt);
    }

    void Reset() override {
        BTDecorator::Reset();
        m_accumulator = 0.0;
    }

    void SetInterval(double interval) { m_interval = interval; }
    double GetInterval() const { return m_interval; }

private:
    double m_interval = 1.0;
    double m_accumulator = 0.0;
};

// ═══════════════════════════════════════════════════════════════════
// 辅助函数：状态转字符串
// ═══════════════════════════════════════════════════════════════════

inline const char* BTStatusToString(BTNode::Status status) {
    switch (status) {
        case BTNode::Status::Success: return "Success";
        case BTNode::Status::Failure: return "Failure";
        case BTNode::Status::Running: return "Running";
        default: return "Unknown";
    }
}

} // namespace AI
} // namespace Prisma
