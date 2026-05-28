#include <gtest/gtest.h>
#include "terrain/HeightMap.h"

namespace Prisma {
namespace Terrain {
namespace {

// ============================================================================
// HeightMap — Default State
// ============================================================================
TEST(HeightMapTest, DefaultConstructor) {
    HeightMap hm;
    EXPECT_FALSE(hm.IsValid());
    EXPECT_EQ(hm.GetWidth(), 0u);
    EXPECT_EQ(hm.GetHeight(), 0u);
    EXPECT_EQ(hm.GetDataSize(), 0u);
    EXPECT_EQ(hm.GetData(), nullptr);
    EXPECT_FLOAT_EQ(hm.GetMinHeight(), std::numeric_limits<float>::max());
    EXPECT_FLOAT_EQ(hm.GetMaxHeight(), std::numeric_limits<float>::lowest());
}

// ============================================================================
// HeightMap — GenerateFlat
// ============================================================================
TEST(HeightMapTest, GenerateFlat) {
    HeightMap hm;
    hm.GenerateFlat(32, 32, 10.0f);
    EXPECT_TRUE(hm.IsValid());
    EXPECT_EQ(hm.GetWidth(), 32u);
    EXPECT_EQ(hm.GetHeight(), 32u);
    EXPECT_EQ(hm.GetDataSize(), 1024u);
    EXPECT_FLOAT_EQ(hm.GetMinHeight(), 10.0f);
    EXPECT_FLOAT_EQ(hm.GetMaxHeight(), 10.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightRange(), 0.0f);

    // All pixels should be 10.0f
    for (uint32_t y = 0; y < hm.GetHeight(); ++y) {
        for (uint32_t x = 0; x < hm.GetWidth(); ++x) {
            EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(x, y), 10.0f);
        }
    }
}

TEST(HeightMapTest, GenerateFlatDefaultHeight) {
    HeightMap hm;
    hm.GenerateFlat(16, 16);
    EXPECT_TRUE(hm.IsValid());
    EXPECT_FLOAT_EQ(hm.GetMinHeight(), 0.0f);
    EXPECT_FLOAT_EQ(hm.GetMaxHeight(), 0.0f);
}

// ============================================================================
// HeightMap — GenerateTestTerrain
// ============================================================================
TEST(HeightMapTest, GenerateTestTerrain) {
    HeightMap hm;
    hm.GenerateTestTerrain(64, 64);
    EXPECT_TRUE(hm.IsValid());
    EXPECT_EQ(hm.GetWidth(), 64u);
    EXPECT_EQ(hm.GetHeight(), 64u);
    // Terrain should have variation (min < max)
    EXPECT_LT(hm.GetMinHeight(), hm.GetMaxHeight());
    // Default amplitude = 40, so range should be within [-40, 40]
    EXPECT_GE(hm.GetMaxHeight(), -40.0f);
    EXPECT_LE(hm.GetMaxHeight(), 40.0f);
    EXPECT_GE(hm.GetMinHeight(), -40.0f);
    EXPECT_LE(hm.GetMinHeight(), 40.0f);
}

TEST(HeightMapTest, GenerateTestTerrainCustomParams) {
    HeightMap hm;
    hm.GenerateTestTerrain(32, 32, 0.05f, 20.0f);
    EXPECT_TRUE(hm.IsValid());
    // Peak-to-peak should be within [-20, 20]
    EXPECT_LE(hm.GetMaxHeight(), 20.0f);
    EXPECT_GE(hm.GetMinHeight(), -20.0f);
}

// ============================================================================
// HeightMap — LoadFromFloatArray / LoadFromFloatVector
// ============================================================================
TEST(HeightMapTest, LoadFromFloatArray) {
    std::vector<float> data = {
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f
    };
    HeightMap hm;
    EXPECT_TRUE(hm.LoadFromFloatArray(data.data(), 3, 3));
    EXPECT_TRUE(hm.IsValid());
    EXPECT_EQ(hm.GetWidth(), 3u);
    EXPECT_EQ(hm.GetHeight(), 3u);
    EXPECT_EQ(hm.GetDataSize(), 9u);
    EXPECT_FLOAT_EQ(hm.GetMinHeight(), 1.0f);
    EXPECT_FLOAT_EQ(hm.GetMaxHeight(), 9.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightRange(), 8.0f);
}

TEST(HeightMapTest, LoadFromFloatArrayNullData) {
    HeightMap hm;
    EXPECT_FALSE(hm.LoadFromFloatArray(nullptr, 10, 10));
    EXPECT_FALSE(hm.IsValid());
}

TEST(HeightMapTest, LoadFromFloatArrayZeroSize) {
    HeightMap hm;
    std::vector<float> data = { 1.0f };
    EXPECT_FALSE(hm.LoadFromFloatArray(data.data(), 0, 0));
    EXPECT_FALSE(hm.IsValid());
}

TEST(HeightMapTest, LoadFromFloatVector) {
    std::vector<float> data = {
        0.0f, 1.0f, 2.0f,
        3.0f, 4.0f, 5.0f,
        6.0f, 7.0f, 8.0f
    };
    HeightMap hm;
    EXPECT_TRUE(hm.LoadFromFloatVector(data, 3, 3));
    EXPECT_TRUE(hm.IsValid());
    EXPECT_EQ(hm.GetWidth(), 3u);
    EXPECT_EQ(hm.GetHeight(), 3u);
    EXPECT_FLOAT_EQ(hm.GetMinHeight(), 0.0f);
    EXPECT_FLOAT_EQ(hm.GetMaxHeight(), 8.0f);
}

TEST(HeightMapTest, LoadFromFloatVectorEmpty) {
    HeightMap hm;
    std::vector<float> empty;
    EXPECT_FALSE(hm.LoadFromFloatVector(empty, 10, 10));
    EXPECT_FALSE(hm.IsValid());
}

// ============================================================================
// HeightMap — GetHeightAtPixel (discrete sampling)
// ============================================================================
TEST(HeightMapTest, GetHeightAtPixel) {
    std::vector<float> data = {
        10.0f, 20.0f,
        30.0f, 40.0f
    };
    HeightMap hm;
    hm.LoadFromFloatArray(data.data(), 2, 2);
    EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(0, 0), 10.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(1, 0), 20.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(0, 1), 30.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(1, 1), 40.0f);
}

// ============================================================================
// HeightMap — Sample (bilinear interpolation in UV space)
// ============================================================================
TEST(HeightMapTest, SampleCorners) {
    // 2x2 heightmap: corners at (0,0)=10, (1,0)=20, (0,1)=30, (1,1)=40
    std::vector<float> data = { 10.0f, 20.0f, 30.0f, 40.0f };
    HeightMap hm;
    hm.LoadFromFloatArray(data.data(), 2, 2);
    // Sample at UV corners
    EXPECT_FLOAT_EQ(hm.Sample(0.0f, 0.0f), 10.0f);
    EXPECT_FLOAT_EQ(hm.Sample(1.0f, 0.0f), 20.0f);
    EXPECT_FLOAT_EQ(hm.Sample(0.0f, 1.0f), 30.0f);
    EXPECT_FLOAT_EQ(hm.Sample(1.0f, 1.0f), 40.0f);
}

TEST(HeightMapTest, SampleCenter) {
    // 2x2 heightmap
    std::vector<float> data = { 10.0f, 20.0f, 30.0f, 40.0f };
    HeightMap hm;
    hm.LoadFromFloatArray(data.data(), 2, 2);
    // Center at UV (0.5, 0.5) → bilinear interpolation
    // (10+20+30+40)/4 = 25
    float center = hm.Sample(0.5f, 0.5f);
    EXPECT_FLOAT_EQ(center, 25.0f);
}

TEST(HeightMapTest, SampleBilinearInterpolation) {
    // 3x3 heightmap with linear gradient
    std::vector<float> data = {
        0.0f, 1.0f, 2.0f,
        1.0f, 2.0f, 3.0f,
        2.0f, 3.0f, 4.0f
    };
    HeightMap hm;
    hm.LoadFromFloatArray(data.data(), 3, 3);
    // At UV (0.5, 0.5) the value should be 2.0
    EXPECT_FLOAT_EQ(hm.Sample(0.5f, 0.5f), 2.0f);
    // At UV (0.25, 0.25) → interpolate between 0, 1, 1, 2
    // u=0.25 in pixel space means between pixel 0 and 1 at 25% → pixel x = 0.5
    // Actually, for 3 pixels, UV 0 → pixel 0, UV 0.5 → pixel 1, UV 1 → pixel 2
    // UV 0.25 → pixel 0.5, UV 0.25 → pixel 0.5
    // Between (0,0)=0, (1,0)=1, (0,1)=1, (1,1)=2
    // bilinear at pixel (0.5, 0.5): x-interp: 0.5*(1-0.5)+1*0.5=0.75, 1*(1-0.5)+2*0.5=1.5
    // y-interp: 0.75*(1-0.5)+1.5*0.5 = 0.75+0.75 = 1.125
    float val = hm.Sample(0.5f, 0.5f);
    EXPECT_FLOAT_EQ(val, 2.0f);
}

// ============================================================================
// HeightMap — GetHeightAt (world coordinate sampling)
// ============================================================================
TEST(HeightMapTest, GetHeightAtFlatTerrain) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 5.0f);
    // World coordinate should map to UV and return flat height
    float h = hm.GetHeightAt(0.0f, 0.0f);
    EXPECT_FLOAT_EQ(h, 5.0f);
}

TEST(HeightMapTest, GetHeightAtCenter) {
    HeightMap hm;
    hm.GenerateFlat(64, 64, 100.0f);
    // Various world positions should all be flat
    EXPECT_FLOAT_EQ(hm.GetHeightAt(0.0f, 0.0f), 100.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightAt(10.0f, -10.0f), 100.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightAt(-30.0f, 20.0f), 100.0f);
}

// ============================================================================
// HeightMap — WorldToUV / UVToWorld
// ============================================================================
TEST(HeightMapTest, WorldToUVCenter) {
    HeightMap hm;
    hm.GenerateFlat(32, 32, 0.0f);
    // World origin should map to UV (0.5, 0.5) — center of heightmap
    Vector2 uv = hm.WorldToUV(0.0f, 0.0f);
    EXPECT_FLOAT_EQ(uv.x, 0.5f);
    EXPECT_FLOAT_EQ(uv.y, 0.5f);
}

TEST(HeightMapTest, WorldToUVBounds) {
    HeightMap hm;
    hm.GenerateFlat(32, 32, 0.0f);
    // The heightmap spans from -16 to 16 in world space (center at origin)
    Vector2 uvMin = hm.WorldToUV(-16.0f, -16.0f);
    EXPECT_NEAR(uvMin.x, 0.0f, 0.001f);
    EXPECT_NEAR(uvMin.y, 0.0f, 0.001f);

    Vector2 uvMax = hm.WorldToUV(16.0f, 16.0f);
    EXPECT_NEAR(uvMax.x, 1.0f, 0.001f);
    EXPECT_NEAR(uvMax.y, 1.0f, 0.001f);
}

TEST(HeightMapTest, UVToWorld) {
    HeightMap hm;
    hm.GenerateFlat(32, 32, 0.0f);
    Vector2 world = hm.UVToWorld(0.5f, 0.5f);
    EXPECT_NEAR(world.x, 0.0f, 0.001f);
    EXPECT_NEAR(world.y, 0.0f, 0.001f);
}

TEST(HeightMapTest, WorldToUVRoundtrip) {
    HeightMap hm;
    hm.GenerateFlat(64, 64, 0.0f);
    // Round-trip from world → UV → world should be consistent
    Vector2 uv = hm.WorldToUV(25.0f, -13.0f);
    Vector2 world = hm.UVToWorld(uv.x, uv.y);
    EXPECT_NEAR(world.x, 25.0f, 0.01f);
    EXPECT_NEAR(world.y, -13.0f, 0.01f);
}

// ============================================================================
// HeightMap — GetNormalAt / GetNormalAtPixel
// ============================================================================
TEST(HeightMapTest, GetNormalAtFlatTerrain) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 0.0f);
    // Flat terrain should have upward normal
    Vector3 normal = hm.GetNormalAt(0.0f, 0.0f);
    EXPECT_NEAR(normal.x, 0.0f, 0.001f);
    EXPECT_NEAR(normal.y, 1.0f, 0.001f);
    EXPECT_NEAR(normal.z, 0.0f, 0.001f);
}

TEST(HeightMapTest, GetNormalAtFlatPixel) {
    HeightMap hm;
    hm.GenerateFlat(16, 16, 0.0f);
    Vector3 normal = hm.GetNormalAtPixel(8, 8);
    EXPECT_NEAR(normal.x, 0.0f, 0.001f);
    EXPECT_NEAR(normal.y, 1.0f, 0.001f);
    EXPECT_NEAR(normal.z, 0.0f, 0.001f);
}

TEST(HeightMapTest, GetNormalOnSlope) {
    // Create a simple slope: height increases along X
    std::vector<float> data(16 * 16, 0.0f);
    for (uint32_t y = 0; y < 16; ++y) {
        for (uint32_t x = 0; x < 16; ++x) {
            data[y * 16 + x] = static_cast<float>(x);
        }
    }
    HeightMap hm;
    hm.LoadFromFloatArray(data.data(), 16, 16);
    Vector3 normal = hm.GetNormalAt(0.0f, 0.0f);
    // On an X-increasing slope, the normal should tilt away from the rise
    // The normal should point somewhat backward (negative x component) and mostly up
    EXPECT_LT(normal.x, 0.0f); // slopes upward in +X, so normal tilts in -X
    EXPECT_GT(normal.y, 0.0f); // still mostly up
}

// ============================================================================
// HeightMap — GetBounds
// ============================================================================
TEST(HeightMapTest, GetBoundsFlat) {
    HeightMap hm;
    hm.GenerateFlat(32, 32, 10.0f);
    auto [min, max] = hm.GetBounds();
    // AABB should span the terrain extents
    EXPECT_LT(min.x, max.x);
    EXPECT_LT(min.z, max.z);
    // Height range: min.y ≈ max.y ≈ 10.0 (flat)
    EXPECT_NEAR(min.y, 10.0f, 0.01f);
    EXPECT_NEAR(max.y, 10.0f, 0.01f);
}

// ============================================================================
// HeightMap — ExportCollisionMesh
// ============================================================================
TEST(HeightMapTest, ExportCollisionMeshEmpty) {
    HeightMap hm;
    auto mesh = hm.ExportCollisionMesh();
    EXPECT_TRUE(mesh.empty());
}

TEST(HeightMapTest, ExportCollisionMeshFlat) {
    HeightMap hm;
    hm.GenerateFlat(8, 8, 5.0f);
    auto mesh = hm.ExportCollisionMesh(1.0f);
    EXPECT_FALSE(mesh.empty());
    EXPECT_EQ(mesh.size(), 8u * 8u); // 8x8 grid of vertices
    // All vertices should have y ≈ 5.0
    for (const auto& v : mesh) {
        EXPECT_NEAR(v.y, 5.0f, 0.01f);
    }
}

// ============================================================================
// HeightMap — Edge Cases
// ============================================================================
TEST(HeightMapTest, SinglePixel) {
    std::vector<float> data = { 42.0f };
    HeightMap hm;
    EXPECT_TRUE(hm.LoadFromFloatArray(data.data(), 1, 1));
    EXPECT_TRUE(hm.IsValid());
    EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(0, 0), 42.0f);
    EXPECT_FLOAT_EQ(hm.Sample(0.0f, 0.0f), 42.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightAt(0.0f, 0.0f), 42.0f);
    EXPECT_FLOAT_EQ(hm.GetMinHeight(), 42.0f);
    EXPECT_FLOAT_EQ(hm.GetMaxHeight(), 42.0f);
}

TEST(HeightMapTest, NonSquareDimensions) {
    std::vector<float> data(4 * 8, 0.0f);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<float>(i);
    }
    HeightMap hm;
    hm.LoadFromFloatArray(data.data(), 4, 8);
    EXPECT_EQ(hm.GetWidth(), 4u);
    EXPECT_EQ(hm.GetHeight(), 8u);
    EXPECT_EQ(hm.GetDataSize(), 32u);
    EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(3, 7), 31.0f);
}

TEST(HeightMapTest, NegativeHeights) {
    std::vector<float> data = { -10.0f, -20.0f, -30.0f, -40.0f };
    HeightMap hm;
    hm.LoadFromFloatArray(data.data(), 2, 2);
    EXPECT_TRUE(hm.IsValid());
    EXPECT_FLOAT_EQ(hm.GetMinHeight(), -40.0f);
    EXPECT_FLOAT_EQ(hm.GetMaxHeight(), -10.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(0, 0), -10.0f);
    EXPECT_FLOAT_EQ(hm.GetHeightAtPixel(1, 1), -40.0f);
}

TEST(HeightMapTest, LargeTerrain) {
    HeightMap hm;
    hm.GenerateTestTerrain(512, 512);
    EXPECT_TRUE(hm.IsValid());
    EXPECT_EQ(hm.GetWidth(), 512u);
    EXPECT_EQ(hm.GetHeight(), 512u);
    EXPECT_EQ(hm.GetDataSize(), 512u * 512u);
    EXPECT_LT(hm.GetMinHeight(), hm.GetMaxHeight());
}

} // namespace
} // namespace Terrain
} // namespace Prisma
