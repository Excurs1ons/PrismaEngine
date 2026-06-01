#include <gtest/gtest.h>
#include "navigation/NavMesh.h"
#include <glm/glm.hpp>
#include <random>
#include <type_traits>

namespace Prisma {
namespace {

using namespace Prisma::Navigation;

NavPolygon MakeRightTriangle(
    const glm::dvec3& origin,
    double width, double depth,
    int32_t n0 = -1, int32_t n1 = -1, int32_t n2 = -1)
{
    NavPolygon tri;
    tri.vertices[0] = origin;
    tri.vertices[1] = origin + glm::dvec3(width, 0, 0);
    tri.vertices[2] = origin + glm::dvec3(0, 0, depth);
    tri.neighbors[0] = n0;
    tri.neighbors[1] = n1;
    tri.neighbors[2] = n2;
    tri.id = 0;
    tri.areaType = AreaType::Walkable;
    return tri;
}

TEST(NavMeshTest, EmptyNavMesh) {
    NavMesh mesh;

    EXPECT_TRUE(mesh.IsEmpty());
    EXPECT_EQ(mesh.GetPolygonCount(), 0);

    glm::dvec3 min, max;
    mesh.GetBounds(min, max);
    EXPECT_EQ(min, glm::dvec3(0));
    EXPECT_EQ(max, glm::dvec3(0));

    EXPECT_EQ(mesh.FindPolygonContainingPoint(glm::dvec3(0)), -1);

    int32_t idx;
    glm::dvec3 cp;
    EXPECT_FALSE(mesh.FindClosestPolygon(glm::dvec3(0), idx, cp));
}

TEST(NavMeshTest, Construction) {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2));
    polys.push_back(MakeRightTriangle({2, 0, 0}, 2, 2));

    NavMesh mesh(std::move(polys));
    EXPECT_FALSE(mesh.IsEmpty());
    EXPECT_EQ(mesh.GetPolygonCount(), 2);
    EXPECT_EQ(mesh.GetPolygon(0).id, 0u);
    EXPECT_EQ(mesh.GetPolygon(1).id, 0u);

    const NavMesh& cmesh = mesh;
    EXPECT_EQ(cmesh.GetPolygonCount(), 2);
}

TEST(NavMeshTest, PointContainment) {
    NavPolygon tri = MakeRightTriangle({0, 0, 0}, 2, 2);

    // 内部
    EXPECT_TRUE(tri.ContainsPoint({0.5, 0, 0.5}));
    EXPECT_TRUE(tri.ContainsPoint({0.2, 0, 1.5}));

    // 顶点 / 边
    EXPECT_TRUE(tri.ContainsPoint({0, 0, 0}));
    EXPECT_TRUE(tri.ContainsPoint({2, 0, 0}));
    EXPECT_TRUE(tri.ContainsPoint({0, 0, 2}));
    EXPECT_TRUE(tri.ContainsPoint({1, 0, 0}));
    EXPECT_TRUE(tri.ContainsPoint({0, 0, 1}));

    // 外部
    EXPECT_FALSE(tri.ContainsPoint({3, 0, 3}));
    EXPECT_FALSE(tri.ContainsPoint({-1, 0, -1}));
    EXPECT_FALSE(tri.ContainsPoint({2, 0, 2}));
    EXPECT_FALSE(tri.ContainsPoint({10, 0, 0}));

    // 沿法线方向（barycentric 投影到三角形平面）
    EXPECT_TRUE(tri.ContainsPoint({0.5, 5, 0.5}));
}

TEST(NavMeshTest, FindPolygonContainingPoint) {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2));
    polys.push_back(MakeRightTriangle({2, 0, 2}, 2, 2));

    NavMesh mesh(std::move(polys));

    EXPECT_EQ(mesh.FindPolygonContainingPoint({0.5, 0, 0.5}), 0);
    EXPECT_EQ(mesh.FindPolygonContainingPoint({2.5, 0, 2.5}), 1);
    EXPECT_EQ(mesh.FindPolygonContainingPoint({10, 0, 10}), -1);
}

TEST(NavMeshTest, BoundaryComputation) {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2));
    polys.push_back(MakeRightTriangle({2, 0, 0}, 2, 2));
    polys.push_back(MakeRightTriangle({0, 0, 2}, 2, 2));

    NavMesh mesh(std::move(polys));

    glm::dvec3 min, max;
    mesh.GetBounds(min, max);

    EXPECT_EQ(min, glm::dvec3(0, 0, 0));
    EXPECT_EQ(max, glm::dvec3(4, 0, 4));
}

TEST(NavMeshTest, FindClosestPolygon) {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2));

    NavMesh mesh(std::move(polys));

    int32_t outIdx;
    glm::dvec3 outPoint;

    bool found = mesh.FindClosestPolygon({3, 0, 0}, outIdx, outPoint);
    EXPECT_TRUE(found);
    EXPECT_EQ(outIdx, 0);

    {
        NavMesh emptyMesh;
        found = emptyMesh.FindClosestPolygon({0, 0, 0}, outIdx, outPoint);
        EXPECT_FALSE(found);
    }
}

TEST(NavMeshTest, PolygonCenter) {
    NavPolygon tri = MakeRightTriangle({0, 0, 0}, 3, 3);
    glm::dvec3 center = tri.GetCenter();

    EXPECT_DOUBLE_EQ(center.x, 1.0);
    EXPECT_DOUBLE_EQ(center.y, 0.0);
    EXPECT_DOUBLE_EQ(center.z, 1.0);
}

TEST(NavMeshTest, PolygonNormal) {
    NavPolygon tri = MakeRightTriangle({0, 0, 0}, 2, 2);
    glm::dvec3 normal = tri.GetNormal();

    EXPECT_NEAR(normal.x, 0.0, 1e-6);
    EXPECT_NEAR(normal.y, -1.0, 1e-6);
    EXPECT_NEAR(normal.z, 0.0, 1e-6);
}

TEST(NavMeshTest, PolygonArea) {
    NavPolygon tri = MakeRightTriangle({0, 0, 0}, 2, 2);
    double area = tri.GetArea();

    EXPECT_NEAR(area, 2.0, 1e-6);
}

TEST(NavMeshTest, ClosestPointOnPolygon) {
    NavPolygon tri = MakeRightTriangle({0, 0, 0}, 2, 2);

    glm::dvec3 closest = tri.ClosestPointOnPolygon({0.5, 10, 0.5});
    EXPECT_EQ(closest, glm::dvec3(0.5, 0, 0.5));
}

TEST(NavMeshTest, EdgeMidpoint) {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2, 1, -1, -1));
    polys.push_back(MakeRightTriangle({2, 0, 0}, 2, 2, 0, -1, -1));

    NavMesh mesh(std::move(polys));

    glm::dvec3 mid;
    bool ok = mesh.GetEdgeMidpoint(0, 1, mid);
    EXPECT_TRUE(ok);

    EXPECT_EQ(mid, glm::dvec3(1, 0, 0));

    ok = mesh.GetEdgeMidpoint(0, -1, mid);
    EXPECT_FALSE(ok);
}

TEST(NavMeshTest, RandomPointSampling) {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2));

    NavMesh mesh(std::move(polys));
    std::mt19937 rng(42);

    glm::dvec3 pt = mesh.GetRandomPointInPolygon(0, rng);
    EXPECT_TRUE(mesh.GetPolygon(0).ContainsPoint(pt));

    pt = mesh.GetRandomPointInPolygon(5, rng);
    EXPECT_EQ(pt, glm::dvec3(0));

    pt = mesh.GetRandomPoint(rng);
    EXPECT_TRUE(mesh.GetPolygon(0).ContainsPoint(pt));

    NavMesh empty;
    pt = empty.GetRandomPoint(rng);
    EXPECT_EQ(pt, glm::dvec3(0));
}

TEST(NavMeshTest, SetPolygonsAndClear) {
    NavMesh mesh;
    EXPECT_TRUE(mesh.IsEmpty());

    std::vector<NavPolygon> polys;
    polys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2));
    mesh.SetPolygons(std::move(polys));
    EXPECT_FALSE(mesh.IsEmpty());
    EXPECT_EQ(mesh.GetPolygonCount(), 1);

    mesh.Clear();
    EXPECT_TRUE(mesh.IsEmpty());
    EXPECT_EQ(mesh.GetPolygonCount(), 0);
}

TEST(NavMeshTest, BinaryRoundTrip) {
    // 验证 NavPolygon 的二进序列化/反序列化往返
    // 使用内存缓冲区模拟二进制 I/O

    // 确认 NavPolygon 是可平凡复制的
    EXPECT_TRUE(std::is_trivially_copyable_v<NavPolygon>);

    // 准备原始多边形数据
    NavPolygon original;
    original.vertices[0] = glm::dvec3(1.0, 2.0, 3.0);
    original.vertices[1] = glm::dvec3(4.0, 5.0, 6.0);
    original.vertices[2] = glm::dvec3(7.0, 8.0, 9.0);
    original.neighbors[0] = 10;
    original.neighbors[1] = -1;
    original.neighbors[2] = 42;
    original.areaType = AreaType::Water;
    original.id = 12345;

    // 序列化到内存缓冲区
    std::vector<uint8_t> buffer(sizeof(NavPolygon));
    std::memcpy(buffer.data(), &original, sizeof(NavPolygon));

    // 反序列化回新对象
    NavPolygon restored;
    std::memcpy(&restored, buffer.data(), sizeof(NavPolygon));

    // 验证所有字段一致
    EXPECT_DOUBLE_EQ(restored.vertices[0].x, 1.0);
    EXPECT_DOUBLE_EQ(restored.vertices[0].y, 2.0);
    EXPECT_DOUBLE_EQ(restored.vertices[0].z, 3.0);
    EXPECT_DOUBLE_EQ(restored.vertices[1].x, 4.0);
    EXPECT_DOUBLE_EQ(restored.vertices[1].y, 5.0);
    EXPECT_DOUBLE_EQ(restored.vertices[1].z, 6.0);
    EXPECT_DOUBLE_EQ(restored.vertices[2].x, 7.0);
    EXPECT_DOUBLE_EQ(restored.vertices[2].y, 8.0);
    EXPECT_DOUBLE_EQ(restored.vertices[2].z, 9.0);

    EXPECT_EQ(restored.neighbors[0], 10);
    EXPECT_EQ(restored.neighbors[1], -1);
    EXPECT_EQ(restored.neighbors[2], 42);

    EXPECT_EQ(restored.areaType, AreaType::Water);
    EXPECT_EQ(restored.id, 12345u);
}

TEST(NavMeshTest, BinaryRoundTripMultiplePolygons) {
    // 验证多个多边形的二进制序列化往返

    std::vector<NavPolygon> originalPolys;
    originalPolys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2));
    originalPolys.push_back(MakeRightTriangle({3, 0, 0}, 2, 2));
    originalPolys.push_back(MakeRightTriangle({0, 0, 3}, 2, 2));

    // 设置不同的 ID 和 areaType
    originalPolys[0].id = 10;
    originalPolys[0].areaType = AreaType::Walkable;
    originalPolys[1].id = 20;
    originalPolys[1].areaType = AreaType::Lava;
    originalPolys[2].id = 30;
    originalPolys[2].areaType = AreaType::Door;

    // 序列化
    std::vector<uint8_t> buffer(originalPolys.size() * sizeof(NavPolygon));
    std::memcpy(buffer.data(), originalPolys.data(), buffer.size());

    // 反序列化
    std::vector<NavPolygon> restoredPolys(originalPolys.size());
    std::memcpy(restoredPolys.data(), buffer.data(), buffer.size());

    // 验证
    ASSERT_EQ(restoredPolys.size(), 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(restoredPolys[i].id, originalPolys[i].id);
        EXPECT_EQ(restoredPolys[i].areaType, originalPolys[i].areaType);
        EXPECT_DOUBLE_EQ(restoredPolys[i].GetCenter().x, originalPolys[i].GetCenter().x);
        EXPECT_DOUBLE_EQ(restoredPolys[i].GetCenter().y, originalPolys[i].GetCenter().y);
        EXPECT_DOUBLE_EQ(restoredPolys[i].GetCenter().z, originalPolys[i].GetCenter().z);
    }
}

TEST(NavMeshTest, NavMeshBinaryRoundTrip) {
    // 验证完整的 NavMesh 二进制序列化/反序列化往返

    std::vector<NavPolygon> polys;
    polys.push_back(MakeRightTriangle({0, 0, 0}, 2, 2));
    polys.push_back(MakeRightTriangle({2, 0, 0}, 2, 2, 0, -1, -1));

    NavMesh original(std::move(polys));

    // 序列化多边形数量 + 所有多边形
    uint32_t polyCount = static_cast<uint32_t>(original.GetPolygonCount());
    std::vector<uint8_t> buffer(
        sizeof(uint32_t) + polyCount * sizeof(NavPolygon)
    );

    size_t offset = 0;
    std::memcpy(buffer.data() + offset, &polyCount, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    for (uint32_t i = 0; i < polyCount; ++i) {
        NavPolygon p = original.GetPolygon(i);
        std::memcpy(buffer.data() + offset, &p, sizeof(NavPolygon));
        offset += sizeof(NavPolygon);
    }

    // 反序列化
    offset = 0;
    uint32_t restoredCount;
    std::memcpy(&restoredCount, buffer.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::vector<NavPolygon> restored(restoredCount);
    for (uint32_t i = 0; i < restoredCount; ++i) {
        std::memcpy(&restored[i], buffer.data() + offset, sizeof(NavPolygon));
        offset += sizeof(NavPolygon);
    }

    NavMesh restoredMesh(std::move(restored));

    // 验证
    EXPECT_EQ(restoredMesh.GetPolygonCount(), 2);
    EXPECT_EQ(restoredMesh.GetPolygon(0).id, original.GetPolygon(0).id);
    EXPECT_EQ(restoredMesh.GetPolygon(1).neighbors[0], 0);
    EXPECT_EQ(restoredMesh.GetPolygon(1).neighbors[1], -1);

    glm::dvec3 min, max;
    restoredMesh.GetBounds(min, max);
    EXPECT_EQ(min, glm::dvec3(0, 0, 0));
    EXPECT_EQ(max, glm::dvec3(4, 0, 2));
}

TEST(NavMeshTest, BinaryRoundTripEdgeCases) {
    // 验证边角情况：空网格和单多边形

    // 1. 空网格
    NavMesh emptyMesh;
    uint32_t emptyCount = 0;
    std::vector<uint8_t> emptyBuf(sizeof(uint32_t));
    std::memcpy(emptyBuf.data(), &emptyCount, sizeof(uint32_t));

    uint32_t restoredEmptyCount;
    std::memcpy(&restoredEmptyCount, emptyBuf.data(), sizeof(uint32_t));
    EXPECT_EQ(restoredEmptyCount, 0u);

    // 2. 单多边形
    NavPolygon single;
    single.vertices[0] = glm::dvec3(-1.5, 0.0, -2.5);
    single.vertices[1] = glm::dvec3(3.5, 0.0, 0.0);
    single.vertices[2] = glm::dvec3(0.0, 0.0, 4.5);
    single.neighbors[0] = -1;
    single.neighbors[1] = -1;
    single.neighbors[2] = -1;
    single.areaType = AreaType::Obstacle;
    single.id = 999;

    std::vector<uint8_t> singleBuf(sizeof(NavPolygon));
    std::memcpy(singleBuf.data(), &single, sizeof(NavPolygon));

    NavPolygon restoredSingle;
    std::memcpy(&restoredSingle, singleBuf.data(), sizeof(NavPolygon));

    EXPECT_DOUBLE_EQ(restoredSingle.vertices[0].x, -1.5);
    EXPECT_DOUBLE_EQ(restoredSingle.vertices[0].z, -2.5);
    EXPECT_EQ(restoredSingle.areaType, AreaType::Obstacle);
    EXPECT_EQ(restoredSingle.id, 999u);
}

} // namespace
} // namespace Prisma
