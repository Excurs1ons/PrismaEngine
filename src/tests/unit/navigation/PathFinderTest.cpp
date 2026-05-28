#include <gtest/gtest.h>
#include "navigation/NavMesh.h"
#include "navigation/PathFinder.h"
#include <glm/glm.hpp>

namespace Prisma {
namespace {

using namespace Prisma::Navigation;

NavPolygon MakeTri(
    const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c,
    int32_t n0, int32_t n1, int32_t n2, uint32_t id)
{
    NavPolygon poly;
    poly.vertices[0] = a;
    poly.vertices[1] = b;
    poly.vertices[2] = c;
    poly.neighbors[0] = n0;
    poly.neighbors[1] = n1;
    poly.neighbors[2] = n2;
    poly.id = id;
    return poly;
}

// 3 个三角形排成一线：T0 → T1 → T2
//   T0(0..2,0..2)  T1(3..5,0..2)  T2(6..8,0..2)
NavMesh MakeLineOf3() {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeTri({0, 0, 0}, {2, 0, 0}, {0, 0, 2},  1, -1, -1, 0));
    polys.push_back(MakeTri({3, 0, 0}, {5, 0, 0}, {3, 0, 2},  0,  2, -1, 1));
    polys.push_back(MakeTri({6, 0, 0}, {8, 0, 0}, {6, 0, 2},  1, -1, -1, 2));
    return NavMesh(std::move(polys));
}

NavMesh MakeDisconnected() {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeTri({0, 0, 0}, {2, 0, 0}, {0, 0, 2}, -1, -1, -1, 0));
    polys.push_back(MakeTri({10, 0, 0}, {12, 0, 0}, {10, 0, 2}, -1, -1, -1, 1));
    return NavMesh(std::move(polys));
}

TEST(PathFinderTest, StraightLinePath) {
    NavMesh mesh = MakeLineOf3();
    PathFinder finder;

    PathResult result = finder.FindPath({0.5, 0, 0.5}, {6.5, 0, 0.5}, mesh);

    EXPECT_TRUE(result.found);
    EXPECT_TRUE(result.IsValid());
    EXPECT_GT(result.cost, 0.0);

    // 必须经过 T0 → T1 → T2
    ASSERT_GE(result.polygonPath.size(), 3);
    EXPECT_EQ(result.polygonPath[0], 0);
    EXPECT_EQ(result.polygonPath[1], 1);
    EXPECT_EQ(result.polygonPath[2], 2);

    EXPECT_EQ(result.waypoints.front(), glm::dvec3(0.5, 0, 0.5));
    EXPECT_EQ(result.waypoints.back(),  glm::dvec3(6.5, 0, 0.5));
}

TEST(PathFinderTest, StartEqualsTarget) {
    NavMesh mesh = MakeLineOf3();
    PathFinder finder;

    PathResult result = finder.FindPath({0.5, 0, 0.5}, {0.5, 0, 0.5}, mesh);

    EXPECT_TRUE(result.found);
    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.polygonPath.size(), 1);
    EXPECT_EQ(result.polygonPath[0], 0);

    ASSERT_EQ(result.waypoints.size(), 2);
    EXPECT_EQ(result.waypoints[0], result.waypoints[1]);
    EXPECT_EQ(result.cost, 0.0);
}

TEST(PathFinderTest, UnreachableTarget) {
    NavMesh mesh = MakeDisconnected();
    PathFinder finder;

    PathResult result = finder.FindPath({0.5, 0, 0.5}, {10.5, 0, 0.5}, mesh);

    EXPECT_FALSE(result.found);
    EXPECT_FALSE(result.IsValid());
    EXPECT_TRUE(result.waypoints.empty());
    EXPECT_TRUE(result.polygonPath.empty());
    EXPECT_EQ(result.cost, 0.0);
}

TEST(PathFinderTest, EmptyNavMesh) {
    NavMesh empty;
    PathFinder finder;

    PathResult result = finder.FindPath({0, 0, 0}, {1, 0, 0}, empty);

    EXPECT_FALSE(result.found);
    EXPECT_FALSE(result.IsValid());
}

TEST(PathFinderTest, PathAroundObstacle) {
    // 图中 T0 ↔ T1 的直接连接被阻断，只能走 T0→T2→T1→T3
    //   T0 → T2 → T1 → T3
    std::vector<NavPolygon> polys;
    polys.push_back(MakeTri({0, 0, 0}, {2, 0, 0}, {0, 0, 2},  2, -1, -1, 0));
    polys.push_back(MakeTri({3, 0, 0}, {5, 0, 0}, {3, 0, 2},  2,  3, -1, 1));
    polys.push_back(MakeTri({0, 0, 3}, {2, 0, 3}, {0, 0, 5},  0,  1, -1, 2));
    polys.push_back(MakeTri({6, 0, 0}, {8, 0, 0}, {6, 0, 2},  1, -1, -1, 3));

    NavMesh mesh(std::move(polys));
    PathFinder finder;

    PathResult result = finder.FindPath({0.5, 0, 0.5}, {6.5, 0, 0.5}, mesh);

    EXPECT_TRUE(result.found);
    ASSERT_EQ(result.polygonPath.size(), 4);
    EXPECT_EQ(result.polygonPath[0], 0);
    EXPECT_EQ(result.polygonPath[1], 2);
    EXPECT_EQ(result.polygonPath[2], 1);
    EXPECT_EQ(result.polygonPath[3], 3);

    EXPECT_EQ(result.waypoints.front(), glm::dvec3(0.5, 0, 0.5));
    EXPECT_EQ(result.waypoints.back(),  glm::dvec3(6.5, 0, 0.5));
}

TEST(PathFinderTest, ShortestPathSelection) {
    // 分支图：T0→T1→T3（短）和 T0→T2→T1→T3（长）
    //       T0 ──→ T1 ──→ T3
    //        ↘      ↗
    //         T2
    std::vector<NavPolygon> polys;
    polys.push_back(MakeTri({0, 0, 0}, {2, 0, 0}, {0, 0, 2},  1,  2, -1, 0));
    polys.push_back(MakeTri({3, 0, 0}, {5, 0, 0}, {3, 0, 2},  0,  3, -1, 1));
    polys.push_back(MakeTri({0, 0, 3}, {2, 0, 3}, {0, 0, 5},  1, -1, -1, 2));
    polys.push_back(MakeTri({6, 0, 0}, {8, 0, 0}, {6, 0, 2},  1, -1, -1, 3));

    NavMesh mesh(std::move(polys));
    PathFinder finder;

    PathResult result = finder.FindPath({0.5, 0, 0.5}, {6.5, 0, 0.5}, mesh);

    EXPECT_TRUE(result.found);
    ASSERT_EQ(result.polygonPath.size(), 3);
    EXPECT_EQ(result.polygonPath[0], 0);
    EXPECT_EQ(result.polygonPath[1], 1);
    EXPECT_EQ(result.polygonPath[2], 3);
}

TEST(PathFinderTest, StartNotInAnyPolygon) {
    NavMesh mesh = MakeLineOf3();
    PathFinder finder;

    PathResult result = finder.FindPath({100, 0, 0}, {6.5, 0, 0.5}, mesh);
    EXPECT_TRUE(result.found);
    EXPECT_TRUE(result.IsValid());
}

TEST(PathFinderTest, SinglePolygonPath) {
    std::vector<NavPolygon> polys;
    polys.push_back(MakeTri({0, 0, 0}, {2, 0, 0}, {0, 0, 2}, -1, -1, -1, 0));

    NavMesh mesh(std::move(polys));
    PathFinder finder;

    PathResult result = finder.FindPath({0.2, 0, 0.2}, {0.8, 0, 0.8}, mesh);

    EXPECT_TRUE(result.found);
    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.polygonPath.size(), 1);
    EXPECT_EQ(result.polygonPath[0], 0);
    ASSERT_EQ(result.waypoints.size(), 2);
    EXPECT_EQ(result.waypoints[0], glm::dvec3(0.2, 0, 0.2));
    EXPECT_EQ(result.waypoints[1], glm::dvec3(0.8, 0, 0.8));
}

} // namespace
} // namespace Prisma
