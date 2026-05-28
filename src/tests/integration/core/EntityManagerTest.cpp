#include <gtest/gtest.h>
#include "core/EntityManager.h"

namespace Prisma {
namespace {

class EntityManagerTest : public ::testing::Test {
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

TEST_F(EntityManagerTest, InitiallyEmpty) {
    EXPECT_EQ(m_em->GetAliveCount(), 0u);
    EXPECT_EQ(m_em->GetCommittedCount(), 0u);
}

TEST_F(EntityManagerTest, CreateSingleNode) {
    Node n = m_em->CreateNode();
    EXPECT_TRUE(n.IsValid());
    EXPECT_NE(n.handle, 0u);
    EXPECT_EQ(m_em->GetAliveCount(), 1u);
}

TEST_F(EntityManagerTest, CreateAndDestroy) {
    Node n = m_em->CreateNode();
    ASSERT_TRUE(n.IsValid());
    EXPECT_EQ(m_em->GetAliveCount(), 1u);

    m_em->DestroyNode(n.handle);
    EXPECT_FALSE(n.IsValid());
}

TEST_F(EntityManagerTest, CreateMultipleNodes) {
    constexpr uint32_t kCount = 100;
    std::vector<Node> nodes;
    nodes.reserve(kCount);

    for (uint32_t i = 0; i < kCount; ++i) {
        nodes.push_back(m_em->CreateNode());
        EXPECT_TRUE(nodes.back().IsValid());
    }

    EXPECT_EQ(m_em->GetAliveCount(), kCount);
    EXPECT_GE(m_em->GetCommittedCount(), kCount);
}

TEST_F(EntityManagerTest, SlotReuseAfterDestroy) {
    Node a = m_em->CreateNode();
    Node b = m_em->CreateNode();
    ASSERT_TRUE(a.IsValid());
    ASSERT_TRUE(b.IsValid());

    uint32_t oldAlive = m_em->GetAliveCount();
    m_em->DestroyNode(a.handle);
    EXPECT_FALSE(a.IsValid());

    Node c = m_em->CreateNode();
    EXPECT_TRUE(c.IsValid());
    EXPECT_EQ(m_em->GetAliveCount(), oldAlive);
}

TEST_F(EntityManagerTest, DoubleBufferWriteThenRead) {
    Node n = m_em->CreateNode();

    n.SetX(42.0f);
    n.SetY(100.0f);

    m_em->SwapBuffers();

    EXPECT_FLOAT_EQ(n.GetX(), 42.0f);
    EXPECT_FLOAT_EQ(n.GetY(), 100.0f);
}

TEST_F(EntityManagerTest, DoubleBufferSwapToggles) {
    Node n = m_em->CreateNode();

    m_em->SwapBuffers();
    n.SetX(1.0f);
    m_em->SwapBuffers();
    EXPECT_FLOAT_EQ(n.GetX(), 1.0f);

    n.SetX(2.0f);
    m_em->SwapBuffers();
    EXPECT_FLOAT_EQ(n.GetX(), 2.0f);
}

TEST_F(EntityManagerTest, DoubleBufferReadWritePointerDistinct) {
    auto* readA = m_em->GetTransformRead();
    auto* writeA = m_em->GetTransformWrite();
    EXPECT_NE(readA, writeA);

    m_em->SwapBuffers();
    auto* readB = m_em->GetTransformRead();
    auto* writeB = m_em->GetTransformWrite();
    EXPECT_NE(readB, writeB);

    EXPECT_EQ(readA, writeB);
    EXPECT_EQ(writeA, readB);
}

TEST_F(EntityManagerTest, SoAComponentIsolation) {
    Node a = m_em->CreateNode();
    Node b = m_em->CreateNode();
    Node c = m_em->CreateNode();

    a.SetX(10.0f);
    b.SetX(20.0f);
    c.SetX(30.0f);
    m_em->SwapBuffers();

    EXPECT_FLOAT_EQ(a.GetX(), 10.0f);
    EXPECT_FLOAT_EQ(b.GetX(), 20.0f);
    EXPECT_FLOAT_EQ(c.GetX(), 30.0f);

    a.SetX(99.0f);
    b.SetX(20.0f);
    c.SetX(30.0f);
    m_em->SwapBuffers();
    EXPECT_FLOAT_EQ(a.GetX(), 99.0f);
    EXPECT_FLOAT_EQ(b.GetX(), 20.0f);
    EXPECT_FLOAT_EQ(c.GetX(), 30.0f);
}

TEST_F(EntityManagerTest, SoAFieldIndependence) {
    Node n = m_em->CreateNode();
    n.SetX(1.0f);
    n.SetY(2.0f);
    n.SetRotation(3.0f);
    n.SetScale({4.0f, 5.0f});
    m_em->SwapBuffers();

    EXPECT_FLOAT_EQ(n.GetX(), 1.0f);
    EXPECT_FLOAT_EQ(n.GetY(), 2.0f);
    EXPECT_FLOAT_EQ(n.GetRotation(), 3.0f);

    auto scale = n.GetScale();
    EXPECT_FLOAT_EQ(scale.x, 4.0f);
    EXPECT_FLOAT_EQ(scale.y, 5.0f);
}

TEST_F(EntityManagerTest, DestroyInvalidHandleDoesNotCrash) {
    m_em->DestroyNode(0);
    m_em->DestroyNode(0xDEADBEEF);
    SUCCEED();
}

TEST_F(EntityManagerTest, CreateAfterDestroyReusesSlot) {
    Node a = m_em->CreateNode();
    uint32_t aHandle = a.handle;
    m_em->DestroyNode(aHandle);

    Node b = m_em->CreateNode();
    uint32_t bIndex = b.handle & 0xFFFF;
    uint32_t aIndex = aHandle & 0xFFFF;

    EXPECT_EQ(bIndex, aIndex);
    EXPECT_NE(b.handle, aHandle);
}

} // namespace
} // namespace Prisma
