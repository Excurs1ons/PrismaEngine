#include <gtest/gtest.h>
#include "terrain/HeightMap.h"
#include "terrain/TerrainCollision.h"

namespace Prisma {
namespace Terrain {
namespace {

// ============================================================================
// TerrainCollision — Default State
// ============================================================================
TEST(TerrainCollisionTest, DefaultConstructor) {
    TerrainCollision tc;
    EXPECT_EQ(tc.GetHeightMap(), nullptr);
    // With no heightmap, these should return safe defaults
    EXPECT_FLOAT_EQ(tc.GetHeightAt(0.0f, 0.0f), 0.0f);
    Vector3 normal = tc.GetNormalAt(0.0f, 0.0f);
    EXPECT_FLOAT_EQ(normal.y, 1.0f);
}

// ============================================================================
// TerrainCollision — HeightMap Reference
// ============================================================================
TEST(TerrainCollisionTest, SetAndGetHeightMap) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 10.0f);
    TerrainCollision tc;
    tc.SetHeightMap(&hm);
    EXPECT_EQ(tc.GetHeightMap(), &hm);
    EXPECT_FLOAT_EQ(tc.GetHeightAt(0.0f, 0.0f), 10.0f);
}

TEST(TerrainCollisionTest, ConstructorWithHeightMap) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 25.0f);
    TerrainCollision tc(&hm);
    EXPECT_EQ(tc.GetHeightMap(), &hm);
    EXPECT_FLOAT_EQ(tc.GetHeightAt(5.0f, -3.0f), 25.0f);
}

// ============================================================================
// TerrainCollision — Surface Queries with Flat Terrain
// ============================================================================
TEST(TerrainCollisionTest, GetHeightAtFlat) {
    HeightMap hm;
    hm.GenerateFlat(32, 32, 50.0f);
    TerrainCollision tc(&hm);
    // Flat terrain → same height everywhere
    EXPECT_FLOAT_EQ(tc.GetHeightAt(0.0f, 0.0f), 50.0f);
    EXPECT_FLOAT_EQ(tc.GetHeightAt(10.0f, 10.0f), 50.0f);
    EXPECT_FLOAT_EQ(tc.GetHeightAt(-15.0f, 5.0f), 50.0f);
}

TEST(TerrainCollisionTest, GetNormalAtFlat) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 0.0f);
    TerrainCollision tc(&hm);
    Vector3 normal = tc.GetNormalAt(0.0f, 0.0f);
    EXPECT_NEAR(normal.x, 0.0f, 0.001f);
    EXPECT_NEAR(normal.y, 1.0f, 0.001f);
    EXPECT_NEAR(normal.z, 0.0f, 0.001f);
}

// ============================================================================
// TerrainCollision — Surface Queries with Varied Terrain
// ============================================================================
TEST(TerrainCollisionTest, GetHeightAtVariedTerrain) {
    HeightMap hm;
    hm.GenerateTestTerrain(32, 32);
    TerrainCollision tc(&hm);
    // Terrain should have different heights at different positions
    float h1 = tc.GetHeightAt(0.0f, 0.0f);
    float h2 = tc.GetHeightAt(5.0f, 5.0f);
    // Both should be within the terrain range
    EXPECT_GE(h1, hm.GetMinHeight());
    EXPECT_LE(h1, hm.GetMaxHeight());
    EXPECT_GE(h2, hm.GetMinHeight());
    EXPECT_LE(h2, hm.GetMaxHeight());
}

// ============================================================================
// TerrainCollision — TestAABB
// ============================================================================
TEST(TerrainCollisionTest, TestAABB_AboveTerrain) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 10.0f);
    TerrainCollision tc(&hm);
    // AABB completely above terrain
    Vector3 boxMin(-5.0f, 20.0f, -5.0f);
    Vector3 boxMax(5.0f, 30.0f, 5.0f);
    EXPECT_FALSE(tc.TestAABB(boxMin, boxMax));
}

TEST(TerrainCollisionTest, TestAABB_BelowTerrainSurface) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 10.0f);
    TerrainCollision tc(&hm);
    // AABB bottom is below terrain (y=10)
    Vector3 boxMin(-5.0f, 5.0f, -5.0f);
    Vector3 boxMax(5.0f, 15.0f, 5.0f);
    EXPECT_TRUE(tc.TestAABB(boxMin, boxMax));
}

TEST(TerrainCollisionTest, TestAABB_ExactlyAtTerrain) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 10.0f);
    TerrainCollision tc(&hm);
    // AABB bottom exactly at terrain level
    Vector3 boxMin(-5.0f, 10.0f, -5.0f);
    Vector3 boxMax(5.0f, 20.0f, 5.0f);
    // At exact terrain level, should not be below, so no collision
    EXPECT_FALSE(tc.TestAABB(boxMin, boxMax));
}

TEST(TerrainCollisionTest, TestAABB_PartiallyBelow) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 10.0f);
    TerrainCollision tc(&hm);
    // One corner below terrain, rest above
    Vector3 boxMin(-15.0f, 5.0f, -15.0f); // Bottom at y=5, below terrain
    Vector3 boxMax(15.0f, 20.0f, 15.0f);
    EXPECT_TRUE(tc.TestAABB(boxMin, boxMax));
}

// ============================================================================
// TerrainCollision — ResolveAABB
// ============================================================================
TEST(TerrainCollisionTest, ResolveAABB_NoCollision) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 10.0f);
    TerrainCollision tc(&hm);
    Vector3 boxMin(-5.0f, 20.0f, -5.0f);
    Vector3 boxMax(5.0f, 30.0f, 5.0f);
    auto [penetration, normal] = tc.ResolveAABB(boxMin, boxMax);
    EXPECT_LE(penetration, 0.0f);
}

TEST(TerrainCollisionTest, ResolveAABB_Collision) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 100.0f);
    TerrainCollision tc(&hm);
    // Box at y=50, terrain at y=100 → penetration = 50
    Vector3 boxMin(-5.0f, 50.0f, -5.0f);
    Vector3 boxMax(5.0f, 60.0f, 5.0f);
    auto [penetration, normal] = tc.ResolveAABB(boxMin, boxMax);
    EXPECT_GT(penetration, 0.0f);
    EXPECT_NEAR(penetration, 50.0f, 0.01f);
    // Normal should point upward
    EXPECT_GT(normal.y, 0.0f);
}

// ============================================================================
// TerrainCollision — Raycast
// ============================================================================
TEST(TerrainCollisionTest, RaycastHitFromAbove) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 0.0f);
    TerrainCollision tc(&hm);
    // Ray from above pointing down
    Vector3 origin(0.0f, 100.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);
    TerrainRaycastHit hit = tc.Raycast(origin, direction, 200.0f, 1.0f);
    EXPECT_TRUE(hit.valid);
    EXPECT_NEAR(hit.point.y, 0.0f, 1.0f);
    EXPECT_NEAR(hit.distance, 100.0f, 1.0f);
    EXPECT_NEAR(hit.normal.y, 1.0f, 0.1f);
}

TEST(TerrainCollisionTest, RaycastMissDirection) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 0.0f);
    TerrainCollision tc(&hm);
    // Ray pointing away from terrain (upward)
    Vector3 origin(0.0f, 10.0f, 0.0f);
    Vector3 direction(0.0f, 1.0f, 0.0f);
    TerrainRaycastHit hit = tc.Raycast(origin, direction, 100.0f, 1.0f);
    EXPECT_FALSE(hit.valid);
}

TEST(TerrainCollisionTest, RaycastMissTooShort) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 0.0f);
    TerrainCollision tc(&hm);
    // Ray from above but max distance too short
    Vector3 origin(0.0f, 100.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);
    TerrainRaycastHit hit = tc.Raycast(origin, direction, 10.0f, 1.0f);
    EXPECT_FALSE(hit.valid);
}

TEST(TerrainCollisionTest, RaycastStartBelowTerrain) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 10.0f);
    TerrainCollision tc(&hm);
    Vector3 origin(0.0f, -10.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);
    TerrainRaycastHit hit = tc.Raycast(origin, direction, 100.0f, 1.0f);
    EXPECT_TRUE(hit.valid);
    EXPECT_NEAR(hit.point.y, -9.0f, 1.0f);
}

TEST(TerrainCollisionTest, RaycastFromSide) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 0.0f);
    TerrainCollision tc(&hm);
    // Ray from side (horizontal) - should miss since terrain only has height
    Vector3 origin(100.0f, 10.0f, 0.0f);
    Vector3 direction(-1.0f, 0.0f, 0.0f);
    TerrainRaycastHit hit = tc.Raycast(origin, direction, 200.0f, 1.0f);
    EXPECT_FALSE(hit.valid);
}

TEST(TerrainCollisionTest, RaycastDiagonalHit) {
    HeightMap hm;
    hm.GenerateFlat(32, 32, 0.0f);
    TerrainCollision tc(&hm);
    // Diagonal ray from above
    Vector3 origin(10.0f, 50.0f, 10.0f);
    Vector3 direction = glm::normalize(Vector3(-0.2f, -1.0f, -0.2f));
    TerrainRaycastHit hit = tc.Raycast(origin, direction, 200.0f, 0.5f);
    EXPECT_TRUE(hit.valid);
    EXPECT_GT(hit.distance, 0.0f);
    EXPECT_NEAR(hit.point.y, 0.0f, 1.0f);
}

// ============================================================================
// TerrainCollision — IsBelowTerrain / ProjectToSurface
// ============================================================================
TEST(TerrainCollisionTest, IsBelowTerrain) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 10.0f);
    TerrainCollision tc(&hm);
    // Point above terrain
    EXPECT_FALSE(tc.IsBelowTerrain(Vector3(0.0f, 20.0f, 0.0f)));
    // Point below terrain
    EXPECT_TRUE(tc.IsBelowTerrain(Vector3(0.0f, 5.0f, 0.0f)));
    // Point exactly at terrain level: implementation uses <=
    EXPECT_TRUE(tc.IsBelowTerrain(Vector3(0.0f, 10.0f, 0.0f)));
}

TEST(TerrainCollisionTest, ProjectToSurface) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 42.0f);
    TerrainCollision tc(&hm);
    Vector3 point(10.0f, 0.0f, -5.0f);
    Vector3 projected = tc.ProjectToSurface(point);
    EXPECT_FLOAT_EQ(projected.x, 10.0f);
    EXPECT_FLOAT_EQ(projected.y, 42.0f);
    EXPECT_FLOAT_EQ(projected.z, -5.0f);
}

// ============================================================================
// TerrainCollision — Null HeightMap Edge Cases
// ============================================================================
TEST(TerrainCollisionTest, NullHeightMapTestAABB) {
    TerrainCollision tc;
    // Without a heightmap, TestAABB should return false (no collision)
    EXPECT_FALSE(tc.TestAABB(Vector3(-10, -10, -10), Vector3(10, 10, 10)));
}

TEST(TerrainCollisionTest, NullHeightMapResolveAABB) {
    TerrainCollision tc;
    auto [penetration, normal] = tc.ResolveAABB(Vector3(0, 0, 0), Vector3(1, 1, 1));
    EXPECT_LE(penetration, 0.0f);
}

TEST(TerrainCollisionTest, NullHeightMapIsBelowTerrain) {
    TerrainCollision tc;
    EXPECT_FALSE(tc.IsBelowTerrain(Vector3(0.0f, 0.0f, 0.0f)));
}

TEST(TerrainCollisionTest, NullHeightMapProjectToSurface) {
    TerrainCollision tc;
    Vector3 p(1.0f, 2.0f, 3.0f);
    Vector3 projected = tc.ProjectToSurface(p);
    // Without heightmap, should return the same point (y unchanged)
    EXPECT_FLOAT_EQ(projected.x, 1.0f);
    EXPECT_FLOAT_EQ(projected.y, 2.0f);
    EXPECT_FLOAT_EQ(projected.z, 3.0f);
}

// ============================================================================
// TerrainCollision — Raycast with Varied Terrain
// ============================================================================
TEST(TerrainCollisionTest, RaycastHitVariedTerrain) {
    HeightMap hm;
    hm.GenerateTestTerrain(32, 32, 0.02f, 20.0f);
    TerrainCollision tc(&hm);
    // Ray from far above pointing down should hit terrain
    Vector3 origin(0.0f, 200.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);
    TerrainRaycastHit hit = tc.Raycast(origin, direction, 500.0f, 1.0f);
    EXPECT_TRUE(hit.valid);
    // Hit point should be within terrain height range
    EXPECT_GE(hit.point.y, hm.GetMinHeight());
    EXPECT_LE(hit.point.y, hm.GetMaxHeight());
    EXPECT_GT(hit.distance, 0.0f);
}

} // namespace
} // namespace Terrain
} // namespace Prisma
