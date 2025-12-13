#include "TestFramework.h"
#include "../src/engine/core/ECS.h"
#include "../src/engine/core/Components.h"

using namespace Engine::Core::ECS;

// 创建测试组件
struct TestComponent : public IComponent {
    static constexpr ComponentTypeID TYPE_ID = 100;
    ComponentTypeID GetTypeID() const override { return TYPE_ID; }
    int value = 0;
};

struct TestComponent2 : public IComponent {
    static constexpr ComponentTypeID TYPE_ID = 101;
    ComponentTypeID GetTypeID() const override { return TYPE_ID; }
    float value = 0.0f;
};

// ECS系统测试套件
class ECSTestSuite : public TestFramework::TestSuite {
public:
    ECSTestSuite() : TestSuite("ECS Tests") {
        AddTest(new CreateEntityTest());
        AddTest(new AddComponentTest());
        AddTest(new GetComponentTest());
        AddTest(new RemoveComponentTest());
        AddTest(new HasComponentTest());
        AddTest(new SystemTest());
    }

private:
    // 创建实体测试
    class CreateEntityTest : public TestFramework::TestCase {
    public:
        CreateEntityTest() : TestFramework::TestCase("CreateEntity") {}
    protected:
        void RunTest() override {
            World& world = World::GetInstance();
            EntityID entity = world.CreateEntity();

            Assert(entity != INVALID_ENTITY, "Entity should be valid");
            Assert(world.IsEntityValid(entity), "Entity should be valid in world");
        }
    };

    // 添加组件测试
    class AddComponentTest : public TestFramework::TestCase {
    public:
        AddComponentTest() : TestFramework::TestCase("AddComponent") {}
    protected:
        void RunTest() override {
            World& world = World::GetInstance();
            EntityID entity = world.CreateEntity();

            auto* component = world.AddComponent<TestComponent>(entity);
            AssertNotNull(component, "Component should not be null");
            AssertEquals(0, component->value);
        }
    };

    // 获取组件测试
    class GetComponentTest : public TestFramework::TestCase {
    public:
        GetComponentTest() : TestFramework::TestCase("GetComponent") {}
    protected:
        void RunTest() override {
            World& world = World::GetInstance();
            EntityID entity = world.CreateEntity();

            auto* added = world.AddComponent<TestComponent>(entity);
            added->value = 42;

            auto* component = world.GetComponent<TestComponent>(entity);
            AssertNotNull(component, "Component should not be null");
            AssertEquals(42, component->value);
            Assert(added == component, "Should return same component instance");
        }
    };

    // 移除组件测试
    class RemoveComponentTest : public TestFramework::TestCase {
    public:
        RemoveComponentTest() : TestFramework::TestCase("RemoveComponent") {}
    protected:
        void RunTest() override {
            World& world = World::GetInstance();
            EntityID entity = world.CreateEntity();

            world.AddComponent<TestComponent>(entity);
            AssertTrue(world.HasComponent<TestComponent>(entity), "Should have component");

            world.RemoveComponent<TestComponent>(entity);
            AssertFalse(world.HasComponent<TestComponent>(entity), "Should not have component");
        }
    };

    // 检查组件测试
    class HasComponentTest : public TestFramework::TestCase {
    public:
        HasComponentTest() : TestFramework::TestCase("HasComponent") {}
    protected:
        void RunTest() override {
            World& world = World::GetInstance();
            EntityID entity = world.CreateEntity();

            AssertFalse(world.HasComponent<TestComponent>(entity), "Should not have component initially");

            world.AddComponent<TestComponent>(entity);
            AssertTrue(world.HasComponent<TestComponent>(entity), "Should have component after adding");
        }
    };

    // 系统测试
    class SystemTest : public TestFramework::TestCase {
    public:
        SystemTest() : TestFramework::TestCase("System") {}
    protected:
        void RunTest() override {
            World& world = World::GetInstance();

            // 添加系统
            auto* system = world.AddSystem<RenderSystem>();
            AssertNotNull(system, "System should not be null");
            AssertTrue(system->enabled, "System should be enabled by default");

            // 获取系统
            auto* retrieved = world.GetSystem<RenderSystem>();
            AssertNotNull(retrieved, "Should retrieve system");
            Assert(system == retrieved, "Should return same system");
        }
    };
};

// 注册测试套件
REGISTER_TEST_SUITE(ECSTestSuite)