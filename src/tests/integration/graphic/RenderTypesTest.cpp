#include <gtest/gtest.h>
#include "graphic/interfaces/RenderTypes.h"

namespace Prisma::Graphic {
namespace {

// ── BoundingBox ──

TEST(RenderTypesTest, BoundingBoxDefaultConstructor) {
    BoundingBox bb;
    EXPECT_EQ(bb.minBounds, Vector3(0, 0, 0));
    EXPECT_EQ(bb.maxBounds, Vector3(0, 0, 0));
}

TEST(RenderTypesTest, BoundingBoxParameterizedConstructor) {
    BoundingBox bb(Vector3(-1, -2, -3), Vector3(4, 5, 6));
    EXPECT_EQ(bb.minBounds, Vector3(-1, -2, -3));
    EXPECT_EQ(bb.maxBounds, Vector3(4, 5, 6));
}

TEST(RenderTypesTest, BoundingBoxGetCenter) {
    BoundingBox bb(Vector3(-2, 0, -2), Vector3(2, 4, 2));
    Vector3 center = bb.GetCenter();
    EXPECT_FLOAT_EQ(center.x, 0.0f);
    EXPECT_FLOAT_EQ(center.y, 2.0f);
    EXPECT_FLOAT_EQ(center.z, 0.0f);
}

TEST(RenderTypesTest, BoundingBoxGetSize) {
    BoundingBox bb(Vector3(0, 0, 0), Vector3(3, 4, 5));
    Vector3 size = bb.GetSize();
    EXPECT_FLOAT_EQ(size.x, 3.0f);
    EXPECT_FLOAT_EQ(size.y, 4.0f);
    EXPECT_FLOAT_EQ(size.z, 5.0f);
}

TEST(RenderTypesTest, BoundingBoxEncapsulate) {
    BoundingBox bb(Vector3(0, 0, 0), Vector3(1, 1, 1));
    bb.Encapsulate(Vector3(-2, 3, 5));
    EXPECT_EQ(bb.minBounds, Vector3(-2, 0, 0));
    EXPECT_EQ(bb.maxBounds, Vector3(1, 3, 5));
}

TEST(RenderTypesTest, BoundingBoxEncapsulateInsideNoChange) {
    BoundingBox bb(Vector3(0, 0, 0), Vector3(5, 5, 5));
    bb.Encapsulate(Vector3(2, 2, 2));
    EXPECT_EQ(bb.minBounds, Vector3(0, 0, 0));
    EXPECT_EQ(bb.maxBounds, Vector3(5, 5, 5));
}

TEST(RenderTypesTest, BoundingBoxMerge) {
    BoundingBox a(Vector3(0, 0, 0), Vector3(3, 3, 3));
    BoundingBox b(Vector3(-1, 2, -2), Vector3(5, 6, 7));
    a.Merge(b);
    EXPECT_EQ(a.minBounds, Vector3(-1, 0, -2));
    EXPECT_EQ(a.maxBounds, Vector3(5, 6, 7));
}

TEST(RenderTypesTest, BoundingBoxMergeEmpty) {
    BoundingBox a(Vector3(1, 1, 1), Vector3(4, 4, 4));
    BoundingBox empty;
    a.Merge(empty);
    EXPECT_EQ(a.minBounds, Vector3(0, 0, 0));
    EXPECT_EQ(a.maxBounds, Vector3(4, 4, 4));
}

// ── Vertex ──

TEST(RenderTypesTest, VertexDefaultConstructor) {
    Vertex v;
    EXPECT_EQ(v.position, Vector4(0, 0, 0, 0));
    EXPECT_EQ(v.color, Vector4(1, 1, 1, 1));
    EXPECT_EQ(v.uv, Vector4(0, 0, 0, 0));
    EXPECT_EQ(v.normal, Vector4(0, 0, 0, 0));
}

TEST(RenderTypesTest, VertexParameterizedConstructor) {
    Vertex v(Vector4(1, 2, 3, 1), Vector4(0, 1, 0, 1), Vector4(0.5f, 0.5f, 0, 0));
    EXPECT_EQ(v.position, Vector4(1, 2, 3, 1));
    EXPECT_EQ(v.color, Vector4(0, 1, 0, 1));
    EXPECT_EQ(v.uv, Vector4(0.5f, 0.5f, 0, 0));
}

TEST(RenderTypesTest, VertexWithNormalConstructor) {
    Vertex v(Vector4(0, 1, 0, 1), Vector4(1, 0, 0, 1),
             Vector4(0, 0, 0, 0), Vector4(0, 0, 1, 0));
    EXPECT_EQ(v.position, Vector4(0, 1, 0, 1));
    EXPECT_EQ(v.normal, Vector4(0, 0, 1, 0));
}

TEST(RenderTypesTest, VertexFullConstructor) {
    Vertex v(Vector4(1, 0, 0, 1), Vector4(0, 1, 0, 1),
             Vector4(0, 0, 0, 0), Vector4(0, 0, 1, 0),
             Vector4(0.5f, 0.5f, 0, 0), Vector4(1, 0, 0, 0));
    EXPECT_EQ(v.tangent, Vector4(1, 0, 0, 0));
}

TEST(RenderTypesTest, VertexStride) {
    EXPECT_EQ(Vertex::GetVertexStride(), sizeof(Vertex));
    EXPECT_GT(sizeof(Vertex), 0u);
}

// ── Light ──

TEST(RenderTypesTest, LightStructure) {
    Light light;
    light.position = Vector4(10, 20, 30, 0);
    light.color = Vector4(1, 0.5f, 0.2f, 50.0f);
    light.direction = Vector4(0, -1, 0, 1);

    EXPECT_EQ(light.position, Vector4(10, 20, 30, 0));
    EXPECT_EQ(light.color, Vector4(1, 0.5f, 0.2f, 50.0f));
    EXPECT_EQ(light.direction, Vector4(0, -1, 0, 1));
}

TEST(RenderTypesTest, LightSize) {
    EXPECT_EQ(sizeof(Light), sizeof(Vector4) * 3);
}

// ── SubMeshBuffer ──

TEST(RenderTypesTest, SubMeshBufferAssignment) {
    SubMeshBuffer sub;
    sub.name = "test";
    sub.materialIndex = 2;
    sub.baseVertex = 10;
    sub.baseIndex = 20;
    sub.vertexCount = 100;
    sub.indexCount = 300;
    sub.use16BitIndices = true;
    EXPECT_EQ(sub.name, "test");
    EXPECT_EQ(sub.materialIndex, 2u);
    EXPECT_EQ(sub.baseVertex, 10u);
    EXPECT_EQ(sub.baseIndex, 20u);
    EXPECT_EQ(sub.vertexCount, 100u);
    EXPECT_EQ(sub.indexCount, 300u);
    EXPECT_TRUE(sub.use16BitIndices);
}

TEST(RenderTypesTest, SubMeshBufferFields) {
    SubMeshBuffer sub;
    sub.name = "test_mesh";
    sub.materialIndex = 1;
    sub.baseVertex = 0;
    sub.baseIndex = 0;
    sub.indexCount = 36;
    sub.vertexCount = 24;
    sub.use16BitIndices = true;

    EXPECT_EQ(sub.name, "test_mesh");
    EXPECT_EQ(sub.materialIndex, 1u);
    EXPECT_EQ(sub.indexCount, 36u);
    EXPECT_EQ(sub.vertexCount, 24u);
    EXPECT_TRUE(sub.use16BitIndices);
}

// ── 关键枚举 ──

TEST(RenderTypesTest, BufferTypeEnum) {
    EXPECT_NE(BufferType::Vertex, BufferType::Index);
    EXPECT_NE(BufferType::Constant, BufferType::Structured);
}

TEST(RenderTypesTest, ResourceTypeEnumHasTexture) {
    EXPECT_EQ(static_cast<int>(ResourceType::Texture), 1);
}

TEST(RenderTypesTest, TextureFormatRGBASize) {
    EXPECT_NE(TextureFormat::RGBA8_UNorm, TextureFormat::RGBA32_Float);
}

// ── BufferUsage 位操作 ──

TEST(RenderTypesTest, BufferUsageOperatorOr) {
    BufferUsage combined = BufferUsage::Dynamic | BufferUsage::Upload;
    EXPECT_TRUE(HasFlag(combined, BufferUsage::Dynamic));
    EXPECT_TRUE(HasFlag(combined, BufferUsage::Upload));
    EXPECT_FALSE(HasFlag(combined, BufferUsage::Immutable));
}

TEST(RenderTypesTest, BufferUsageDefault) {
    EXPECT_EQ(static_cast<int>(BufferUsage::Default), 0);
}

// ── Color ──

TEST(RenderTypesTest, ColorDefault) {
    Color c;
    EXPECT_FLOAT_EQ(c.r, 0.0f);
    EXPECT_FLOAT_EQ(c.g, 0.0f);
    EXPECT_FLOAT_EQ(c.b, 0.0f);
    EXPECT_FLOAT_EQ(c.a, 1.0f);
}

TEST(RenderTypesTest, ColorParametrized) {
    Color c(0.2f, 0.4f, 0.6f, 0.8f);
    EXPECT_FLOAT_EQ(c.r, 0.2f);
    EXPECT_FLOAT_EQ(c.g, 0.4f);
    EXPECT_FLOAT_EQ(c.b, 0.6f);
    EXPECT_FLOAT_EQ(c.a, 0.8f);
}

} // namespace
} // namespace Prisma::Graphic
