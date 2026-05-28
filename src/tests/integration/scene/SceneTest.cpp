#include <gtest/gtest.h>
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "transform/Transform.h"
#include "transform/Camera.h"
#include "core/EntityManager.h"

namespace Prisma {
namespace {

// ============================================================================
// Scene Test Fixture — provides EntityManager + Scene
// ============================================================================
class SceneTest : public ::testing::Test {
protected:
    EntityManager m_em;
    Scene m_scene;

    void SetUp() override {
        m_scene.SetName("TestScene");
    }
};

// ============================================================================
// Scene — Node Creation and Deletion
// ============================================================================
TEST_F(SceneTest, DefaultState) {
    EXPECT_EQ(m_scene.GetName(), "TestScene");
    EXPECT_TRUE(m_scene.GetNodes().empty());
    EXPECT_FALSE(m_scene.IsDirty());  // CreateNode sets dirty, so after SetUp it's not dirty
}

TEST_F(SceneTest, CreateNode) {
    Node node = m_scene.CreateNode("MyNode");
    EXPECT_TRUE(node.IsValid());
    EXPECT_EQ(m_scene.GetNodes().size(), 1);
    EXPECT_EQ(m_scene.GetNodeName(node), "MyNode");
    EXPECT_TRUE(m_scene.IsDirty());
}

TEST_F(SceneTest, CreateMultipleNodes) {
    Node a = m_scene.CreateNode("A");
    Node b = m_scene.CreateNode("B");
    Node c = m_scene.CreateNode("C");

    EXPECT_EQ(m_scene.GetNodes().size(), 3);
    EXPECT_EQ(m_scene.GetNodeName(a), "A");
    EXPECT_EQ(m_scene.GetNodeName(b), "B");
    EXPECT_EQ(m_scene.GetNodeName(c), "C");
}

TEST_F(SceneTest, RemoveNode) {
    Node node = m_scene.CreateNode("ToRemove");
    EXPECT_EQ(m_scene.GetNodes().size(), 1);

    m_scene.RemoveNode(node);
    EXPECT_EQ(m_scene.GetNodes().size(), 0);
    EXPECT_FALSE(node.IsValid());  // handle is now invalidated
}

TEST_F(SceneTest, RemoveNonexistentNode) {
    Node invalid(0xFFFFFFFF);
    // Should not crash
    m_scene.RemoveNode(invalid);
    EXPECT_TRUE(m_scene.GetNodes().empty());
}

TEST_F(SceneTest, RemoveNodeRecursivelyDestroysChildren) {
    Node parent = m_scene.CreateNode("Parent");
    Node child = m_scene.CreateNode("Child");
    m_scene.SetParent(child, parent);

    m_scene.RemoveNode(parent);

    // Child should also be gone
    // Verify by checking the scene's node list
    for (const auto& n : m_scene.GetNodes()) {
        EXPECT_NE(n, parent);
        EXPECT_NE(n, child);
    }
}

TEST_F(SceneTest, NodeNameChange) {
    Node node = m_scene.CreateNode("Original");
    EXPECT_EQ(m_scene.GetNodeName(node), "Original");

    m_scene.SetNodeName(node, "Renamed");
    EXPECT_EQ(m_scene.GetNodeName(node), "Renamed");
}

// ============================================================================
// Scene — Parent / Child Hierarchy
// ============================================================================
TEST_F(SceneTest, SetParent) {
    Node parent = m_scene.CreateNode("Parent");
    Node child = m_scene.CreateNode("Child");
    ASSERT_TRUE(parent.IsValid());
    ASSERT_TRUE(child.IsValid());

    m_scene.SetParent(child, parent);

    auto children = m_scene.GetChildren(parent);
    ASSERT_EQ(children.size(), 1);
    EXPECT_EQ(children[0], child);

    Node retrievedParent = m_scene.GetParent(child);
    EXPECT_TRUE(retrievedParent.IsValid());
    EXPECT_EQ(retrievedParent, parent);
}

TEST_F(SceneTest, MultipleChildren) {
    Node parent = m_scene.CreateNode("Parent");
    Node c1 = m_scene.CreateNode("C1");
    Node c2 = m_scene.CreateNode("C2");
    Node c3 = m_scene.CreateNode("C3");

    m_scene.SetParent(c1, parent);
    m_scene.SetParent(c2, parent);
    m_scene.SetParent(c3, parent);

    auto children = m_scene.GetChildren(parent);
    EXPECT_EQ(children.size(), 3);
}

TEST_F(SceneTest, RootNodes) {
    Node root1 = m_scene.CreateNode("Root1");
    Node root2 = m_scene.CreateNode("Root2");
    Node child = m_scene.CreateNode("Child");
    m_scene.SetParent(child, root1);

    auto roots = m_scene.GetRootNodes();
    EXPECT_EQ(roots.size(), 2);

    // root1 and root2 should be in root nodes, child should not
    bool foundRoot1 = false, foundRoot2 = false;
    for (const auto& r : roots) {
        if (r == root1) foundRoot1 = true;
        if (r == root2) foundRoot2 = true;
        EXPECT_NE(r, child);
    }
    EXPECT_TRUE(foundRoot1);
    EXPECT_TRUE(foundRoot2);
}

TEST_F(SceneTest, GetParentOfRootNode) {
    Node root = m_scene.CreateNode("Root");
    Node parent = m_scene.GetParent(root);
    EXPECT_FALSE(parent.IsValid());
}

TEST_F(SceneTest, Reparent) {
    Node grandparent = m_scene.CreateNode("Grandparent");
    Node parent = m_scene.CreateNode("Parent");
    Node child = m_scene.CreateNode("Child");

    m_scene.SetParent(parent, grandparent);
    m_scene.SetParent(child, parent);

    // Initially child's parent is 'parent'
    EXPECT_EQ(m_scene.GetParent(child), parent);

    // Reparent: child goes directly under grandparent
    m_scene.SetParent(child, grandparent);

    EXPECT_EQ(m_scene.GetParent(child), grandparent);
    auto gpChildren = m_scene.GetChildren(grandparent);
    EXPECT_EQ(gpChildren.size(), 2);  // parent + child

    auto pChildren = m_scene.GetChildren(parent);
    EXPECT_TRUE(pChildren.empty());
}

TEST_F(SceneTest, DetachParent) {
    Node parent = m_scene.CreateNode("Parent");
    Node child = m_scene.CreateNode("Child");
    m_scene.SetParent(child, parent);

    // Detach: set parent to invalid node
    m_scene.SetParent(child, Node{});

    // Child should now be a root node
    EXPECT_FALSE(m_scene.GetParent(child).IsValid());

    auto children = m_scene.GetChildren(parent);
    EXPECT_TRUE(children.empty());
}

// ============================================================================
// Scene — Transform Hierarchy (GetWorldTransform)
// ============================================================================
TEST_F(SceneTest, WorldTransformIsIdentityForDefaultNode) {
    Node node = m_scene.CreateNode("Node");
    m_scene.AddComponent<Transform>(node);

    Matrix4x4 world = m_scene.GetWorldTransform(node);
    EXPECT_EQ(world, Matrix4x4(1.0f));
}

TEST_F(SceneTest, WorldTransformPropagatesParentTranslation) {
    Node parent = m_scene.CreateNode("Parent");
    Node child = m_scene.CreateNode("Child");
    m_scene.SetParent(child, parent);

    auto parentTransform = m_scene.AddComponent<Transform>(parent);
    auto childTransform = m_scene.AddComponent<Transform>(child);

    // Move parent to (10, 0, 0), child at (0, 0, 0) local
    parentTransform->SetPosition(Vector3(10.0f, 0.0f, 0.0f));

    Matrix4x4 childWorld = m_scene.GetWorldTransform(child);
    // Child's world position should be (10, 0, 0)
    EXPECT_FLOAT_EQ(childWorld[3][0], 10.0f);
    EXPECT_FLOAT_EQ(childWorld[3][1], 0.0f);
    EXPECT_FLOAT_EQ(childWorld[3][2], 0.0f);
}

TEST_F(SceneTest, WorldTransformCombinesParentAndChild) {
    Node parent = m_scene.CreateNode("Parent");
    Node child = m_scene.CreateNode("Child");
    m_scene.SetParent(child, parent);

    auto parentTransform = m_scene.AddComponent<Transform>(parent);
    auto childTransform = m_scene.AddComponent<Transform>(child);

    parentTransform->SetPosition(Vector3(10.0f, 0.0f, 0.0f));
    childTransform->SetPosition(Vector3(0.0f, 5.0f, 0.0f));

    Matrix4x4 childWorld = m_scene.GetWorldTransform(child);
    // Child's world position: parent translation (10,0,0) + child local (0,5,0) = (10,5,0)
    EXPECT_FLOAT_EQ(childWorld[3][0], 10.0f);
    EXPECT_FLOAT_EQ(childWorld[3][1], 5.0f);
    EXPECT_FLOAT_EQ(childWorld[3][2], 0.0f);
}

TEST_F(SceneTest, WorldTransformAfterDetach) {
    Node parent = m_scene.CreateNode("Parent");
    Node child = m_scene.CreateNode("Child");
    m_scene.SetParent(child, parent);

    auto parentTransform = m_scene.AddComponent<Transform>(parent);
    auto childTransform = m_scene.AddComponent<Transform>(child);

    parentTransform->SetPosition(Vector3(100.0f, 0.0f, 0.0f));
    childTransform->SetPosition(Vector3(0.0f, 10.0f, 0.0f));

    // After detaching, child's world should equal its local transform (no parent influence)
    m_scene.SetParent(child, Node{});
    Matrix4x4 childWorld = m_scene.GetWorldTransform(child);
    EXPECT_FLOAT_EQ(childWorld[3][0], 0.0f);
    EXPECT_FLOAT_EQ(childWorld[3][1], 10.0f);
    EXPECT_FLOAT_EQ(childWorld[3][2], 0.0f);
}

TEST_F(SceneTest, DeepHierarchyWorldTransform) {
    // Grandparent → Parent → Child (3 levels)
    Node gp = m_scene.CreateNode("GP");
    Node p = m_scene.CreateNode("P");
    Node c = m_scene.CreateNode("C");

    m_scene.SetParent(p, gp);
    m_scene.SetParent(c, p);

    auto gpT = m_scene.AddComponent<Transform>(gp);
    auto pT = m_scene.AddComponent<Transform>(p);
    auto cT = m_scene.AddComponent<Transform>(c);

    gpT->SetPosition(Vector3(1.0f, 0.0f, 0.0f));
    pT->SetPosition(Vector3(2.0f, 0.0f, 0.0f));
    cT->SetPosition(Vector3(3.0f, 0.0f, 0.0f));

    Matrix4x4 world = m_scene.GetWorldTransform(c);
    EXPECT_FLOAT_EQ(world[3][0], 6.0f);  // 1 + 2 + 3
    EXPECT_FLOAT_EQ(world[3][1], 0.0f);
    EXPECT_FLOAT_EQ(world[3][2], 0.0f);
}

// ============================================================================
// Scene — Component Add / Remove
// ============================================================================
TEST_F(SceneTest, AddComponent) {
    Node node = m_scene.CreateNode("CompNode");
    auto transform = m_scene.AddComponent<Transform>(node);

    EXPECT_NE(transform, nullptr);
    EXPECT_EQ(transform->GetOwnerNode(), node);
    EXPECT_EQ(transform->GetOwnerScene(), &m_scene);
}

TEST_F(SceneTest, GetComponent) {
    Node node = m_scene.CreateNode("CompNode");
    m_scene.AddComponent<Transform>(node);

    auto retrieved = m_scene.GetComponent<Transform>(node);
    EXPECT_NE(retrieved, nullptr);
    EXPECT_FLOAT_EQ(retrieved->GetPosition().x, 0.0f);
}

TEST_F(SceneTest, GetComponentReturnsNullForMissing) {
    Node node = m_scene.CreateNode("NoComp");
    auto retrieved = m_scene.GetComponent<Transform>(node);
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(SceneTest, RemoveComponent) {
    Node node = m_scene.CreateNode("CompNode");
    auto transform = m_scene.AddComponent<Transform>(node);
    ASSERT_NE(transform, nullptr);

    m_scene.RemoveComponent(node, transform.get());

    auto retrieved = m_scene.GetComponent<Transform>(node);
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(SceneTest, MultipleComponentsOnSameNode) {
    Node node = m_scene.CreateNode("MultiComp");
    auto t1 = m_scene.AddComponent<Transform>(node);
    auto t2 = m_scene.AddComponent<Transform>(node);

    // Both should be retrievable
    auto components = m_scene.GetComponents(node);
    EXPECT_EQ(components.size(), 2);
}

TEST_F(SceneTest, RemoveNodeAlsoRemovesComponents) {
    Node node = m_scene.CreateNode("HasComp");
    m_scene.AddComponent<Transform>(node);

    m_scene.RemoveNode(node);
    EXPECT_TRUE(m_scene.GetComponent<Transform>(node) == nullptr);
}

// ============================================================================
// Scene — Main Camera
// ============================================================================
TEST_F(SceneTest, GetMainCamera) {
    Node cameraNode = m_scene.CreateNode("Camera");
    m_scene.AddComponent<Graphic::Camera>(cameraNode);

    auto camera = m_scene.GetMainCamera();
    EXPECT_NE(camera, nullptr);
}

TEST_F(SceneTest, GetMainCameraReturnsNullWithNoCamera) {
    m_scene.CreateNode("NoCameraNode");
    auto camera = m_scene.GetMainCamera();
    EXPECT_EQ(camera, nullptr);
}

// ============================================================================
// Scene — Entity Wrappers
// ============================================================================
TEST_F(SceneTest, CreateEntity) {
    Scene::Entity e = m_scene.CreateEntity("TestEntity");
    EXPECT_NE(e, Scene::INVALID_ENTITY);
    EXPECT_EQ(m_scene.GetNodes().size(), 1);
}

TEST_F(SceneTest, DestroyEntity) {
    Scene::Entity e = m_scene.CreateEntity("ToDestroy");
    ASSERT_EQ(m_scene.GetNodes().size(), 1);

    m_scene.DestroyEntity(e);
    EXPECT_TRUE(m_scene.GetNodes().empty());
}

TEST_F(SceneTest, CreateMultipleEntities) {
    for (int i = 0; i < 10; ++i) {
        m_scene.CreateEntity("Entity" + std::to_string(i));
    }
    EXPECT_EQ(m_scene.GetNodes().size(), 10);
}

// ============================================================================
// Scene — ForEach with two component types
// ============================================================================
TEST_F(SceneTest, ForEachWithTwoComponents) {
    Node a = m_scene.CreateNode("A");
    auto tA = m_scene.AddComponent<Transform>(a);

    Node b = m_scene.CreateNode("B");
    auto tB = m_scene.AddComponent<Transform>(b);

    // Only a has both Transform and Camera
    // (Both have Transform, but only we're checking Camera which only A has)

    // Actually, ForEach checks T1 AND T2. Let's make a node with both Transform and Camera.
    Node camNode = m_scene.CreateNode("Cam");
    m_scene.AddComponent<Transform>(camNode);
    m_scene.AddComponent<Graphic::Camera>(camNode);

    int count = 0;
    m_scene.ForEach<Transform, Graphic::Camera>([&count](Transform&, Graphic::Camera&) {
        ++count;
    });

    EXPECT_EQ(count, 1);  // only camNode has both
}

// ============================================================================
// Scene — Update delegates to components
// ============================================================================
TEST_F(SceneTest, UpdateDoesNotCrash) {
    Node node = m_scene.CreateNode("Node");
    m_scene.AddComponent<Transform>(node);

    // Should not crash
    m_scene.Update(Timestep(1.0f / 60.0f));
}

} // namespace
} // namespace Prisma
