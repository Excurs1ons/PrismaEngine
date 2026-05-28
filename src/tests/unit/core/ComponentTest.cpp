#include <gtest/gtest.h>
#include "core/Component.h"
#include <memory>

namespace Prisma {
namespace {

// A concrete test component for testing Component base class
class TestComponent : public Component {
public:
    static ComponentId GetStaticComponentId() {
        static const ComponentId id = GetComponentTypeId<TestComponent>();
        return id;
    }

    ComponentId GetComponentId() const override {
        return GetStaticComponentId();
    }

    const char* GetComponentTypeName() const override {
        return "TestComponent";
    }

    // Track lifecycle calls
    bool initialized = false;
    bool updated = false;
    bool shutdown = false;
    int updateCount = 0;

    void Initialize() override { initialized = true; }
    void Update(Timestep /*ts*/) override { updated = true; updateCount++; }
    void Shutdown() override { shutdown = true; }
};

// Another component for testing unique type IDs
class AnotherTestComponent : public Component {
public:
    static ComponentId GetStaticComponentId() {
        static const ComponentId id = GetComponentTypeId<AnotherTestComponent>();
        return id;
    }

    ComponentId GetComponentId() const override {
        return GetStaticComponentId();
    }
};

// Component ID is unique per type
TEST(ComponentTest, UniqueTypeId) {
    ComponentId id1 = GetComponentTypeId<TestComponent>();
    ComponentId id2 = GetComponentTypeId<AnotherTestComponent>();
    EXPECT_NE(id1, id2);
}

// Same type returns same ComponentId
TEST(ComponentTest, SameTypeSameId) {
    ComponentId id1 = GetComponentTypeId<TestComponent>();
    ComponentId id2 = GetComponentTypeId<TestComponent>();
    EXPECT_EQ(id1, id2);
}

// Component ID from instance matches type ID
TEST(ComponentTest, InstanceIdMatchesTypeId) {
    auto comp = std::make_shared<TestComponent>();
    EXPECT_EQ(comp->GetComponentId(), TestComponent::GetStaticComponentId());
}

// Component type name
TEST(ComponentTest, TypeName) {
    auto comp = std::make_shared<TestComponent>();
    EXPECT_STREQ(comp->GetComponentTypeName(), "TestComponent");
}

// Component lifecycle: Initialize, Update, Shutdown
TEST(ComponentTest, LifecycleMethods) {
    auto comp = std::make_shared<TestComponent>();

    EXPECT_FALSE(comp->initialized);
    EXPECT_FALSE(comp->updated);
    EXPECT_FALSE(comp->shutdown);
    EXPECT_EQ(comp->updateCount, 0);

    comp->Initialize();
    EXPECT_TRUE(comp->initialized);

    comp->Update(Timestep(1.0f));
    EXPECT_TRUE(comp->updated);
    EXPECT_EQ(comp->updateCount, 1);

    comp->Update(Timestep(0.5f));
    EXPECT_EQ(comp->updateCount, 2);

    comp->Shutdown();
    EXPECT_TRUE(comp->shutdown);
}

// Component enabled/disabled
TEST(ComponentTest, EnabledDisabled) {
    auto comp = std::make_shared<TestComponent>();
    EXPECT_TRUE(comp->IsEnabled());

    comp->SetEnabled(false);
    EXPECT_FALSE(comp->IsEnabled());

    comp->SetEnabled(true);
    EXPECT_TRUE(comp->IsEnabled());
}

// Default component has no owner
TEST(ComponentTest, DefaultNoOwner) {
    auto comp = std::make_shared<TestComponent>();
    EXPECT_EQ(comp->GetOwnerNode().handle, 0u);
    EXPECT_EQ(comp->GetOwnerScene(), nullptr);
}

// ComponentId is never zero (0 is reserved as invalid)
TEST(ComponentTest, ComponentIdNonZero) {
    ComponentId id = GetComponentTypeId<TestComponent>();
    EXPECT_NE(id, 0u);
}

} // namespace
} // namespace Prisma
