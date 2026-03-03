# EngineTargets.cmake
# 引擎核心库目标定义
# 此文件定义可独立构建的 Engine 库目标

# ========== 构建选项 ==========

option(PRISMA_BUILD_ENGINE "构建引擎核心库" ON)
option(PRISMA_ENGINE_BUILD_SHARED "构建引擎为共享库(DLL)" OFF)

# ========== Engine 目标定义 ==========
# 注意：实际的 Engine 目标在 src/engine/CMakeLists.txt 中定义
# 此文件主要用于配置和验证

if(PRISMA_BUILD_ENGINE)
    message(STATUS "Engine: 构建已启用 (模式: ${PRISMA_ENGINE_BUILD_SHARED})")

    # 设置导出配置
    if(PRISMA_ENGINE_BUILD_SHARED)
        set(PRISMA_ENGINE_BUILD_SHARED ON CACHE BOOL "构建引擎为共享库" FORCE)
        set(PRISMA_BUILD_SHARED_LIBS ON CACHE BOOL "构建共享库" FORCE)
    else()
        set(PRISMA_ENGINE_BUILD_SHARED OFF CACHE BOOL "构建引擎为共享库" FORCE)
        set(PRISMA_BUILD_SHARED_LIBS OFF CACHE BOOL "构建共享库" FORCE)
    endif()

    # 定义 Engine 组件
    set(PRISMA_ENGINE_COMPONENTS
        "Core"       # 核心系统 (ECS, Asset, 异步加载)
        "Graphic"    # 渲染系统 (Vulkan/DX12/OpenGL)
        "Audio"      # 音频系统 (SDL3/XAudio2)
        "Input"      # 输入系统
        "Physics"    # 物理碰撞系统
        "UI"         # UI 系统
    )

    message(STATUS "Engine 组件: ${PRISMA_ENGINE_COMPONENTS}")
else()
    message(STATUS "Engine: 构建已禁用")
endif()

# ========== Engine 安装配置 ==========

if(PRISMA_BUILD_ENGINE)
    # 安装引擎库
    install(TARGETS Engine
        EXPORT PrismaEngineTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )

    # 安装公共头文件
    install(DIRECTORY src/engine/
        DESTINATION include/PrismaEngine
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*/adapters/*" EXCLUDE
        PATTERN "*/drivers/*" EXCLUDE
        PATTERN "*/platform/*" EXCLUDE
        PATTERN "*.cpp" EXCLUDE
    )

    # 安装 ECS 核心头文件
    install(DIRECTORY src/engine/core/
        DESTINATION include/PrismaEngine/core
        FILES_MATCHING PATTERN "*.h"
    )

    # 安装物理系统头文件
    install(FILES
        src/engine/physics/CollisionSystem.h
        src/engine/physics/FrustustumCulling.h
        DESTINATION include/PrismaEngine/physics
    )

    # 安装渲染接口
    install(DIRECTORY src/engine/graphic/interfaces/
        DESTINATION include/PrismaEngine/graphic/interfaces
        FILES_MATCHING PATTERN "*.h"
    )

    # 导出 CMake 目标
    install(EXPORT PrismaEngineTargets
        FILE PrismaEngineTargets.cmake
        NAMESPACE PrismaEngine::
        DESTINATION cmake
    )

    # 安装纹理系统头文件
    install(FILES
        src/engine/graphic/TextureAtlas.h
        src/engine/graphic/FrustumCulling.h
        DESTINATION include/PrismaEngine/graphic
    )

    # 安装输入系统头文件
    install(FILES
        src/engine/input/InputManager.h
        src/engine/input/InputDevice.h
        src/engine/input/EnhancedInputManager.h
        DESTINATION include/PrismaEngine/input
    )

    # 安装音频系统头文件
    install(FILES
        src/engine/audio/AudioManager.h
        src/engine/audio/IAudioDevice.h
        src/engine/audio/AudioAPI.h
        DESTINATION include/PrismaEngine/audio
    )
endif()

# ========== Engine 版本信息 ==========

set(PRISMA_ENGINE_VERSION_MAJOR 1)
set(PRISMA_ENGINE_VERSION_MINOR 0)
set(PRISMA_ENGINE_VERSION_PATCH 0)
set(PRISMA_ENGINE_VERSION "${PRISMA_ENGINE_VERSION_MAJOR}.${PRISMA_ENGINE_VERSION_MINOR}.${PRISMA_ENGINE_VERSION_PATCH}")

# ========== 引擎依赖配置 ==========

# 配置公共依赖
set(PRISMA_ENGINE_PUBLIC_DEPENDENCIES
    "glm::glm-header-only"
    "nlohmann_json::nlohmann_json"
)

# 配置私有依赖
set(PRISMA_ENGINE_PRIVATE_DEPENDENCIES
    "stb"
)

# 渲染后端依赖
if(PRISMA_ENABLE_RENDER_VULKAN)
    list(APPEND PRISMA_ENGINE_PUBLIC_DEPENDENCIES
        "Vulkan::Headers"
        "GPUOpen::VulkanMemoryAllocator"
        "vk-bootstrap::vk-bootstrap"
    )
endif()

if(PRISMA_ENABLE_RENDER_DX12)
    list(APPEND PRISMA_ENGINE_PUBLIC_DEPENDENCIES
        "Microsoft::DirectX-Headers"
    )
endif()

if(PRISMA_ENABLE_RENDER_OPENGL)
    list(APPEND PRISMA_ENGINE_PUBLIC_DEPENDENCIES
        "OpenGL::GL"
        "GLAD::GLAD"
    )
endif()

# 音频后端依赖
if(PRISMA_ENABLE_AUDIO_SDL3)
    list(APPEND PRISMA_ENGINE_PUBLIC_DEPENDENCIES
        "SDL3::SDL3-static"
    )
endif()

# UI 系统依赖
if(PRISMA_ENABLE_IMGUI_DEBUG)
    list(APPEND PRISMA_ENGINE_PUBLIC_DEPENDENCIES
        "imgui::imgui"
    )
endif()

message(STATUS "Engine 公共依赖: ${PRISMA_ENGINE_PUBLIC_DEPENDENCIES}")
