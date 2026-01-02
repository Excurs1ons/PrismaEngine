//
// Created by JasonGu on 26-1-2.
//
#include "RenderDeviceVulkan.h"
namespace PrismaEngine {
    namespace Graphic {
        namespace Vulkan {
            RenderDeviceVulkan::RenderDeviceVulkan() {}

            RenderDeviceVulkan::~RenderDeviceVulkan() {}

            bool RenderDeviceVulkan::Initialize(const DeviceDesc &desc) {
                return false;
            }

            void RenderDeviceVulkan::Shutdown() {}

            std::string RenderDeviceVulkan::GetName() const {
                return "";
            }

            std::string RenderDeviceVulkan::GetAPIName() const {
                return "";
            }

            std::unique_ptr <ICommandBuffer>
            RenderDeviceVulkan::CreateCommandBuffer(CommandBufferType type) {
                return nullptr;
            }

            void
            RenderDeviceVulkan::SubmitCommandBuffer(ICommandBuffer *cmdBuffer, IFence *fence) {}

            void RenderDeviceVulkan::SubmitCommandBuffers(
                    const std::vector<ICommandBuffer *> &cmdBuffers,
                    const std::vector<IFence *> &fences) {}

            void RenderDeviceVulkan::WaitForIdle() {}

            std::unique_ptr <PrismaEngine::Graphic::IFence> RenderDeviceVulkan::CreateFence() {
                return nullptr;
            }

            void RenderDeviceVulkan::WaitForFence(IFence *fence) {}

            PrismaEngine::Graphic::IResourceFactory *
            RenderDeviceVulkan::GetResourceFactory() const {
                return nullptr;
            }

            std::unique_ptr <PrismaEngine::Graphic::ISwapChain> RenderDeviceVulkan::CreateSwapChain(
                    void *windowHandle, uint32_t width, uint32_t height,
                    bool vsync) { return nullptr; }

            PrismaEngine::Graphic::ISwapChain *
            RenderDeviceVulkan::GetSwapChain() const { return nullptr; }

            void RenderDeviceVulkan::BeginFrame() {}

            void RenderDeviceVulkan::EndFrame() {}

            void RenderDeviceVulkan::Present() {}

            bool RenderDeviceVulkan::SupportsMultiThreaded() const { return nullptr; }

            bool RenderDeviceVulkan::SupportsBindlessTextures() const { return nullptr; }

            bool RenderDeviceVulkan::SupportsComputeShader() const { return nullptr; }

            bool RenderDeviceVulkan::SupportsRayTracing() const { return nullptr; }

            bool RenderDeviceVulkan::SupportsMeshShader() const { return nullptr; }

            bool RenderDeviceVulkan::SupportsVariableRateShading() const { return nullptr; }

            PrismaEngine::Graphic::IRenderDevice::GPUMemoryInfo
            RenderDeviceVulkan::GetGPUMemoryInfo() const { return nullptr; }

            PrismaEngine::Graphic::IRenderDevice::RenderStats
            RenderDeviceVulkan::GetRenderStats() const { return nullptr; }

            void RenderDeviceVulkan::BeginDebugMarker(const std::string &name) {}

            void RenderDeviceVulkan::EndDebugMarker() {}

            void RenderDeviceVulkan::SetDebugMarker(const std::string &name) {}

            VulkanQueue *
            RenderDeviceVulkan::GetGraphicsQueue() const { return nullptr; }

            VulkanQueue *RenderDeviceVulkan::GetComputeQueue() const {
                return nullptr;
            }

            VulkanQueue *
            RenderDeviceVulkan::GetTransferQueue() const { return nullptr; }

            bool RenderDeviceVulkan::InitializeInstance() { return nullptr; }

            bool RenderDeviceVulkan::SelectPhysicalDevice() { return nullptr; }

            bool RenderDeviceVulkan::CreateLogicalDevice() { return nullptr; }

            bool RenderDeviceVulkan::CreateCommandPools() { return nullptr; }

            bool RenderDeviceVulkan::InitializeDebug() { return nullptr; }

            bool RenderDeviceVulkan::CreateSynchronizationObjects() { return nullptr; }

            void RenderDeviceVulkan::ReleaseAll() {}

            bool RenderDeviceVulkan::CheckValidationLayerSupport() { return nullptr; }

            std::vector<const char *>
            RenderDeviceVulkan::GetRequiredExtensions() { return nullptr; }

            bool RenderDeviceVulkan::IsDeviceExtensionSupported(
                    const std::string &extension) { return nullptr; }
        }
    }
}