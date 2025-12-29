#include "RenderAPIVulkan.h"
#include "../platform/ApplicationWindows.h"
#include "../LogScope.h"
#include "../Logger.h"
#include "RenderErrorHandling.h"
#include "../SceneManager.h"
#include "../math/MathTypes.h"
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include "../math/Math.h"
namespace PrismaEngine::Graphic {

// Vulkan渲染命令上下文的改进实现
    class VulkanRenderCommandContext : public PrismaEngine::Graphic::RenderCommandContext {
    public:
        VulkanRenderCommandContext(RenderBackendVulkan *backend) : m_backend(backend) {
            // 验证后端有效性
            if (m_backend == nullptr) {
                throw std::runtime_error("VulkanRenderCommandContext: Backend is null");
            }
        }

        void SetConstantBuffer(const std::string &name, Matrix4x4 matrix) {
            RenderCommandContext::SetConstantBuffer(name, matrix);
            if (name.empty()) {
                LOG_WARNING("VulkanRenderCommand", "SetConstantBuffer called with null name");
                return;
            }
            //TODO:
            // 在实际实现中，这里应该更新Vulkan uniform buffer

            LOG_DEBUG("VulkanRenderCommand", "Set constant buffer '{0}' with matrix data", name);
        }

        void SetConstantBuffer(const std::string &name, const float *data, size_t size) override {
            if (name.empty() || (data == nullptr)) {
                LOG_WARNING("VulkanRenderCommand", "SetConstantBuffer called with null parameters");
                return;
            }

            if (size == 0) {
                LOG_WARNING("VulkanRenderCommand", "SetConstantBuffer called with zero size");
                return;
            }

            // 复制数据到常量缓冲区映射
            m_constantBuffers[name] = std::vector<float>(data, data + (size / sizeof(float)));

            LOG_DEBUG("VulkanRenderCommand", "Set constant buffer '{0}' with {1} bytes", name,
                      size);
        }

        void SetVertexBuffer(const void *data, uint32_t sizeInBytes, uint32_t strideInBytes) {
            if ((data == nullptr) || sizeInBytes == 0 || strideInBytes == 0) {
                LOG_WARNING("VulkanRenderCommand", "Invalid vertex buffer parameters");
                return;
            }

            m_vertexBufferData.resize(sizeInBytes);
            std::memcpy(m_vertexBufferData.data(), data, sizeInBytes);
            m_vertexStride = strideInBytes;

            LOG_DEBUG("VulkanRenderCommand", "Set vertex buffer: {0} bytes, stride {1}",
                      sizeInBytes, strideInBytes);
        }

        void SetIndexBuffer(const void *data, uint32_t sizeInBytes, bool use16BitIndices = true) {
            if ((data == nullptr) || sizeInBytes == 0) {
                LOG_WARNING("VulkanRenderCommand", "Invalid index buffer parameters");
                return;
            }

            m_indexBufferData.resize(sizeInBytes);
            std::memcpy(m_indexBufferData.data(), data, sizeInBytes);
            m_use16BitIndices = use16BitIndices;

            LOG_DEBUG("VulkanRenderCommand", "Set index buffer: {0} bytes, 16-bit: {1}",
                      sizeInBytes, use16BitIndices);
        }

        void SetShaderResource(const std::string &name, void *resource) {
            if (name.empty()) {
                LOG_WARNING("VulkanRenderCommand", "SetShaderResource called with null name");
                return;
            }

            m_shaderResources[name] = resource;

            LOG_DEBUG("VulkanRenderCommand", "Set shader resource '{0}': 0x{1:x}",
                      name, reinterpret_cast<uintptr_t>(resource));
        }

        void SetSampler(const std::string &name, void *sampler) {
            if (name.empty()) {
                LOG_WARNING("VulkanRenderCommand", "SetSampler called with null name");
                return;
            }

            m_samplers[name] = sampler;

            LOG_DEBUG("VulkanRenderCommand", "Set sampler '{0}': 0x{1:x}",
                      name, reinterpret_cast<uintptr_t>(sampler));
        }

        void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0,
                         uint32_t baseVertexLocation = 0) {
            if (indexCount == 0) {
                LOG_WARNING("VulkanRenderCommand", "DrawIndexed called with zero index count");
                return;
            }

            if (m_indexBufferData.empty()) {
                LOG_ERROR("VulkanRenderCommand", "DrawIndexed called without index buffer");
                return;
            }

            // 在实际实现中，这里应该记录Vulkan绘制命令
            // 目前只记录调试信息
            LOG_DEBUG("VulkanRenderCommand", "DrawIndexed: {0} indices, start {1}, base vertex {2}",
                      indexCount, startIndexLocation, baseVertexLocation);
        }

        void Draw(uint32_t vertexCount, uint32_t startVertexLocation = 0) override {
            if (vertexCount == 0) {
                LOG_WARNING("VulkanRenderCommand", "Draw called with zero vertex count");
                return;
            }

            if (m_vertexBufferData.empty()) {
                LOG_ERROR("VulkanRenderCommand", "Draw called without vertex buffer");
                return;
            }

            // 在实际实现中，这里应该记录Vulkan绘制命令
            LOG_DEBUG("VulkanRenderCommand", "Draw: {0} vertices, start {1}", vertexCount,
                      startVertexLocation);
        }

        void SetViewport(float x, float y, float width, float height) override {
            if (width <= 0 || height <= 0) {
                LOG_WARNING("VulkanRenderCommand", "Invalid viewport dimensions: {0}x{1}", width,
                            height);
                return;
            }

            // 存储视口参数
            m_viewport.x = x;
            m_viewport.y = y;
            m_viewport.width = width;
            m_viewport.height = height;

            LOG_DEBUG("VulkanRenderCommand", "Set viewport: ({0},{1}) {2}x{3}", x, y, width,
                      height);
        }

        void SetScissorRect(int left, int top, int right, int bottom) {
            // 验证矩形有效性
            if (right <= left || bottom <= top) {
                LOG_WARNING("VulkanRenderCommand", "Invalid scissor rect: ({0},{1}) to ({2},{3})",
                            left, top, right, bottom);
                return;
            }

            m_scissorRect.left = left;
            m_scissorRect.top = top;
            m_scissorRect.right = right;
            m_scissorRect.bottom = bottom;

            LOG_DEBUG("VulkanRenderCommand", "Set scissor rect: ({0},{1}) to ({2},{3})",
                      left, top, right, bottom);
        }

    private:
        RenderBackendVulkan *m_backend;

        // 存储渲染状态数据
        std::unordered_map<std::string, std::vector<float>> m_constantBuffers;
        std::unordered_map<std::string, void *> m_shaderResources;
        std::unordered_map<std::string, void *> m_samplers;

        std::vector<uint8_t> m_vertexBufferData;
        std::vector<uint8_t> m_indexBufferData;
        uint32_t m_vertexStride = 0;
        bool m_use16BitIndices = true;

        struct Viewport {
            float x, y, width, height;
        } m_viewport = {0.0f, 0.0f, 0.0f, 0.0f};

        struct ScissorRect {
            int left, top, right, bottom;
        } m_scissorRect = {0, 0, 0, 0};
    };
}  // namespace Engine
