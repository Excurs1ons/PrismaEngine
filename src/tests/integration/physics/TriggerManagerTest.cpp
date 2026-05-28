#include <gtest/gtest.h>
#include "physics/TriggerManager.h"

namespace Prisma {
namespace Physics {
namespace {

// ============================================================================
// TriggerManager — Basic Operations
// ============================================================================
TEST(TriggerManagerTest, DefaultState) {
    TriggerManager mgr;
    EXPECT_EQ(mgr.getTriggerCount(), 0);
}

TEST(TriggerManagerTest, AddTrigger) {
    TriggerManager mgr;
    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(0, 0, 0, 10, 10, 10)
    );
    uint32_t id = mgr.addTrigger(tv);
    EXPECT_GT(id, 0);
    EXPECT_EQ(mgr.getTriggerCount(), 1);
}

TEST(TriggerManagerTest, RemoveTrigger) {
    TriggerManager mgr;
    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(0, 0, 0, 10, 10, 10)
    );
    uint32_t id = mgr.addTrigger(tv);
    EXPECT_EQ(mgr.getTriggerCount(), 1);

    mgr.removeTrigger(id);
    EXPECT_EQ(mgr.getTriggerCount(), 0);
}

TEST(TriggerManagerTest, RemoveNonExistentTrigger) {
    TriggerManager mgr;
    // Should not crash when removing non-existent trigger
    mgr.removeTrigger(999);
    SUCCEED();
}

TEST(TriggerManagerTest, GetTrigger) {
    TriggerManager mgr;
    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(5, 5, 5, 15, 15, 15)
    );
    uint32_t id = mgr.addTrigger(tv);

    TriggerVolume* retrieved = mgr.getTrigger(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->shapeType, TriggerShapeType::AABB);
    EXPECT_DOUBLE_EQ(retrieved->aabb.minX, 5.0);
    EXPECT_DOUBLE_EQ(retrieved->aabb.maxX, 15.0);
}

TEST(TriggerManagerTest, GetNonExistentTrigger) {
    TriggerManager mgr;
    TriggerVolume* retrieved = mgr.getTrigger(999);
    EXPECT_EQ(retrieved, nullptr);
}

TEST(TriggerManagerTest, ClearAllTriggers) {
    TriggerManager mgr;
    mgr.addTrigger(TriggerVolume::createAABB(AABB(0, 0, 0, 5, 5, 5)));
    mgr.addTrigger(TriggerVolume::createSphere(glm::dvec3(0, 0, 0), 5.0));
    EXPECT_EQ(mgr.getTriggerCount(), 2);

    mgr.clear();
    EXPECT_EQ(mgr.getTriggerCount(), 0);
}

// ============================================================================
// TriggerManager — AABB Trigger Zone
// ============================================================================
TEST(TriggerManagerTest, AABBTriggerEnterEvent) {
    TriggerManager mgr;
    int enterCount = 0;
    int stayCount = 0;
    int exitCount = 0;

    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(0, 0, 0, 10, 10, 10),
        [&enterCount](uint32_t) { enterCount++; },
        [&exitCount](uint32_t) { exitCount++; },
        [&stayCount](uint32_t) { stayCount++; }
    );
    mgr.addTrigger(tv);

    // Entity enters the trigger zone
    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(5, 5, 5, 6, 6, 6)}  // Entity inside trigger
    };

    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1) << "Entity should trigger enter event";
    EXPECT_EQ(stayCount, 0) << "No stay events on first frame of entry";
    EXPECT_EQ(exitCount, 0) << "No exit events";
}

TEST(TriggerManagerTest, AABBTriggerStayEvent) {
    TriggerManager mgr;
    int enterCount = 0;
    int stayCount = 0;

    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(0, 0, 0, 10, 10, 10),
        [&enterCount](uint32_t) { enterCount++; },
        nullptr,
        [&stayCount](uint32_t) { stayCount++; }
    );
    mgr.addTrigger(tv);

    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(5, 5, 5, 6, 6, 6)}
    };

    // First update: enter
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1);
    EXPECT_EQ(stayCount, 0);

    // Second update: stay
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1);   // No new enter
    EXPECT_EQ(stayCount, 1);    // Stay event fired
}

TEST(TriggerManagerTest, AABBTriggerExitEvent) {
    TriggerManager mgr;
    int enterCount = 0;
    int exitCount = 0;

    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(0, 0, 0, 10, 10, 10),
        [&enterCount](uint32_t) { enterCount++; },
        [&exitCount](uint32_t) { exitCount++; }
    );
    mgr.addTrigger(tv);

    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(5, 5, 5, 6, 6, 6)}
    };

    // First update: enter
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1);

    // Entity leaves
    std::vector<std::pair<uint32_t, AABB>> emptyEntities;
    mgr.update(1.0 / 60.0, emptyEntities);
    EXPECT_EQ(enterCount, 1);  // No new enter
    EXPECT_EQ(exitCount, 1);   // Exit event fired
}

TEST(TriggerManagerTest, AABBTriggerMultipleEntities) {
    TriggerManager mgr;
    int enterCount = 0;
    int exitCount = 0;

    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(0, 0, 0, 10, 10, 10),
        [&enterCount](uint32_t) { enterCount++; },
        [&exitCount](uint32_t) { exitCount++; }
    );
    mgr.addTrigger(tv);

    // Two entities enter
    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(1, 1, 1, 2, 2, 2)},
        {2, AABB(3, 3, 3, 4, 4, 4)}
    };
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 2);

    // One entity leaves
    std::vector<std::pair<uint32_t, AABB>> oneEntity = {
        {1, AABB(1, 1, 1, 2, 2, 2)}
    };
    mgr.update(1.0 / 60.0, oneEntity);
    EXPECT_EQ(exitCount, 1);  // Only entity 2 should trigger exit
}

// ============================================================================
// TriggerManager — Sphere Trigger Zone
// ============================================================================
TEST(TriggerManagerTest, SphereTriggerEnterEvent) {
    TriggerManager mgr;
    int enterCount = 0;

    TriggerVolume tv = TriggerVolume::createSphere(
        glm::dvec3(0, 0, 0),
        10.0,
        [&enterCount](uint32_t) { enterCount++; }
    );
    mgr.addTrigger(tv);

    // Entity close to sphere center
    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(1, 1, 1, 2, 2, 2)}  // Distance ~1.7 < radius 10
    };
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1);
}

TEST(TriggerManagerTest, SphereTriggerNoEnterIfOutside) {
    TriggerManager mgr;
    int enterCount = 0;

    TriggerVolume tv = TriggerVolume::createSphere(
        glm::dvec3(0, 0, 0),
        5.0,
        [&enterCount](uint32_t) { enterCount++; }
    );
    mgr.addTrigger(tv);

    // Entity far from sphere center
    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(20, 20, 20, 21, 21, 21)}  // Distance > radius
    };
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 0);
}

// ============================================================================
// TriggerManager — Disabled Trigger
// ============================================================================
TEST(TriggerManagerTest, DisabledTriggerDoesNotFireEvents) {
    TriggerManager mgr;
    int enterCount = 0;

    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(0, 0, 0, 10, 10, 10),
        [&enterCount](uint32_t) { enterCount++; }
    );
    tv.enabled = false;
    mgr.addTrigger(tv);

    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(5, 5, 5, 6, 6, 6)}
    };
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 0) << "Disabled trigger should not fire events";
}

// ============================================================================
// TriggerManager — Box (OBB) Trigger Zone
// ============================================================================
TEST(TriggerManagerTest, BoxTriggerDetectsOverlap) {
    TriggerManager mgr;
    int enterCount = 0;

    TriggerVolume tv = TriggerVolume::createBox(
        glm::dvec3(0, 0, 0),
        glm::dquat(1.0, 0.0, 0.0, 0.0),  // Identity rotation
        glm::dvec3(5.0, 5.0, 5.0),
        [&enterCount](uint32_t) { enterCount++; }
    );
    mgr.addTrigger(tv);

    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(1, 1, 1, 2, 2, 2)}  // Inside box
    };
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1);
}

// ============================================================================
// TriggerManager — Lifecycle: Enter → Stay → Exit
// ============================================================================
TEST(TriggerManagerTest, FullLifecycle) {
    TriggerManager mgr;
    int enterCount = 0;
    int stayCount = 0;
    int exitCount = 0;

    TriggerVolume tv = TriggerVolume::createAABB(
        AABB(0, 0, 0, 10, 10, 10),
        [&enterCount](uint32_t) { enterCount++; },
        [&exitCount](uint32_t) { exitCount++; },
        [&stayCount](uint32_t) { stayCount++; }
    );
    mgr.addTrigger(tv);

    std::vector<std::pair<uint32_t, AABB>> entities = {
        {1, AABB(5, 5, 5, 6, 6, 6)}
    };

    // Frame 1: Enter
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1);
    EXPECT_EQ(stayCount, 0);
    EXPECT_EQ(exitCount, 0);

    // Frame 2: Stay
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1);
    EXPECT_EQ(stayCount, 1);
    EXPECT_EQ(exitCount, 0);

    // Frame 3: Stay again
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 1);
    EXPECT_EQ(stayCount, 2);
    EXPECT_EQ(exitCount, 0);

    // Frame 4: Exit (entity removed)
    std::vector<std::pair<uint32_t, AABB>> empty;
    mgr.update(1.0 / 60.0, empty);
    EXPECT_EQ(enterCount, 1);
    EXPECT_EQ(stayCount, 2);
    EXPECT_EQ(exitCount, 1);

    // Frame 5: Entity returns — should enter again
    mgr.update(1.0 / 60.0, entities);
    EXPECT_EQ(enterCount, 2);
    EXPECT_EQ(exitCount, 1);
}

// ============================================================================
// TriggerVolume — Contains
// ============================================================================
TEST(TriggerVolumeTest, PointInsideAABB) {
    TriggerVolume tv = TriggerVolume::createAABB(AABB(0, 0, 0, 10, 10, 10));
    EXPECT_TRUE(tv.contains(glm::dvec3(5, 5, 5)));
    EXPECT_FALSE(tv.contains(glm::dvec3(15, 5, 5)));
}

TEST(TriggerVolumeTest, PointInsideSphere) {
    TriggerVolume tv = TriggerVolume::createSphere(glm::dvec3(0, 0, 0), 5.0);
    EXPECT_TRUE(tv.contains(glm::dvec3(3, 0, 0)));
    EXPECT_FALSE(tv.contains(glm::dvec3(10, 0, 0)));
}

TEST(TriggerVolumeTest, PointInsideBox) {
    TriggerVolume tv = TriggerVolume::createBox(
        glm::dvec3(0, 0, 0),
        glm::dquat(1.0, 0.0, 0.0, 0.0),
        glm::dvec3(5.0, 5.0, 5.0)
    );
    EXPECT_TRUE(tv.contains(glm::dvec3(2, 2, 2)));
    EXPECT_FALSE(tv.contains(glm::dvec3(10, 0, 0)));
}

// ============================================================================
// TriggerVolume — Overlaps with AABB
// ============================================================================
TEST(TriggerVolumeTest, AABBOverlapsWithAABBTrigger) {
    TriggerVolume tv = TriggerVolume::createAABB(AABB(0, 0, 0, 5, 5, 5));
    AABB entityBox(2, 2, 2, 4, 4, 4);
    EXPECT_TRUE(tv.overlaps(entityBox));

    AABB outsideBox(10, 10, 10, 12, 12, 12);
    EXPECT_FALSE(tv.overlaps(outsideBox));
}

TEST(TriggerVolumeTest, AABBOverlapsWithSphereTrigger) {
    TriggerVolume tv = TriggerVolume::createSphere(glm::dvec3(0, 0, 0), 5.0);
    AABB entityBox(1, 1, 1, 2, 2, 2);
    EXPECT_TRUE(tv.overlaps(entityBox));

    AABB outsideBox(20, 20, 20, 21, 21, 21);
    EXPECT_FALSE(tv.overlaps(outsideBox));
}

TEST(TriggerVolumeTest, DisabledTriggerNoOverlap) {
    TriggerVolume tv = TriggerVolume::createAABB(AABB(0, 0, 0, 5, 5, 5));
    tv.enabled = false;
    AABB entityBox(2, 2, 2, 4, 4, 4);
    EXPECT_FALSE(tv.overlaps(entityBox));
}

} // namespace
} // namespace Physics
} // namespace Prisma
