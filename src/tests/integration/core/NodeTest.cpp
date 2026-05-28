#include <gtest/gtest.h>
#include "core/EntityManager.h"

namespace Prisma {
namespace {

class NodeTest : public ::testing::Test {
protected:
    EntityManager* m_em = nullptr;

    void SetUp() override {
        m_em = new EntityManager();
    }

    void TearDown() override {
        delete m_em;
        m_em = nullptr;
    }
};

TEST_F(NodeTest, CreateNodeFromEntityManager) {
    Node n = m_em->CreateNode();
    EXPECT_TRUE(n.IsValid());
    EXPECT_TRUE(static_cast<bool>(n));
    EXPECT_EQ(n.GetIndex(), n.handle & 0xFFFF);
    EXPECT_EQ(n.GetGeneration(), n.handle >> 16);
}

TEST_F(NodeTest, DefaultNodeIsInvalid) {
    Node n;
    EXPECT_FALSE(n.IsValid());
    EXPECT_FALSE(static_cast<bool>(n));
    EXPECT_EQ(n.handle, 0u);
}

TEST_F(NodeTest, DestroyViaNodeMethod) {
    Node n = m_em->CreateNode();
    ASSERT_TRUE(n.IsValid());
    n.Destroy();
    EXPECT_FALSE(n.IsValid());
}

TEST_F(NodeTest, NodeComparisonOperators) {
    Node a = m_em->CreateNode();
    Node b = m_em->CreateNode();
    EXPECT_NE(a, b);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a == b);

    Node aCopy(a.handle);
    EXPECT_EQ(a, aCopy);
    EXPECT_TRUE(a == aCopy);
    EXPECT_FALSE(a != aCopy);
}

TEST_F(NodeTest, SetAndGetPosition) {
    Node n = m_em->CreateNode();
    n.SetPosition({10.0f, 20.0f});
    m_em->SwapBuffers();

    Vector2 pos = n.GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 10.0f);
    EXPECT_FLOAT_EQ(pos.y, 20.0f);
}

TEST_F(NodeTest, SetAndGetRotation) {
    Node n = m_em->CreateNode();
    n.SetRotation(45.0f);
    m_em->SwapBuffers();
    EXPECT_FLOAT_EQ(n.GetRotation(), 45.0f);
}

TEST_F(NodeTest, SetAndGetScale) {
    Node n = m_em->CreateNode();
    n.SetScale({2.0f, 3.0f});
    m_em->SwapBuffers();

    Vector2 scale = n.GetScale();
    EXPECT_FLOAT_EQ(scale.x, 2.0f);
    EXPECT_FLOAT_EQ(scale.y, 3.0f);
}

TEST_F(NodeTest, DefaultTransformValues) {
    Node n = m_em->CreateNode();
    m_em->SwapBuffers();

    EXPECT_FLOAT_EQ(n.GetX(), 0.0f);
    EXPECT_FLOAT_EQ(n.GetY(), 0.0f);
    EXPECT_FLOAT_EQ(n.GetRotation(), 0.0f);

    Vector2 scale = n.GetScale();
    EXPECT_FLOAT_EQ(scale.x, 1.0f);
    EXPECT_FLOAT_EQ(scale.y, 1.0f);
}

TEST_F(NodeTest, SetXandYIndividually) {
    Node n = m_em->CreateNode();
    n.SetX(5.0f);
    n.SetY(15.0f);
    m_em->SwapBuffers();

    EXPECT_FLOAT_EQ(n.GetX(), 5.0f);
    EXPECT_FLOAT_EQ(n.GetY(), 15.0f);
}

TEST_F(NodeTest, WriteVisibleOnlyAfterSwap) {
    Node n = m_em->CreateNode();
    n.SetX(88.0f);
    m_em->SwapBuffers();
    EXPECT_FLOAT_EQ(n.GetX(), 88.0f);
}

TEST_F(NodeTest, MultipleNodesWriteThenRead) {
    constexpr int kCount = 5;
    std::vector<Node> nodes;
    for (int i = 0; i < kCount; ++i) {
        nodes.push_back(m_em->CreateNode());
        nodes.back().SetX(static_cast<float>(i * 10));
        nodes.back().SetY(static_cast<float>(i * 20));
    }

    m_em->SwapBuffers();

    for (int i = 0; i < kCount; ++i) {
        EXPECT_FLOAT_EQ(nodes[i].GetX(), static_cast<float>(i * 10));
        EXPECT_FLOAT_EQ(nodes[i].GetY(), static_cast<float>(i * 20));
    }
}

TEST_F(NodeTest, HandleEncodesIndexAndGeneration) {
    Node n = m_em->CreateNode();
    uint32_t index = n.handle & 0xFFFF;
    uint32_t gen = n.handle >> 16;

    EXPECT_EQ(n.GetIndex(), index);
    EXPECT_EQ(n.GetGeneration(), gen);
    EXPECT_GE(gen, 1u);
    EXPECT_LT(index, m_em->GetAliveCount());
}

TEST_F(NodeTest, GenerationIncrementsOnDestroy) {
    Node a = m_em->CreateNode();
    uint32_t gen1 = a.GetGeneration();

    m_em->DestroyNode(a.handle);
    Node b = m_em->CreateNode();

    EXPECT_EQ(a.GetIndex(), b.GetIndex());
    EXPECT_EQ(b.GetGeneration(), gen1 + 1);
}

TEST_F(NodeTest, CanCreateUpToCommitStep) {
    constexpr uint32_t kBatchSize = 16384;
    std::vector<Node> nodes;
    nodes.reserve(kBatchSize);

    for (uint32_t i = 0; i < kBatchSize; ++i) {
        Node n = m_em->CreateNode();
        ASSERT_TRUE(n.IsValid());
        nodes.push_back(n);
    }

    EXPECT_EQ(m_em->GetAliveCount(), kBatchSize);
    EXPECT_EQ(m_em->GetCommittedCount(), kBatchSize);
    EXPECT_LE(m_em->GetCommittedCount(), Prisma::kMaxVirtualEntities);
}

TEST_F(NodeTest, MaxVirtualCapacityConstant) {
    EXPECT_EQ(kMaxVirtualEntities, 1024u * 1024u);
    EXPECT_EQ(kCommitStep, 16384u);
}

} // namespace
} // namespace Prisma
