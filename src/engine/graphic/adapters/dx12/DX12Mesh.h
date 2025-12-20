#pragma once

#include "interfaces/IMesh.h"
#include "DX12ResourceFactory.h"
#include <vector>
#include <string>
#include <memory>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;
class DX12ResourceFactory;

/// @brief DirectX12网格适配器
/// 实现IMesh接口，管理DX12特定的网格资源
class DX12Mesh : public IMesh {
public:
    /// @brief 构造函数
    /// @param device DX12渲染设备
    /// @param factory DX12资源工厂
    DX12Mesh(DX12RenderDevice* device, DX12ResourceFactory* factory);

    /// @brief 析构函数
    ~DX12Mesh() override;

    // IMesh接口实现
    uint32_t GetSubMeshCount() const override;
    const SubMesh* GetSubMesh(uint32_t index) const override;
    uint32_t AddSubMesh(const SubMesh& subMesh) override;
    const BoundingBox& GetBoundingBox() const override;
    void UpdateBoundingBox() override;
    void Bind(class ICommandBuffer* commandBuffer, uint32_t subMeshIndex = 0) override;
    void Draw(class ICommandBuffer* commandBuffer, uint32_t subMeshIndex = 0) override;
    void DrawInstanced(class ICommandBuffer* commandBuffer, uint32_t instanceCount, uint32_t subMeshIndex = 0) override;
    const std::string& GetName() const override;
    void SetName(const std::string& name) override;
    void SetKeepCpuData(bool keep) override;
    bool IsUploaded() const override;
    bool UploadToGPU(class IRenderDevice* device) override;
    void UnloadFromGPU() override;

    // DX12特定方法
    /// @brief 从顶点数据创建网格
    /// @param vertices 顶点数据
    /// @param indices 索引数据
    /// @return 是否成功
    bool CreateFromData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    /// @brief 创建立方体网格
    /// @param size 立方体大小
    void CreateCube(float size = 1.0f);

    /// @brief 创建球体网格
    /// @param radius 半径
    /// @param segments 分段数
    void CreateSphere(float radius = 1.0f, uint32_t segments = 32);

    /// @brief 创建平面网格
    /// @param width 宽度
    /// @param height 高度
    /// @param widthSegments 宽度分段数
    /// @param heightSegments 高度分段数
    void CreatePlane(float width = 1.0f, float height = 1.0f,
                     uint32_t widthSegments = 1, uint32_t heightSegments = 1);


private:
    DX12RenderDevice* m_device;
    DX12ResourceFactory* m_factory;
    std::vector<SubMesh> m_subMeshes;
    BoundingBox m_boundingBox;
    std::string m_name;
    bool m_keepCpuData;
    bool m_isUploaded;

    /// @brief 计算子网格包围盒
    /// @param vertices 顶点数据
    /// @param indices 索引数据
    /// @return 包围盒
    BoundingBox CalculateSubMeshBounds(const std::vector<Vertex>& vertices,
                                      const std::vector<uint32_t>& indices) const;

    /// @brief 创建顶点缓冲区
    /// @param vertices 顶点数据
    /// @param subMesh 子网格
    /// @return 是否成功
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices, SubMesh& subMesh);

    /// @brief 创建索引缓冲区
    /// @param indices 索引数据
    /// @param subMesh 子网格
    /// @return 是否成功
    bool CreateIndexBuffer(const std::vector<uint32_t>& indices, SubMesh& subMesh);

    /// @brief 更新全局包围盒
    void UpdateGlobalBoundingBox();
};

} // namespace PrismaEngine::Graphic::DX12