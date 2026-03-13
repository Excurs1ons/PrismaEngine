# 引擎核心库 - Prune 分支版本
# 仅支持 Windows x64 + Vulkan + SDL3

# 平台检测 (Windows only for build, Linux for LSP)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    add_compile_options(/DNOMINMAX /DWIN32_LEAN_AND_MEAN)
endif()

# 包含子目录逻辑
include(graphic/CMakeLists.txt)
include(physics/CMakeLists.txt)

# 寻找 Vulkan
if(PRISMA_ENABLE_RENDER_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

# ========== 收集核心源文件 (所有平台共用) ==========
set(CORE_SOURCES
    CommandLineParser.cpp
    Engine.cpp
    IApplication.cpp
    JobSystem.cpp
    Logger.cpp
    LogScope.cpp
    Object.cpp
    PhysicsSystem.cpp
    Platform.cpp
    ResourceManager.cpp
    Scene.cpp
    SceneManager.cpp
    Serializable.cpp
    ThreadManager.cpp
    WorkerThread.cpp
    Camera.cpp
    CameraController.cpp
    GameObject.cpp
    ScriptComponent.cpp
    Transform.cpp
    SoundSynthesizer.cpp
    TriangleExample.cpp
    pch.cpp
    scripting/MonoRuntime.cpp
    scripting/ScriptSystem.cpp
    core/AssetManager.cpp
    core/ECS.cpp
    core/AsyncLoader.cpp
    core/Serialization.cpp
    ui/UIComponent.cpp
    ui/UIInputManager.cpp
    ui/2d/ButtonComponent.cpp
    ui/2d/CanvasComponent.cpp
    audio/AudioDeviceNull.cpp
    audio/AudioDeviceSDL3.cpp
    audio/AudioAPI.cpp
    input/InputDevice.cpp
    input/InputManager.cpp
    resource/Asset.cpp
    resource/AssetSerializer.cpp
    resource/MeshAsset.cpp
    resource/ResourceFallback.cpp
    resource/ResourceFallbackImpl.cpp
    resource/TextureAsset.cpp
    tilemap/TilemapAsset.cpp
    tilemap/format/TileDataDecoder.cpp
    tilemap/format/TmxParser.cpp
    tilemap/format/TsxParser.cpp
    tilemap/renderer/TilemapRenderer.cpp
)

# ========== 平台特定源文件 ==========
if(WIN32)
    list(APPEND CORE_SOURCES
        PlatformWindows.cpp
        platform/Application.cpp
        platform/ApplicationWindows.cpp
        platform/WindowsUtils.cpp
        input/drivers/InputDriverWin32.cpp
    )
elseif(LINUX)
    list(APPEND CORE_SOURCES
        PlatformSDL.cpp
        platform/Application.cpp
    )
endif()

# ========== 收集核心头文件 ==========
set(CORE_HEADERS
    CommandLineParser.h
    Common.h
    Engine.h
    JobSystem.h
    Logger.h
    LogScope.h
    Object.h
    PhysicsSystem.h
    Platform.h
    ResourceManager.h
    Scene.h
    SceneManager.h
    Serializable.h
    Serialization.h
    ThreadManager.h
    WorkerThread.h
    Camera.h
    CameraController.h
    GameObject.h
    ScriptComponent.h
    Transform.h
    CubemapTextureAsset.h
    TextureAsset.h
    SoundSynthesizer.h
    TriangleExample.h
    pch.h
    packing/Pipeline.h
    scripting/MonoRuntime.h
    scripting/ScriptSystem.h
    core/AssetManager.h
    core/ECS.h
    core/ProjectSettings.h
    core/Serialization.h
    ui/UIComponent.h
    ui/UIInputManager.h
    ui/2d/ButtonComponent.h
    ui/2d/CanvasComponent.h
    audio/AudioAPI.h
    audio/AudioDevice.h
    audio/AudioDeviceSDL3.h
    audio/AudioTypes.h
    audio/IAudioDevice.h
    input/InputDevice.h
    input/InputManager.h
    resource/Asset.h
    resource/AssetSerializer.h
    resource/MeshAsset.h
    resource/ResourceFallback.h
    resource/TextureAsset.h
    resource/TilemapAsset.h
)

# ========== 创建引擎库 ==========
set(ENGINE_SOURCES
    ${CORE_SOURCES}
    ${ENGINE_GRAPHIC_SOURCES}
)

set(ENGINE_HEADERS
    ${CORE_HEADERS}
    ${ENGINE_GRAPHIC_HEADERS}
)

# Linux 下为 LSP 生成 compile_commands.json
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_library(Engine SHARED
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
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

set_target_properties(Engine PROPERTIES OUTPUT_NAME PrismaEngine)

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
        stb
        tinyxml2
)

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
    if(TARGET imgui)
        target_link_libraries(Engine PUBLIC imgui)
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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/input>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tilemap>
)

target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/glm-src")
target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/stb-src")
target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/vma-src/include")

# Linux 平台额外 include
if(LINUX)
    target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/.dependencies/zstd-src/lib")
endif()
