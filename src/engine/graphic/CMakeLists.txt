set(PIPELINE_FORWARD_SOURCES
    graphic/pipelines/forward/ForwardPipeline.cpp
    graphic/pipelines/forward/ForwardRenderPass.cpp
    graphic/pipelines/forward/DepthPrePass.cpp
    graphic/pipelines/forward/OpaquePass.cpp
    graphic/pipelines/forward/TransparentPass.cpp
)

set(PIPELINE_FORWARD_HEADERS
    graphic/pipelines/forward/ForwardPipeline.h
    graphic/pipelines/forward/ForwardRenderPass.h
    graphic/pipelines/forward/DepthPrePass.h
    graphic/pipelines/forward/OpaquePass.h
    graphic/pipelines/forward/TransparentPass.h
)

set(ENGINE_GRAPHIC_SOURCES
    ${PIPELINE_FORWARD_SOURCES}
    graphic/pipelines/SkyboxRenderPass.cpp
    graphic/GeometryRenderPass.cpp
    graphic/Material.cpp
    graphic/Mesh.cpp
    graphic/MeshRenderer.cpp
    graphic/RenderBackendVulkan.cpp
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
    # Test files
    graphic/RenderSystemTest.cpp
)

set(ENGINE_GRAPHIC_HEADERS
    ${PIPELINE_FORWARD_HEADERS}
    graphic/pipelines/SkyboxRenderPass.h
    graphic/GeometryRenderPass.h
    graphic/Material.h
    graphic/Mesh.h
    graphic/MeshRenderer.h
    graphic/RenderBackend.h
    graphic/RenderBackendVulkan.h
    graphic/RenderCommandContext.h
    graphic/RenderComponent.h
    graphic/RenderGraph.h
    graphic/RenderPass.h
#    graphic/RenderSystem.h
    graphic/RenderSystemNew.h
    graphic/ScriptableRenderPipeline.h
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
    # DX12 adapter headers (conditionally included)
    # graphic/adapters/dx12/ files will be added in parent CMakeLists.txt based on PRISMA_ENABLE_RENDER_DX12
)