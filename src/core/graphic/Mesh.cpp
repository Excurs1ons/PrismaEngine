#include "Mesh.h"

Mesh Mesh::GetCubeMesh()
{
    Mesh cubeMesh;
    
    // 创建立方体的8个顶点
    cubeMesh.subMeshes.resize(1);
    SubMesh& subMesh = cubeMesh.subMeshes[0];
    subMesh.name = "Cube";
    subMesh.materialIndex = 0;
    
    // 立方体的8个顶点
    subMesh.vertices = {
        // 前面 (Z+)
        { XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(0, 0, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(1, 0, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(1, 1, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(0, 1, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        
        // 后面 (Z-)
        { XMFLOAT4(-0.5f, -0.5f, -0.5f, 1.0f), XMFLOAT4(0, 0, -1, 0), XMFLOAT4(0, 0, 0, 0), XMFLOAT4(-1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(0.5f, -0.5f, -0.5f, 1.0f), XMFLOAT4(0, 0, -1, 0), XMFLOAT4(1, 0, 0, 0), XMFLOAT4(-1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(0.5f, 0.5f, -0.5f, 1.0f), XMFLOAT4(0, 0, -1, 0), XMFLOAT4(1, 1, 0, 0), XMFLOAT4(-1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(-0.5f, 0.5f, -0.5f, 1.0f), XMFLOAT4(0, 0, -1, 0), XMFLOAT4(0, 1, 0, 0), XMFLOAT4(-1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} }
    };
    
    // 立方体的12个三角形（36个索引）
    subMesh.indices = {
        // 前面
        0, 1, 2, 0, 2, 3,
        // 后面
        4, 6, 5, 4, 7, 6,
        // 左面
        4, 0, 3, 4, 3, 7,
        // 右面
        1, 5, 6, 1, 6, 2,
        // 顶面
        3, 2, 6, 3, 6, 7,
        // 底面
        4, 5, 1, 4, 1, 0
    };
    
    // 计算包围盒
    XMVECTOR minVec = XMVectorSet(-0.5f, -0.5f, -0.5f, 0.0f);
    XMVECTOR maxVec = XMVectorSet(0.5f, 0.5f, 0.5f, 0.0f);
    BoundingBox::CreateFromPoints(cubeMesh.globalBoundingBox, minVec, maxVec);
    
    return cubeMesh;
}

Mesh Mesh::GetTriangleMesh()
{
    Mesh triangleMesh;
    
    triangleMesh.subMeshes.resize(1);
    SubMesh& subMesh = triangleMesh.subMeshes[0];
    subMesh.name = "Triangle";
    subMesh.materialIndex = 0;
    
    // 三角形的3个顶点
    subMesh.vertices = {
        { XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(0.5f, 0.0f, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 0.0f, 0.0f, 1.0f} },
        { XMFLOAT4(-0.5f, -0.5f, 0.0f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(0.0f, 1.0f, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{0.0f, 1.0f, 0.0f, 1.0f} },
        { XMFLOAT4(0.5f, -0.5f, 0.0f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(1.0f, 1.0f, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{0.0f, 0.0f, 1.0f, 1.0f} }
    };
    
    // 三角形的1个三角形（3个索引）
    subMesh.indices = { 0, 1, 2 };
    
    // 计算包围盒
    XMVECTOR minVec = XMVectorSet(-0.5f, -0.5f, 0.0f, 0.0f);
    XMVECTOR maxVec = XMVectorSet(0.5f, 0.5f, 0.0f, 0.0f);
    BoundingBox::CreateFromPoints(triangleMesh.globalBoundingBox, minVec, maxVec);
    
    return triangleMesh;
}

Mesh Mesh::GetQuadMesh()
{
    Mesh quadMesh;
    
    quadMesh.subMeshes.resize(1);
    SubMesh& subMesh = quadMesh.subMeshes[0];
    subMesh.name = "Quad";
    subMesh.materialIndex = 0;
    
    // 四边形的4个顶点
    subMesh.vertices = {
        { XMFLOAT4(-0.5f, 0.5f, 0.0f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(0.0f, 0.0f, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(0.5f, 0.5f, 0.0f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(1.0f, 0.0f, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(0.5f, -0.5f, 0.0f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(1.0f, 1.0f, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} },
        { XMFLOAT4(-0.5f, -0.5f, 0.0f, 1.0f), XMFLOAT4(0, 0, 1, 0), XMFLOAT4(0.0f, 1.0f, 0, 0), XMFLOAT4(1, 0, 0, 0), XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f} }
    };
    
    // 四边形的2个三角形（6个索引）
    subMesh.indices = { 0, 1, 2, 0, 2, 3 };
    
    // 计算包围盒
    XMVECTOR minVec = XMVectorSet(-0.5f, -0.5f, 0.0f, 0.0f);
    XMVECTOR maxVec = XMVectorSet(0.5f, 0.5f, 0.0f, 0.0f);
    BoundingBox::CreateFromPoints(quadMesh.globalBoundingBox, minVec, maxVec);
    
    return quadMesh;
}

bool Mesh::Load(const std::filesystem::path& path)
{
    // 这里应该实现从文件加载网格的逻辑
    // 例如从OBJ、FBX等格式加载
    // 为了简单起见，这里只是设置路径和名称
    m_path = path;
    m_name = path.filename().string();
    m_isLoaded = !subMeshes.empty();
    return m_isLoaded;
}

void Mesh::Unload()
{
    subMeshes.clear();
    globalBoundingBox = BoundingBox();
    m_isLoaded = false;
}

bool Mesh::IsLoaded() const
{
    return m_isLoaded;
}
