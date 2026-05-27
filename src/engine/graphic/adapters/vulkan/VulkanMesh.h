#pragma once

#include "interfaces/IMesh.h"
#include <vector>
#include <string>
#include <memory>
#include <vulkan/vulkan.h>

namespace Prisma::Graphic::Vulkan {

// 前置声明
class VulkanRenderDevice;
class VulkanResourceFactory;

// Vulkan网格适配器
/// 实现IMesh接口，管理Vulkan特定的网格资源
class VulkanMesh : public IMesh {
public:
    // 构造函数
    VulkanMesh(VulkanRenderDevice* device, VulkanResourceFactory* factory);

    // 析构函数
    ~VulkanMesh() override;

    // IMesh接口实现
    uint32_t GetSubMeshCount() const override;
    const SubMeshBuffer* GetSubMesh(uint32_t index) const override;
    uint32_t AddSubMesh(const SubMeshBuffer& subMesh) override;
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

    // Vulkan特定方法
    // 从顶点数据创建网格
    bool CreateFromData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    // 创建立方体网格
    void CreateCube(float size = 1.0f);

    // 创建球体网格
    void CreateSphere(float radius = 1.0f, uint32_t segments = 32);

    // 创建平面网格
    void CreatePlane(float width = 1.0f, float height = 1.0f,
                     uint32_t widthSegments = 1, uint32_t heightSegments = 1);

    // 获取顶点缓冲区
    VkBuffer GetVertexBuffer(uint32_t subMeshIndex = 0) const;

    // 获取索引缓冲区
    VkBuffer GetIndexBuffer(uint32_t subMeshIndex = 0) const;


private:
    VulkanRenderDevice* m_device;
    VulkanResourceFactory* m_factory;
    std::vector<SubMeshBuffer> m_subMeshes;
    BoundingBox m_boundingBox;
    std::string m_name;
    bool m_keepCpuData;
    bool m_isUploaded;

    // 计算子网格包围盒
    BoundingBox CalculateSubMeshBounds(const std::vector<Vertex>& vertices,
                                      const std::vector<uint32_t>& indices) const;

    // 创建顶点缓冲区
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices, SubMeshBuffer& subMesh);

    // 创建索引缓冲区
    bool CreateIndexBuffer(const std::vector<uint32_t>& indices, SubMeshBuffer& subMesh);

    // 创建缓冲区
    bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer& buffer,
                     VkDeviceMemory& bufferMemory);

    // 复制缓冲区
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    // 查找内存类型
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // 更新全局包围盒
    void UpdateGlobalBoundingBox();

    // 清理资源
    void Cleanup();
};

} // namespace Prisma::Graphic::Vulkan