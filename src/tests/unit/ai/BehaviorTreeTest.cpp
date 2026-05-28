#include <gtest/gtest.h>
#include "ai/BehaviorTree.h"
#include "ai/Blackboard.h"

namespace Prisma {
namespace AI {
namespace {

// ═══════════════════════════════════════════════════════════════════
// Mock BTNode — 返回指定状态的叶子节点
// ═══════════════════════════════════════════════════════════════════

class MockAction final : public BTNode {
public:
    explicit MockAction(BTNode::Status result, const char* name = "MockAction")
        : m_result(result), m_name(name) {}

    Status Execute(Blackboard& /*blackboard*/, double /*dt*/) override {
        m_executeCount++;
        return m_result;
    }

    void Reset() override {
        m_executeCount = 0;
    }

    const char* GetName() const override { return m_name; }

    int GetExecuteCount() const { return m_executeCount; }

    void SetResult(BTNode::Status result) { m_result = result; }

private:
    BTNode::Status m_result;
    const char* m_name;
    int m_executeCount = 0;
};

// ═══════════════════════════════════════════════════════════════════
// MockCondition — 基于回调的条件节点
// ═══════════════════════════════════════════════════════════════════

class MockCondition final : public BTNode {
public:
    using ConditionFn = std::function<bool(Blackboard&)>;

    explicit MockCondition(ConditionFn fn, const char* name = "MockCondition")
        : m_fn(std::move(fn)), m_name(name) {}

    Status Execute(Blackboard& blackboard, double /*dt*/) override {
        m_executeCount++;
        return m_fn(blackboard) ? Status::Success : Status::Failure;
    }

    void Reset() override { m_executeCount = 0; }
    const char* GetName() const override { return m_name; }
    int GetExecuteCount() const { return m_executeCount; }

private:
    ConditionFn m_fn;
    const char* m_name;
    int m_executeCount = 0;
};

// ═══════════════════════════════════════════════════════════════════
// MockRunningAction — 首次返回 Running，之后返回 Success
// ═══════════════════════════════════════════════════════════════════

class MockRunningAction final : public BTNode {
public:
    explicit MockRunningAction(int runFrames = 1, const char* name = "MockRunning")
        : m_runFrames(runFrames), m_counter(0), m_name(name) {}

    Status Execute(Blackboard& /*blackboard*/, double /*dt*/) override {
        if (m_counter++ < m_runFrames) {
            return Status::Running;
        }
        return Status::Success;
    }

    void Reset() override {
        BTNode::Reset();
        m_counter = 0;
    }

    const char* GetName() const override { return m_name; }

private:
    int m_runFrames;
    int m_counter;
    const char* m_name;
};

// ═══════════════════════════════════════════════════════════════════
// Sequence 测试
// ═══════════════════════════════════════════════════════════════════

TEST(BehaviorTree_Sequence, AllChildrenSucceed_ReturnsSuccess) {
    Blackboard bb;
    BTSequence seq("test_seq");

    seq.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "A"));
    seq.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "B"));
    seq.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "C"));

    BTNode::Status result = seq.Execute(bb, 0.016);
    EXPECT_EQ(result, BTNode::Status::Success);
}

TEST(BehaviorTree_Sequence, AnyChildFails_ReturnsFailureAndStops) {
    Blackboard bb;
    BTSequence seq("test_seq");

    auto childA = std::make_unique<MockAction>(BTNode::Status::Success, "A");
    auto childB = std::make_unique<MockAction>(BTNode::Status::Failure, "B");
    auto childC = std::make_unique<MockAction>(BTNode::Status::Success, "C");

    auto* rawA = childA.get();
    auto* rawB = childB.get();
    auto* rawC = childC.get();

    seq.AddChild(std::move(childA));
    seq.AddChild(std::move(childB));
    seq.AddChild(std::move(childC));

    BTNode::Status result = seq.Execute(bb, 0.016);
    EXPECT_EQ(result, BTNode::Status::Failure);

    // A 执行了，B 执行了（失败），C 不应被执行
    EXPECT_EQ(rawA->GetExecuteCount(), 1);
    EXPECT_EQ(rawB->GetExecuteCount(), 1);
    EXPECT_EQ(rawC->GetExecuteCount(), 0);
}

TEST(BehaviorTree_Sequence, EmptySequence_ReturnsSuccess) {
    Blackboard bb;
    BTSequence seq("empty_seq");

    BTNode::Status result = seq.Execute(bb, 0.016);
    EXPECT_EQ(result, BTNode::Status::Success);
}

TEST(BehaviorTree_Sequence, SingleChildSucceeds_ReturnsSuccess) {
    Blackboard bb;
    BTSequence seq("single");

    seq.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "Only"));

    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Success);
}

TEST(BehaviorTree_Sequence, SingleChildFails_ReturnsFailure) {
    Blackboard bb;
    BTSequence seq("single");

    seq.AddChild(std::make_unique<MockAction>(BTNode::Status::Failure, "Only"));

    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Failure);
}

TEST(BehaviorTree_Sequence, RunningChild_SuspendsSequence) {
    Blackboard bb;
    BTSequence seq("suspend_seq");

    seq.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "A"));
    seq.AddChild(std::make_unique<MockRunningAction>(2, "B"));   // 需要 2 帧
    seq.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "C"));

    // 第 1 帧：A 成功，B 返回 Running
    BTNode::Status s1 = seq.Execute(bb, 0.016);
    EXPECT_EQ(s1, BTNode::Status::Running);

    // 第 2 帧：B 继续，返回 Running
    BTNode::Status s2 = seq.Execute(bb, 0.016);
    EXPECT_EQ(s2, BTNode::Status::Running);

    // 第 3 帧：B 完成 -> Success，C 成功
    BTNode::Status s3 = seq.Execute(bb, 0.016);
    EXPECT_EQ(s3, BTNode::Status::Success);
}

TEST(BehaviorTree_Sequence, Reset_ClearsSequenceState) {
    Blackboard bb;
    BTSequence seq("reset_seq");

    seq.AddChild(std::make_unique<MockRunningAction>(3, "Run1"));
    seq.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "Done"));

    // 部分执行
    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Running);

    seq.Reset();

    // 重置后从头开始
    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Running);
    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Running);
    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Running);
    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Success);
}

// ═══════════════════════════════════════════════════════════════════
// Selector 测试
// ═══════════════════════════════════════════════════════════════════

TEST(BehaviorTree_Selector, FirstSuccess_ReturnsSuccessAndStops) {
    Blackboard bb;
    BTSelector sel("test_sel");

    auto childA = std::make_unique<MockAction>(BTNode::Status::Success, "A");
    auto childB = std::make_unique<MockAction>(BTNode::Status::Success, "B");

    auto* rawA = childA.get();
    auto* rawB = childB.get();

    sel.AddChild(std::move(childA));
    sel.AddChild(std::move(childB));

    BTNode::Status result = sel.Execute(bb, 0.016);
    EXPECT_EQ(result, BTNode::Status::Success);

    // A 执行且成功，B 不应执行
    EXPECT_EQ(rawA->GetExecuteCount(), 1);
    EXPECT_EQ(rawB->GetExecuteCount(), 0);
}

TEST(BehaviorTree_Selector, AllFail_ReturnsFailure) {
    Blackboard bb;
    BTSelector sel("all_fail");

    sel.AddChild(std::make_unique<MockAction>(BTNode::Status::Failure, "A"));
    sel.AddChild(std::make_unique<MockAction>(BTNode::Status::Failure, "B"));
    sel.AddChild(std::make_unique<MockAction>(BTNode::Status::Failure, "C"));

    EXPECT_EQ(sel.Execute(bb, 0.016), BTNode::Status::Failure);
}

TEST(BehaviorTree_Selector, SkipFailures_FindsFirstSuccess) {
    Blackboard bb;
    BTSelector sel("skip_fail");

    auto childA = std::make_unique<MockAction>(BTNode::Status::Failure, "A");
    auto childB = std::make_unique<MockAction>(BTNode::Status::Failure, "B");
    auto childC = std::make_unique<MockAction>(BTNode::Status::Success, "C");
    auto childD = std::make_unique<MockAction>(BTNode::Status::Success, "D");

    auto* rawA = childA.get();
    auto* rawB = childB.get();
    auto* rawC = childC.get();
    auto* rawD = childD.get();

    sel.AddChild(std::move(childA));
    sel.AddChild(std::move(childB));
    sel.AddChild(std::move(childC));
    sel.AddChild(std::move(childD));

    EXPECT_EQ(sel.Execute(bb, 0.016), BTNode::Status::Success);

    // A、B 失败，C 成功，D 不应执行
    EXPECT_EQ(rawA->GetExecuteCount(), 1);
    EXPECT_EQ(rawB->GetExecuteCount(), 1);
    EXPECT_EQ(rawC->GetExecuteCount(), 1);
    EXPECT_EQ(rawD->GetExecuteCount(), 0);
}

TEST(BehaviorTree_Selector, EmptySelector_ReturnsFailure) {
    Blackboard bb;
    BTSelector sel("empty_sel");

    EXPECT_EQ(sel.Execute(bb, 0.016), BTNode::Status::Failure);
}

TEST(BehaviorTree_Selector, RunningChild_SuspendsSelector) {
    Blackboard bb;
    BTSelector sel("suspend_sel");

    sel.AddChild(std::make_unique<MockAction>(BTNode::Status::Failure, "A"));
    sel.AddChild(std::make_unique<MockRunningAction>(2, "B"));
    sel.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "C"));

    // 第 1 帧：A 失败，B 返回 Running
    EXPECT_EQ(sel.Execute(bb, 0.016), BTNode::Status::Running);

    // 第 2 帧：B 继续 Running
    EXPECT_EQ(sel.Execute(bb, 0.016), BTNode::Status::Running);

    // 第 3 帧：B 完成 -> Success
    EXPECT_EQ(sel.Execute(bb, 0.016), BTNode::Status::Success);
}

// ═══════════════════════════════════════════════════════════════════
// BTNode 基类测试
// ═══════════════════════════════════════════════════════════════════

TEST(BehaviorTree_Node, DefaultGetName_ReturnsBTNode) {
    // 匿名内部类测试基类默认行为
    class AnonymousNode final : public BTNode {
    public:
        Status Execute(Blackboard&, double) override { return Status::Success; }
    };

    AnonymousNode node;
    EXPECT_STREQ(node.GetName(), "BTNode");
}

TEST(BehaviorTree_Node, Reset_IsSafeToCall) {
    class SafeNode final : public BTNode {
    public:
        Status Execute(Blackboard&, double) override { return Status::Success; }
        bool resetCalled = false;
        void Reset() override { resetCalled = true; }
    };

    SafeNode node;
    node.Reset();
    EXPECT_TRUE(node.resetCalled);
}

// ═══════════════════════════════════════════════════════════════════
// BTComposite 基类测试
// ═══════════════════════════════════════════════════════════════════

TEST(BehaviorTree_Composite, AddChild_IncreasesCount) {
    class DummyComposite final : public BTComposite {
    public:
        explicit DummyComposite(const std::string& name) : BTComposite(name) {}
        Status Execute(Blackboard&, double) override { return Status::Success; }
    };

    DummyComposite comp("test");
    EXPECT_EQ(comp.GetChildCount(), 0);

    comp.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "A"));
    comp.AddChild(std::make_unique<MockAction>(BTNode::Status::Success, "B"));

    EXPECT_EQ(comp.GetChildCount(), 2);
}

TEST(BehaviorTree_Composite, GetChild_ReturnsCorrectPointer) {
    class DummyComposite final : public BTComposite {
    public:
        explicit DummyComposite(const std::string& name) : BTComposite(name) {}
        Status Execute(Blackboard&, double) override { return Status::Success; }
    };

    DummyComposite comp("test");
    auto child = std::make_unique<MockAction>(BTNode::Status::Success, "Child");
    auto* raw = child.get();
    comp.AddChild(std::move(child));

    EXPECT_EQ(comp.GetChild(0), raw);
    EXPECT_EQ(comp.GetChild(1), nullptr); // 越界返回 nullptr
}

TEST(BehaviorTree_Composite, GetChild_OOB_ReturnsNullptr) {
    class DummyComposite final : public BTComposite {
    public:
        explicit DummyComposite(const std::string& name) : BTComposite(name) {}
        Status Execute(Blackboard&, double) override { return Status::Success; }
    };

    DummyComposite comp("test");
    EXPECT_EQ(comp.GetChild(0), nullptr);
    EXPECT_EQ(comp.GetChild(99), nullptr);
}

TEST(BehaviorTree_Composite, Reset_ResetsChildIndexAndChildren) {
    BTSequence seq("reset_children");
    // 使用 NonResettingAction（重置不清零计数）来验证 Reset 后子节点被重新执行
    class NonResettingAction final : public BTNode {
    public:
        explicit NonResettingAction(const char* name) : m_name(name) {}
        Status Execute(Blackboard&, double) override {
            m_executeCount++;
            return Status::Success;
        }
        void Reset() override { /* 不清除 m_executeCount，只清除序列索引 */ }
        const char* GetName() const override { return m_name; }
        int GetExecuteCount() const { return m_executeCount; }
    private:
        const char* m_name;
        int m_executeCount = 0;
    };

    auto childA = std::make_unique<NonResettingAction>("A");
    auto childB = std::make_unique<NonResettingAction>("B");

    auto* rawA = childA.get();
    auto* rawB = childB.get();

    seq.AddChild(std::move(childA));
    seq.AddChild(std::move(childB));

    Blackboard bb;

    // 首次执行：A -> B -> Success
    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Success);
    EXPECT_EQ(rawA->GetExecuteCount(), 1);
    EXPECT_EQ(rawB->GetExecuteCount(), 1);

    // Reset 后子节点会被再次执行（即使它们的运行状态已被重置）
    seq.Reset();
    EXPECT_EQ(seq.Execute(bb, 0.016), BTNode::Status::Success);
    EXPECT_EQ(rawA->GetExecuteCount(), 2); // 再次执行了 A
    EXPECT_EQ(rawB->GetExecuteCount(), 2); // 再次执行了 B
}

// ═══════════════════════════════════════════════════════════════════
// 条件节点 (Condition) 测试
// ═══════════════════════════════════════════════════════════════════

TEST(BehaviorTree_Condition, TrueCondition_ReturnsSuccess) {
    Blackboard bb;
    MockCondition cond([](Blackboard&) { return true; }, "always_true");

    EXPECT_EQ(cond.Execute(bb, 0.016), BTNode::Status::Success);
}

TEST(BehaviorTree_Condition, FalseCondition_ReturnsFailure) {
    Blackboard bb;
    MockCondition cond([](Blackboard&) { return false; }, "always_false");

    EXPECT_EQ(cond.Execute(bb, 0.016), BTNode::Status::Failure);
}

TEST(BehaviorTree_Condition, Condition_ReadsBlackboardValue) {
    Blackboard bb;
    bb.Set<int>("health", 10);

    // 条件：health > 0 时成功
    MockCondition cond([&](Blackboard& b) {
        return b.Get<int>("health") > 0;
    }, "health_check");

    EXPECT_EQ(cond.Execute(bb, 0.016), BTNode::Status::Success);
}

TEST(BehaviorTree_Condition, Reset_ClearsExecuteCount) {
    Blackboard bb;
    MockCondition cond([](Blackboard&) { return true; }, "reset_cond");

    cond.Execute(bb, 0.016);
    EXPECT_EQ(cond.GetExecuteCount(), 1);

    cond.Reset();
    EXPECT_EQ(cond.GetExecuteCount(), 0);
}

// ═══════════════════════════════════════════════════════════════════
// 树组合测试 (Sequence / Selector / Condition 混合)
// ═══════════════════════════════════════════════════════════════════

TEST(BehaviorTree_Composition, SequenceWithSelectorAndCondition) {
    // 构造：Sequence(Selector(护卫, 逃跑), 受伤检查)
    // 逻辑：如果生命值 > 20 则护卫，否则逃跑，然后检查受伤状态
    Blackboard bb;
    bb.Set<int>("health", 50);

    // 护卫动作
    auto guard = std::make_unique<MockAction>(BTNode::Status::Success, "Guard");
    auto* rawGuard = guard.get();

    // 逃跑动作
    auto flee = std::make_unique<MockAction>(BTNode::Status::Success, "Flee");
    auto* rawFlee = flee.get();

    // 条件：健康检查
    auto condition = std::make_unique<MockCondition>(
        [](Blackboard& b) { return b.Get<int>("health") > 20; }, "Healthy");
    auto* rawCond = condition.get();

    // 选择器：优先尝试护卫，失败则逃跑
    auto selector = std::make_unique<BTSelector>("GuardOrFlee");
    selector->AddChild(std::move(guard));
    selector->AddChild(std::move(flee));

    // 序列：先选择行动，再检查伤势
    auto sequence = std::make_unique<BTSequence>("MainSequence");
    sequence->AddChild(std::move(selector));
    sequence->AddChild(std::move(condition));

    // 执行
    BTNode::Status result = sequence->Execute(bb, 0.016);
    EXPECT_EQ(result, BTNode::Status::Success);

    // 健康 > 20，应执行护卫（不执行逃跑）
    EXPECT_EQ(rawGuard->GetExecuteCount(), 1);
    EXPECT_EQ(rawFlee->GetExecuteCount(), 0);
    // 条件应被执行
    EXPECT_EQ(rawCond->GetExecuteCount(), 1);
}

TEST(BehaviorTree_Composition, ComplexTree_FallbackPath) {
    // 构造：Selector(
    //   Sequence(Attack, IsEnemyAlive),
    //   Sequence(Flee, IsSafe)
    // )
    // 先尝试攻击序列，如果敌人死亡则攻击失败，转到逃跑序列
    Blackboard bb;
    bb.Set<bool>("enemyAlive", false);

    auto attack = std::make_unique<MockAction>(BTNode::Status::Success, "Attack");
    auto enemyCheck = std::make_unique<MockCondition>(
        [](Blackboard& b) { return b.Get<bool>("enemyAlive"); }, "IsEnemyAlive");
    auto flee = std::make_unique<MockAction>(BTNode::Status::Success, "Flee");
    auto safeCheck = std::make_unique<MockCondition>(
        [](Blackboard& /*b*/) { return true; }, "IsSafe");

    auto* rawEnemyCheck = enemyCheck.get();
    auto* rawFlee = flee.get();

    auto attackSeq = std::make_unique<BTSequence>("AttackSequence");
    attackSeq->AddChild(std::move(attack));
    attackSeq->AddChild(std::move(enemyCheck));

    auto fleeSeq = std::make_unique<BTSequence>("FleeSequence");
    fleeSeq->AddChild(std::move(flee));
    fleeSeq->AddChild(std::move(safeCheck));

    auto rootSelector = std::make_unique<BTSelector>("RootSelector");
    rootSelector->AddChild(std::move(attackSeq));
    rootSelector->AddChild(std::move(fleeSeq));

    // 执行：攻击序列中 enemyCheck 失败，转至逃跑序列
    BTNode::Status result = rootSelector->Execute(bb, 0.016);
    EXPECT_EQ(result, BTNode::Status::Success);

    // 敌人检查失败，应执行逃跑
    EXPECT_EQ(rawEnemyCheck->GetExecuteCount(), 1);
    EXPECT_EQ(rawEnemyCheck->Execute(bb, 0.016), BTNode::Status::Failure);
    EXPECT_EQ(rawFlee->GetExecuteCount(), 1);
}

// ═══════════════════════════════════════════════════════════════════
// BTNode::Status 枚举与状态工具函数测试
// ═══════════════════════════════════════════════════════════════════

TEST(BehaviorTree_Status, BTStatusToString_AllValues) {
    EXPECT_STREQ(BTStatusToString(BTNode::Status::Success), "Success");
    EXPECT_STREQ(BTStatusToString(BTNode::Status::Failure), "Failure");
    EXPECT_STREQ(BTStatusToString(BTNode::Status::Running), "Running");
}

} // namespace
} // namespace AI
} // namespace Prisma
