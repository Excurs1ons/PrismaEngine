#pragma once

#include "graphic/RenderCommandContext.h"
#include "graphic/interfaces/IPipeline.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/ICamera.h"
#include "graphic/pipelines/deferred/DeferredPipeline.h"
#include <memory>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace Prisma {

class Scene;

class DeferredPipelineAdapter : public Graphic::IPipeline {
public:
    DeferredPipelineAdapter();
    ~DeferredPipelineAdapter() override;

    int Initialize(Graphic::IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const Graphic::RenderContext& ctx) override;
    void OnSceneLoaded(Scene* scene) override;

    Graphic::DeferredPipeline* GetDeferredPipeline() const { return m_pipeline.get(); }
    void SetDebugGBufferConfig(int target, bool show);

private:
    class CameraDataAdapter : public Graphic::ICamera {
    public:
        void SetCameraData(const Graphic::CameraData& data, float width, float height);
        PrismaMath::mat4 GetViewMatrix() const override;
        PrismaMath::mat4 GetProjectionMatrix() const override;
        PrismaMath::mat4 GetViewProjectionMatrix() const override;
        PrismaMath::vec3 GetPosition() const override;
        PrismaMath::vec3 GetForward() const override;
        PrismaMath::vec3 GetUp() const override;
        PrismaMath::vec3 GetRight() const override;
        float GetFOV() const override;
        float GetNearPlane() const override;
        float GetFarPlane() const override;
        float GetAspectRatio() const override;
        void SetFOV(float fov) override;
        void SetNearFarPlanes(float nearPlane, float farPlane) override;
        void SetAspectRatio(float aspectRatio) override;
        void SetViewport(uint32_t width, uint32_t height) override;
        void Update(Graphic::Timestep ts) override;
        bool IsActive() const override;
        void SetActive(bool active) override;
        PrismaMath::vec4 GetClearColor() const override;
        void SetClearColor(float r, float g, float b, float a = 1.0f) override;
    private:
        Graphic::CameraData m_data;
        float m_width = 1280, m_height = 720;
        PrismaMath::vec4 m_clearColor = {0,0,0,1};
    };

    bool CreateResources(uint32_t width, uint32_t height);
    void DestroyResources();
    void CreatePSOs();

    Graphic::IRenderDevice* m_device = nullptr;
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    std::shared_ptr<Graphic::DeferredPipeline> m_pipeline;
    CameraDataAdapter m_cameraAdapter;

    uint32_t m_width = 0, m_height = 0;

    std::shared_ptr<Graphic::ITexture> m_gbPosition;
    std::shared_ptr<Graphic::ITexture> m_gbNormal;
    std::shared_ptr<Graphic::ITexture> m_gbAlbedo;
    std::shared_ptr<Graphic::ITexture> m_gbEmissive;
    std::shared_ptr<Graphic::ITexture> m_gbDepth;
    std::shared_ptr<Graphic::ITexture> m_lightingOutput;

    VkRenderPass m_gbufferRP = VK_NULL_HANDLE;
    VkFramebuffer m_gbufferFB = VK_NULL_HANDLE;
    VkRenderPass m_lightingRP = VK_NULL_HANDLE;
    VkFramebuffer m_lightingFB = VK_NULL_HANDLE;

    std::shared_ptr<Graphic::IPipelineState> m_gbufferPSO;
    std::shared_ptr<Graphic::IPipelineState> m_lightingPSO;
    std::shared_ptr<Graphic::IPipelineState> m_compositePSO;
    std::shared_ptr<Graphic::IPipelineState> m_forwardPSO;
    std::shared_ptr<Graphic::IPipelineState> m_debugGBufferPSO;

    std::shared_ptr<Graphic::IDescriptorSet> m_lightingDS;
    std::shared_ptr<Graphic::IDescriptorSet> m_compositeDS;
    std::shared_ptr<Graphic::IDescriptorSet> m_debugGBufferDS[5]; // one per target, created once
    std::shared_ptr<Graphic::ISampler> m_defaultSampler;

    int m_debugGBufferTarget = 0; // 0=position, 1=normal, 2=albedo, 3=emissive, 4=depth
    bool m_debugGBufferShow = false;

    Graphic::RenderCommandContext m_deviceContext;
    Graphic::SceneData m_sceneData;
    bool m_firstFrameLogged = false;
};

} // namespace Prisma
