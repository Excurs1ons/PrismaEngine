#include <gtest/gtest.h>
#include "graphic/Mesh.h"

namespace Prisma::Graphic {
namespace {

TEST(MeshTest, DefaultConstruction) {
    Mesh mesh;
    EXPECT_FALSE(mesh.IsLoaded());
    EXPECT_TRUE(mesh.GetSubMeshes().empty());
    EXPECT_TRUE(mesh.GetCPUSubMeshes().empty());
    EXPECT_FALSE(mesh.HasCPUMeshData());
    EXPECT_EQ(mesh.GetType(), AssetType::Mesh);
}

TEST(MeshTest, AddSubMesh) {
    Mesh mesh;
    SubMeshBuffer sub;
    sub.name = "test_sub";
    sub.materialIndex = 0;
    sub.baseVertex = 0;
    sub.baseIndex = 0;
    sub.indexCount = 6;
    sub.vertexCount = 4;
    sub.use16BitIndices = true;

    mesh.AddSubMesh(sub);
    ASSERT_EQ(mesh.GetSubMeshes().size(), 1u);
    EXPECT_EQ(mesh.GetSubMeshes()[0].name, "test_sub");
    EXPECT_EQ(mesh.GetSubMeshes()[0].indexCount, 6u);
    EXPECT_EQ(mesh.GetSubMeshes()[0].vertexCount, 4u);
    EXPECT_TRUE(mesh.GetSubMeshes()[0].use16BitIndices);
}

TEST(MeshTest, MultipleSubMeshes) {
    Mesh mesh;
    for (uint32_t i = 0; i < 3; ++i) {
        SubMeshBuffer sub;
        sub.name = "sub_" + std::to_string(i);
        sub.materialIndex = i;
        sub.baseVertex = i * 100;
        sub.baseIndex = i * 200;
        sub.indexCount = 36 + i * 6;
        sub.vertexCount = 24 + i * 4;
        mesh.AddSubMesh(sub);
    }

    ASSERT_EQ(mesh.GetSubMeshes().size(), 3u);
    EXPECT_EQ(mesh.GetSubMeshes()[0].name, "sub_0");
    EXPECT_EQ(mesh.GetSubMeshes()[1].name, "sub_1");
    EXPECT_EQ(mesh.GetSubMeshes()[2].name, "sub_2");
    EXPECT_EQ(mesh.GetSubMeshes()[0].materialIndex, 0u);
    EXPECT_EQ(mesh.GetSubMeshes()[1].materialIndex, 1u);
    EXPECT_EQ(mesh.GetSubMeshes()[2].materialIndex, 2u);
}

TEST(MeshTest, BoundingBox) {
    Mesh mesh;
    BoundingBox bb(Vector3(-5.0f, -3.0f, -2.0f), Vector3(5.0f, 7.0f, 10.0f));
    mesh.SetBoundingBox(bb);

    BoundingBox result = mesh.GetBoundingBox();
    EXPECT_EQ(result.minBounds, Vector3(-5.0f, -3.0f, -2.0f));
    EXPECT_EQ(result.maxBounds, Vector3(5.0f, 7.0f, 10.0f));
}

TEST(MeshTest, CPUMeshDataAddAndRetrieve) {
    Mesh mesh;
    Mesh::CPUMeshData data;
    data.positions = { Vector3(0,0,0), Vector3(1,0,0), Vector3(0,1,0) };
    data.normals   = { Vector3(0,0,1), Vector3(0,0,1), Vector3(0,0,1) };
    data.uvs       = { Vector2(0,0), Vector2(1,0), Vector2(0,1) };
    data.indices   = { 0, 1, 2 };

    mesh.AddCPUMeshData(data);
    ASSERT_TRUE(mesh.HasCPUMeshData());
    ASSERT_EQ(mesh.GetCPUSubMeshes().size(), 1u);

    const auto& retrieved = mesh.GetCPUMeshData(0);
    ASSERT_EQ(retrieved.positions.size(), 3u);
    EXPECT_EQ(retrieved.positions[0], Vector3(0, 0, 0));
    EXPECT_EQ(retrieved.positions[1], Vector3(1, 0, 0));
    EXPECT_EQ(retrieved.indices.size(), 3u);
    EXPECT_EQ(retrieved.indices[0], 0u);
    EXPECT_EQ(retrieved.indices[2], 2u);
}

TEST(MeshTest, CPUMeshDataMultipleSubMeshes) {
    Mesh mesh;
    Mesh::CPUMeshData data0, data1;
    data0.positions = { Vector3(0,0,0), Vector3(1,0,0) };
    data0.indices   = { 0, 1 };
    data1.positions = { Vector3(2,0,0), Vector3(3,0,0), Vector3(4,0,0) };
    data1.indices   = { 0, 1, 2 };

    mesh.AddCPUMeshData(data0);
    mesh.AddCPUMeshData(data1);

    ASSERT_EQ(mesh.GetCPUSubMeshes().size(), 2u);
    EXPECT_EQ(mesh.GetCPUMeshData(0).positions.size(), 2u);
    EXPECT_EQ(mesh.GetCPUMeshData(1).positions.size(), 3u);
}

TEST(MeshTest, ClearCPUMeshData) {
    Mesh mesh;
    Mesh::CPUMeshData data;
    data.positions = { Vector3(0,0,0) };
    mesh.AddCPUMeshData(data);
    ASSERT_TRUE(mesh.HasCPUMeshData());

    mesh.ClearCPUMeshData();
    EXPECT_FALSE(mesh.HasCPUMeshData());
    EXPECT_TRUE(mesh.GetCPUSubMeshes().empty());
}

TEST(MeshTest, MutableCPUMeshData) {
    Mesh mesh;
    Mesh::CPUMeshData data;
    data.positions = { Vector3(1,2,3) };
    mesh.AddCPUMeshData(data);

    auto& mutableData = mesh.GetMutableCPUMeshData();
    ASSERT_EQ(mutableData.size(), 1u);
    mutableData[0].positions[0] = Vector3(9, 9, 9);

    EXPECT_EQ(mesh.GetCPUMeshData(0).positions[0], Vector3(9, 9, 9));
}

TEST(MeshTest, UnloadClearsState) {
    Mesh mesh;

    SubMeshBuffer sub;
    sub.name = "sub";
    mesh.AddSubMesh(sub);

    BoundingBox bb(Vector3(0,0,0), Vector3(1,1,1));
    mesh.SetBoundingBox(bb);

    Mesh::CPUMeshData data;
    data.positions = { Vector3(0,0,0) };
    mesh.AddCPUMeshData(data);

    mesh.Unload();
    EXPECT_FALSE(mesh.IsLoaded());
    EXPECT_TRUE(mesh.GetSubMeshes().empty());
    EXPECT_TRUE(mesh.GetCPUSubMeshes().empty());
    EXPECT_EQ(mesh.GetBoundingBox().minBounds, Vector3(0,0,0));
    EXPECT_EQ(mesh.GetBoundingBox().maxBounds, Vector3(0,0,0));
}

TEST(MeshTest, LoadWithoutFileReturnsFalse) {
    Mesh mesh;
    bool result = mesh.Load("non_existent_file.gltf");
    EXPECT_FALSE(result);
    EXPECT_FALSE(mesh.IsLoaded());
}

} // namespace
} // namespace Prisma::Graphic
