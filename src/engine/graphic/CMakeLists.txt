# 前向渲染管线源文件
set(PIPELINE_FORWARD_SOURCES
    graphic/pipelines/forward/ForwardPipeline.cpp
    graphic/pipelines/forward/DepthPrePass.cpp
    graphic/pipelines/forward/OpaquePass.cpp
    graphic/pipelines/forward/TransparentPass.cpp
)

# 延迟渲染管线源文件
set(PIPELINE_DEFERRED_SOURCES
    graphic/pipelines/deferred/DeferredPipeline.cpp
    graphic/pipelines/deferred/GeometryPass.cpp
    graphic/pipelines/deferred/LightingPass.cpp
    graphic/pipelines/deferred/CompositionPass.cpp
    graphic/pipelines/deferred/GBuffer.cpp
)

# UI 模块源文件
set(UI_SOURCES
    graphic/ui/FontAtlas.cpp
    graphic/ui/TextRendererComponent.cpp
    graphic/ui/UIPass.cpp
)

# UI 模块头文件
set(UI_HEADERS
    graphic/ui/FontAtlas.h
    graphic/ui/TextRendererComponent.h
    graphic/ui/UIPass.h
)

# 新增逻辑 Pass/Pipeline 源文件
set(LOGICAL_SOURCES
    graphic/LogicalPass.cpp
    graphic/LogicalPipeline.cpp
)

# 新增逻辑 Pass/Pipeline 头文件
set(LOGICAL_HEADERS
    graphic/LogicalPass.h
    graphic/LogicalPipeline.h
)

# 前向渲染管线头文件
set(PIPELINE_FORWARD_HEADERS
    graphic/pipelines/forward/ForwardPipeline.h
    graphic/pipelines/forward/DepthPrePass.h
    graphic/pipelines/forward/OpaquePass.h
    graphic/pipelines/forward/TransparentPass.h
)

# 延迟渲染管线头文件
set(PIPELINE_DEFERRED_HEADERS
    graphic/pipelines/deferred/DeferredPipeline.h
    graphic/pipelines/deferred/GeometryPass.h
    graphic/pipelines/deferred/LightingPass.h
    graphic/pipelines/deferred/CompositionPass.h
    graphic/pipelines/deferred/GBuffer.h
)

# Vulkan 后端文件 (条件编译)
set(VULKAN_SOURCES
    graphic/RenderBackendVulkan.cpp
)

set(VULKAN_HEADERS
    graphic/RenderBackendVulkan.h
)

set(ENGINE_GRAPHIC_SOURCES
    ${PIPELINE_FORWARD_SOURCES}
    ${PIPELINE_DEFERRED_SOURCES}
    ${UI_SOURCES}
    ${LOGICAL_SOURCES}
    graphic/pipelines/SkyboxRenderPass.cpp
    graphic/GeometryRenderPass.cpp
    graphic/Material.cpp
    graphic/Mesh.cpp
    graphic/MeshRenderer.cpp
    graphic/RenderComponent.cpp
    graphic/RenderPass.cpp
    graphic/RenderSystemNew.cpp
    graphic/ScriptableRenderPipeline.cpp
    graphic/Shader.cpp
    # New interface files
    graphic/interfaces/RenderTypes.cpp
    graphic/interfaces/IPipelineState.cpp
    # DX12 adapter files (conditionally included)
    # graphic/adapters/dx12/ files will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_DX12
    # Vulkan backend files (conditionally included)
    # VULKAN_SOURCES will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_VULKAN
)

set(ENGINE_GRAPHIC_HEADERS
    ${PIPELINE_FORWARD_HEADERS}
    ${PIPELINE_DEFERRED_HEADERS}
    ${UI_HEADERS}
    ${LOGICAL_HEADERS}
    graphic/pipelines/SkyboxRenderPass.h
    graphic/GeometryRenderPass.h
    graphic/Material.h
    graphic/Mesh.h
    graphic/MeshRenderer.h
    graphic/RenderBackend.h
    graphic/RenderCommandContext.h
    graphic/RenderComponent.h
    graphic/RenderGraph.h
    graphic/RenderPass.h
#    graphic/RenderSystem.h
    graphic/RenderSystemNew.h
    graphic/ScriptableRenderPipeline.h
    graphic/ICamera.h
    graphic/Shader.h
    # New interface headers
    graphic/interfaces/RenderTypes.h
    graphic/interfaces/Interfaces.h
    graphic/interfaces/IResource.h
    graphic/interfaces/IRenderDevice.h
    graphic/interfaces/ICommandBuffer.h
    graphic/interfaces/IResourceManager.h
    graphic/interfaces/IResourceFactory.h
    graphic/interfaces/ITexture.h
    graphic/interfaces/IBuffer.h
    graphic/interfaces/IShader.h
    graphic/interfaces/IPipelineState.h
    graphic/interfaces/IPipeline.h
    graphic/interfaces/ISampler.h
    graphic/interfaces/ISwapChain.h
    graphic/interfaces/IFence.h
    # New logic layer interfaces
    graphic/interfaces/IDeviceContext.h
    graphic/interfaces/IRenderTarget.h
    graphic/interfaces/IPass.h
    graphic/interfaces/IGBuffer.h
    # DX12 adapter headers (conditionally included)
    # graphic/adapters/dx12/ files will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_DX12
    # Vulkan backend headers (conditionally included)
    # VULKAN_HEADERS will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_VULKAN
)