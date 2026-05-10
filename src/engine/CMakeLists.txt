# 引擎核心库 - Prune 分支版本
# 仅支持 Windows x64 + Vulkan + SDL3

# 平台检测 (Windows only for build, Linux for LSP)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    add_compile_options(/DNOMINMAX /DWIN32_LEAN_AND_MEAN)
endif()

# 寻找 Vulkan
if(PRISMA_ENABLE_RENDER_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

# ========== 收集核心源文件 (所有平台共用) ==========
set(CORE_SOURCES
    # App
    app/CommandLineParser.cpp
    app/Engine.cpp
    app/EngineLauncher.cpp
    app/Application.cpp
    app/ECSApplication.cpp

    # Threading
    threading/JobSystem.cpp
    threading/ThreadManager.cpp
    threading/WorkerThread.cpp

    # Logger
    logger/Logger.cpp
    logger/LogScope.cpp

    # Object
    object/Object.cpp

    # Platform
    platform/Platform.cpp
    platform/DynamicLoader.cpp

    # Window
    window/Window.cpp

    # Scene
    scene/Scene.cpp
    scene/SceneManager.cpp
    scene/GameObject.cpp

    # Serialization
    serialization/Serializable.cpp
    serialization/ScriptComponent.cpp

    # Transform
    transform/Transform.cpp
    transform/Camera.cpp
    transform/CameraController.cpp

    # Audio
    audio/SoundSynthesizer.cpp
    audio/AudioDeviceNull.cpp
    audio/AudioDeviceSDL3.cpp
    audio/AudioAPI.cpp

    # Input
    input/InputDevice.cpp
    input/InputManager.cpp

    # Physics
    physics/PhysicsSystem.cpp

    # Resource
    resource/AssetSerializer.cpp
    resource/ArchiveJson.cpp
    resource/MeshAsset.cpp
    resource/ResourceFallback.cpp
    resource/ResourceFallbackImpl.cpp
    resource/TextureAsset.cpp

    # Tilemap
    tilemap/TilemapAsset.cpp
    tilemap/format/TileDataDecoder.cpp
    tilemap/format/TmxParser.cpp
    tilemap/format/TsxParser.cpp
    tilemap/renderer/TilemapRenderer.cpp

    # Scripting
    scripting/MonoRuntime.cpp
    scripting/ScriptSystem.cpp

    # Core
    core/AssetManager.cpp
    core/AssetDatabase.cpp
    core/ECS.cpp
    core/Serialization.cpp
    core/Layer.cpp
    core/LayerStack.cpp
    core/ComponentRegistry.cpp
    core/UUID.cpp

    # UI
    ui/UIComponent.cpp
    ui/UIInputManager.cpp
    ui/2d/ButtonComponent.cpp
    ui/2d/CanvasComponent.cpp

    # Math
    math/MurmurHash3.cpp

    # Graphic
    graphic/Renderer.cpp
    graphic/Renderer2D.cpp
    graphic/CameraComponent.cpp
    graphic/RenderSystem.cpp
    graphic/RenderCommandContext.cpp
    graphic/RenderComponent.cpp
    graphic/RenderGraph.cpp
    graphic/RenderPass.cpp
    graphic/Shader.cpp
    graphic/ShaderFactory.cpp
    graphic/TextureAtlas.cpp
    graphic/VoxelRenderer.cpp
    graphic/GeometryRenderPass.cpp
    graphic/Material.cpp
    graphic/Mesh.cpp
    graphic/MeshRenderer.cpp
    graphic/OrthographicCamera.cpp
    graphic/SpriteRenderer.cpp
    graphic/SpriteAnimation.cpp
    graphic/stb_impl.cpp
    graphic/RenderResourceManager.cpp
    graphic/LogicalPass.cpp
    graphic/LogicalPipeline.cpp
    graphic/interfaces/IPipelineState.cpp

    # Graphic - Forward Pipeline
    graphic/pipelines/forward/ForwardPipeline.cpp
    graphic/pipelines/forward/DepthPrePass.cpp
    graphic/pipelines/forward/OpaquePass.cpp
    graphic/pipelines/forward/TransparentPass.cpp

    # Graphic - Deferred Pipeline
    graphic/pipelines/deferred/DeferredPipeline.cpp
    graphic/pipelines/deferred/GeometryPass.cpp
    graphic/pipelines/deferred/LightingPass.cpp
    graphic/pipelines/deferred/CompositionPass.cpp
    graphic/pipelines/deferred/GBuffer.cpp

    # Graphic - Skybox
    graphic/pipelines/SkyboxRenderPass.cpp

    # Graphic - UI
    graphic/ui/FontAtlas.cpp
    graphic/ui/TextRendererComponent.cpp
    graphic/ui/UIPass.cpp

    # Graphic - 2D
    graphic/2d/Light2D.cpp

    # Graphic - Vulkan Backend
    graphic/adapters/vulkan/VulkanAdapters.cpp
    graphic/adapters/vulkan/RenderDeviceVulkan.cpp
    graphic/adapters/vulkan/VulkanSwapChain.cpp
    graphic/adapters/vulkan/VulkanShader.cpp
    graphic/adapters/vulkan/VulkanResources.cpp
    graphic/adapters/vulkan/VulkanResourceFactory.cpp
    graphic/adapters/vulkan/VulkanFence.cpp
    graphic/adapters/vulkan/VulkanSampler.cpp
    graphic/adapters/vulkan/VulkanPipelineState.cpp
    graphic/adapters/vulkan/VulkanCommandBuffer.cpp

    # Precompiled header
    pch.cpp
)


# ========== 收集核心头文件 ==========
set(CORE_HEADERS
    # App
    app/CommandLineParser.h
    app/Engine.h
    app/EngineLauncher.h
    app/Application.h
    app/ECSApplication.h

    # Threading
    threading/JobSystem.h
    threading/ThreadManager.h
    threading/WorkerThread.h

    # Logger
    logger/Logger.h
    logger/LogScope.h
    logger/LogEntry.h
    logger/IPlatformLogger.h

    # Object
    object/Object.h
    object/Model.h
    object/MetaData.h

    # Platform
    platform/Platform.h
    platform/DynamicLoader.h

    # Window
    window/Window.h

    # Scene
    scene/Scene.h
    scene/SceneManager.h
    scene/SceneNode.h
    scene/GameObject.h

    # Serialization
    serialization/Serializable.h
    serialization/SerializationVersion.h
    serialization/ScriptComponent.h

    # Transform
    transform/Transform.h
    transform/ITransform.h
    transform/Camera.h
    transform/CameraController.h
    transform/FPSCounter.h

    # Audio
    audio/SoundSynthesizer.h
    audio/AudioAPI.h
    audio/AudioDevice.h
    audio/AudioDeviceSDL3.h
    audio/AudioTypes.h
    audio/IAudioDevice.h
    audio/core/IAudioDriver.h

    # Input
    input/InputDevice.h
    input/InputManager.h
    input/Input.h
    input/InputBackend.h
    input/KeyCode.h
    input/core/IInputDriver.h
    input/drivers/InputDriverSDL3.h
    input/drivers/InputDriverWin32.h

    # Physics
    physics/PhysicsSystem.h
    physics/CollisionSystem.h
    physics/PhysicsComponents.h
    physics/RigidBody.h

    # Resource
    resource/AssetSerializer.h
    resource/MeshAsset.h
    resource/ResourceFallback.h
    resource/ResourceSerializer.h
    resource/TextureAsset.h
    resource/TilemapAsset.h
    resource/Archive.h
    resource/ArchiveBinary.h
    resource/ArchiveJson.h
    resource/DefaultShaders.h
    resource/OBJParser.h
    resource/ResourceFallbackImpl.h
    resource/embedded/EmbeddedResources.h

    # Tilemap
    tilemap/TilemapAsset.h
    tilemap/core/GroupLayer.h
    tilemap/core/ImageLayer.h
    tilemap/core/Map.h
    tilemap/core/ObjectLayer.h
    tilemap/core/Tile.h
    tilemap/core/TileLayer.h
    tilemap/core/Tileset.h
    tilemap/core/Types.h
    tilemap/format/TileDataDecoder.h
    tilemap/format/TmxParser.h
    tilemap/format/TsxParser.h
    tilemap/renderer/AnimatedTileManager.h
    tilemap/renderer/TileChunk.h
    tilemap/renderer/TileGeometryBuilder.h
    tilemap/renderer/TilemapRenderer.h

    # Scripting
    scripting/MonoRuntime.h
    scripting/ScriptSystem.h

    # Core
    core/AssetManager.h
    core/Asset.h
    core/UUID.h
    core/ECS.h
    core/ProjectSettings.h
    core/Serialization.h
    core/Layer.h
    core/LayerStack.h
    core/Event.h
    core/Component.h
    core/ComponentRegistry.h
    core/ISubSystem.h
    core/IWorld.h
    core/ManagerBase.h
    core/Singleton.h
    core/Handle.h
    core/StringHash.h
    core/Timestep.h
    core/IImageLoader.h
    core/ImageLoaderSTB.h
    core/HotReloadManager.h
    core/Packaging.cpp

    # UI
    ui/UIComponent.h
    ui/UIInputManager.h
    ui/2d/ButtonComponent.h
    ui/2d/CanvasComponent.h

    # Packing
    packing/Pipeline.h

    # Math
    math/MathTypes.h
    math/MatrixUtils.h
    math/MurmurHash3.h

    # Config
    config/AudioBackendConfig.h
    config/EngineConfig.h
    config/RenderBackendConfig.h

    # Graphic
    graphic/CameraComponent.h

    # PCH
    pch.h
)

# ========== 创建引擎库 ==========

# Linux 下为 LSP 生成 compile_commands.json
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_library(Engine SHARED
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    pch.h
    pch.cpp
    packing/Pipeline.h
)

message(STATUS "Engine: 构建为动态库 (SHARED)")

target_compile_definitions(Engine PRIVATE
    ENGINE_EXPORTS=1
    PRISMA_ENGINE_EXPORTS=1
    PrismaEngine_EXPORTS=1
)
target_compile_definitions(Engine PUBLIC ENGINE_BUILD_SHARED=1)

# 渲染后端宏定义 (仅 Vulkan)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_RENDER_VULKAN=1)
target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_AUDIO_SDL3=1)

set_target_properties(Engine PROPERTIES OUTPUT_NAME Prisma)

if(MSVC)
    set_target_properties(Engine PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    target_compile_options(Engine PUBLIC /utf-8)
endif()

# ========== 链接依赖项 ==========
target_link_libraries(Engine
    PUBLIC
        glm::glm-header-only
        nlohmann_json::nlohmann_json
        glaze::glaze
        stb
        tinyxml2
        imgui::imgui
)

if(PRISMA_ENABLE_AUDIO_SDL3)
    # SDL3 库链接
    # 此处使用 PRIVATE 链接，原因如下：
    # 1. 封装性：SDL3 是 Engine 的内部实现细节，外界（如游戏逻辑）应通过 Engine 接口操作窗口/音频。
    # 2. 避免污染：防止所有链接 Engine 的目标自动获得 SDL3 的所有符号，保持依赖链简洁。
    # 3. 符号一致性：插件（如 Editor）若需直接调用 SDL，应自行显式链接 SDL3-shared 目标以确保正确解析头文件路径。
    if(TARGET SDL3-shared)
        target_link_libraries(Engine PRIVATE SDL3-shared)
    elseif(TARGET SDL3::SDL3-shared)
        target_link_libraries(Engine PRIVATE SDL3::SDL3-shared)
    elseif(TARGET SDL3-static)
        target_link_libraries(Engine PRIVATE SDL3-static)
    elseif(TARGET SDL3)
        target_link_libraries(Engine PRIVATE SDL3)
    endif()
endif()

if(PRISMA_ENABLE_RENDER_VULKAN)
    target_link_libraries(Engine PUBLIC Vulkan::Vulkan)
    if(TARGET vma)
        target_link_libraries(Engine PUBLIC vma)
    elseif(TARGET VulkanMemoryAllocator)
        target_link_libraries(Engine PUBLIC VulkanMemoryAllocator)
    endif()
    if(TARGET vk-bootstrap::vk-bootstrap)
        target_link_libraries(Engine PUBLIC vk-bootstrap::vk-bootstrap)
    endif()

endif()

target_include_directories(Engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/math>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resource>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/audio>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/graphic>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/2d>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/input>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tilemap>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/app>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/scene>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/object>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/transform>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/threading>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/serialization>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/logger>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/window>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platform>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/physics>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/scripting>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/packing>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
)

target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/glm-src")
target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/stb-src")
target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/vma-src/include")

# Linux 平台额外 include
if(LINUX)
    target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/zstd-src/lib")
endif()

# ========== MCP Server ==========

if(PRISMA_ENABLE_MCP)
    target_sources(Engine PRIVATE
        mcp/MCPServer.cpp
        mcp/MCPServer.h
        mcp/MCPTool.cpp
        mcp/MCPTool.h
        mcp/MCPSubSystem.cpp
        mcp/MCPSubSystem.h
        mcp/MCPSession.cpp
        mcp/MCPSession.h
        mcp/serialization/MCPJson.cpp
        mcp/serialization/MCPJson.h
        mcp/session/DeltaTracker.cpp
        mcp/session/DeltaTracker.h
        mcp/session/TokenBudget.h
        mcp/session/TokenBudget.cpp
        mcp/transport/Transport.h
        mcp/transport/TransportStdio.cpp
        mcp/transport/TransportStdio.h
        mcp/transport/TransportTCP.cpp
        mcp/transport/TransportTCP.h
        mcp/tools/SceneTools.cpp
        mcp/tools/SceneTools.h
        mcp/tools/ECSTools.cpp
        mcp/tools/ECSTools.h
        mcp/tools/EngineTools.cpp
        mcp/tools/EngineTools.h
        mcp/tools/DebugTools.cpp
        mcp/tools/DebugTools.h
        mcp/tools/AssetTools.cpp
        mcp/tools/AssetTools.h
        mcp/tools/EditorTools.cpp
        mcp/tools/EditorTools.h
        mcp/tools/GameTools.cpp
        mcp/tools/GameTools.h
    )

    target_compile_definitions(Engine PUBLIC PRISMA_ENABLE_MCP=1)

    target_include_directories(Engine PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mcp>
    )

    # xxhash for hash-based delta tracking
    # Manually create xxhash target (CMakeLists.txt in cmake_unofficial/ is incompatible with CMake 3.31+)
    add_library(xxhash STATIC "${PRISMA_GLOBAL_DEPS_DIR}/xxhash-src/xxhash.c")
    target_include_directories(xxhash PUBLIC "${PRISMA_GLOBAL_DEPS_DIR}/xxhash-src")
    set_target_properties(xxhash PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(Engine PRIVATE xxhash)

    # TransportTCP 需要 Winsock2 (Windows)
    if(WIN32)
        target_link_libraries(Engine PRIVATE ws2_32)
    endif()

    # MCP 自测工具 (可选)
    option(PRISMA_BUILD_MCP_TESTS "Build MCP self-test" ON)
    if(PRISMA_BUILD_MCP_TESTS)
        # 直接包含 MCP 源文件，不链接 Engine DLL (避免 Vulkan/SDL 依赖)
        add_executable(test_mcp_core
            "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/mcp/test_mcp_core.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/MCPServer.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/serialization/MCPJson.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/MCPTool.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/MCPSession.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/session/DeltaTracker.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/session/TokenBudget.cpp"
        )
        target_compile_definitions(test_mcp_core PRIVATE
            PRISMA_ENABLE_MCP=1
        )
        target_include_directories(test_mcp_core PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/mcp"  # Logger.h stub (overrides engine Logger.h)
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp"
            "${CMAKE_CURRENT_SOURCE_DIR}/logger"
            "${CMAKE_CURRENT_SOURCE_DIR}/core"
            "${PRISMA_GLOBAL_DEPS_DIR}/nlohmann_json-src/single_include"
            "${PRISMA_GLOBAL_DEPS_DIR}/xxhash-src"
        )
        target_link_libraries(test_mcp_core PRIVATE xxhash)
        set_target_properties(test_mcp_core PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/mcp"
        )

        # MCP Stdio 集成测试服务器 (被 Python 客户端调用)
        add_executable(test_mcp_stdio
            "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/mcp/test_mcp_stdio.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/MCPServer.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/serialization/MCPJson.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/MCPTool.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/MCPSession.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/session/DeltaTracker.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/session/TokenBudget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp/transport/TransportStdio.cpp"
        )
        target_compile_definitions(test_mcp_stdio PRIVATE
            PRISMA_ENABLE_MCP=1
        )
        target_include_directories(test_mcp_stdio PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/mcp"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/mcp"
            "${CMAKE_CURRENT_SOURCE_DIR}/logger"
            "${CMAKE_CURRENT_SOURCE_DIR}/core"
            "${PRISMA_GLOBAL_DEPS_DIR}/nlohmann_json-src/single_include"
            "${PRISMA_GLOBAL_DEPS_DIR}/xxhash-src"
        )
        target_link_libraries(test_mcp_stdio PRIVATE xxhash)
        set_target_properties(test_mcp_stdio PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/mcp"
        )
    endif()
endif()
