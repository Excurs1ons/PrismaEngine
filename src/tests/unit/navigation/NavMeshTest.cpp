#include <gtest/gtest.h>
#include "navigation/NavMesh.h"
#include <glm/glm.hpp>
#include <random>

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

} // namespace
} // namespace Prisma
