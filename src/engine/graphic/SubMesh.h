//
// Created by JasonGu on 26-1-1.
//
#pragma once

#ifndef SUBMESH_H
#define SUBMESH_H
#include "Handle.h"
#include <string>
#include <vector>
#include "interfaces/RenderTypes.h"
namespace PrismaEngine {
using namespace Graphic;
struct SubMesh {
    std::string name;
    uint32_t materialIndex; // 对应材质数组的索引
    std::vector<Vertex> vertices; // 子网格的顶点数据
    std::vector<uint32_t> indices; // 子网格的顶点索引数据
    uint32_t verticesCount() const { return static_cast<uint32_t>(vertices.size()); }
    uint32_t indicesCount() const { return static_cast<uint32_t>(indices.size()); }
    // 图形API资源句柄
    VertexBufferHandle vertexBufferHandle;
    IndexBufferHandle indexBufferHandle;
};
}
#endif //SUBMESH_H
