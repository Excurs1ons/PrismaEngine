#pragma once

#include "Export.h"
#include "ai/BehaviorTree.h"
#include "ai/FiniteStateMachine.h"
#include "ai/Blackboard.h"

#include <memory>

namespace Prisma {
namespace AI {

/**
 * @brief ECS AI 组件
 *
 * 持有行为树、黑板和有限状态机的实例。
 * 可附加到场景实体上，由 AISystem 每帧更新。
 */
struct ENGINE_API AIComponent {
    /** 行为树根节点（可选） */
    std::unique_ptr<BTNode> behaviorTree;

    /** 黑板数据存储（可选） */
    std::unique_ptr<Blackboard> blackboard;

    /** 有限状态机（可选） */
    std::unique_ptr<FSM> stateMachine;

    /** 组件是否启用 */
    bool enabled = true;

    /** AI 更新间隔（秒，0=每帧更新） */
    double updateInterval = 0.0;

    AIComponent()
        : blackboard(std::make_unique<Blackboard>())
        , stateMachine(std::make_unique<FSM>()) {}

    explicit AIComponent(std::unique_ptr<BTNode> bt)
        : behaviorTree(std::move(bt))
        , blackboard(std::make_unique<Blackboard>())
        , stateMachine(std::make_unique<FSM>()) {}

    AIComponent(AIComponent&&) = default;
    AIComponent& operator=(AIComponent&&) = default;

    AIComponent(const AIComponent&) = delete;
    AIComponent& operator=(const AIComponent&) = delete;

    /** 检查组件是否可用 */
    bool IsValid() const { return enabled; }

    /** 设置行为树 */
    void SetBehaviorTree(std::unique_ptr<BTNode> bt) {
        behaviorTree = std::move(bt);
    }

    /** 获取黑板引用 */
    Blackboard& GetBlackboard() {
        return *blackboard;
    }

    const Blackboard& GetBlackboard() const {
        return *blackboard;
    }

    /** 获取状态机引用 */
    FSM& GetStateMachine() {
        return *stateMachine;
    }

    const FSM& GetStateMachine() const {
        return *stateMachine;
    }

    /** 重置 AI 状态 */
    void Reset() {
        if (behaviorTree) {
            behaviorTree->Reset();
        }
        if (stateMachine) {
            stateMachine->Reset();
        }
        if (blackboard) {
            blackboard->Clear();
        }
    }

    /** 创建带默认配置的 AI 组件 */
    static AIComponent CreateDefault() {
        AIComponent component;
        component.blackboard = std::make_unique<Blackboard>();
        component.stateMachine = std::make_unique<FSM>();
        return component;
    }

    /** 创建带行为树的 AI 组件 */
    static AIComponent CreateWithBehaviorTree(std::unique_ptr<BTNode> bt) {
        AIComponent component(std::move(bt));
        return component;
    }

private:
    // 上次更新的时间戳（用于更新间隔控制）
    double m_lastUpdateTime = 0.0;

    friend class AISystem;
};

} // namespace AI
} // namespace Prisma
