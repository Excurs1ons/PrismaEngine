# 引擎核心库 - 条件编译版本

# 包含graphic子目录的CMakeLists.txt来定义变量
include(graphic/CMakeLists.txt)

# 收集源文件
set(ENGINE_SOURCES "")
set(ENGINE_HEADERS "")

# 添加core目录下的源文件（除了graphic和audio目录）
file(GLOB_RECURSE CORE_SOURCES
    "*.cpp"
)
list(FILTER CORE_SOURCES EXCLUDE REGEX "graphic/")
list(FILTER CORE_SOURCES EXCLUDE REGEX "audio/")

file(GLOB_RECURSE CORE_HEADERS
    "*.h")
)
list(FILTER CORE_HEADERS EXCLUDE REGEX "graphic/")
list(FILTER CORE_HEADERS EXCLUDE REGEX "audio/")

# 添加基础核心文件
set(ENGINE_SOURCES
    ${CORE_SOURCES}
    ${ENGINE_GRAPHIC_SOURCES}
)
set(ENGINE_HEADERS
    ${CORE_HEADERS}
    ${ENGINE_GRAPHIC_HEADERS}
)

# ========== 音频设备条件编译 ==========

# 基础音频文件（总是包含）
list(APPEND ENGINE_SOURCES
    audio/AudioFactory.cpp
)

list(APPEND ENGINE_HEADERS
    audio/AudioTypes.h
    audio/IAudioDevice.h
    audio/AudioManager.h
    audio/AudioFactory.h
)

# XAudio2 (Windows)
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    list(APPEND ENGINE_SOURCES
        audio/AudioDeviceXAudio2.cpp
    )
    list(APPEND ENGINE_HEADERS
        audio/AudioDeviceXAudio2.h
    )
    message(STATUS "Adding XAudio2 audio device")
endif()

# OpenAL (跨平台)
if(PRISMA_ENABLE_AUDIO_OPENAL)
    list(APPEND ENGINE_SOURCES
        audio/AudioDeviceOpenAL.cpp
    )
    list(APPEND ENGINE_HEADERS
        audio/AudioDeviceOpenAL.h
    )
    message(STATUS "Adding OpenAL audio device")
endif()

# SDL3 Audio (跨平台)
if(PRISMA_ENABLE_AUDIO_SDL3)
    list(APPEND ENGINE_SOURCES
        audio/AudioDeviceSDL3.cpp
    )
    list(APPEND ENGINE_HEADERS
        audio/AudioDeviceSDL3.h
    )
    message(STATUS "Adding SDL3 audio device")
endif()

# ========== 渲染设备条件编译 ==========

# DirectX12 (Windows)
if(PRISMA_ENABLE_RENDER_DX12)
    # 重命名旧的DX12RenderDevice为RenderDeviceDX12
    list(APPEND ENGINE_GRAPHIC_SOURCES
        graphic/adapters/dx12/DX12RenderDevice.cpp
    )
    list(APPEND ENGINE_GRAPHIC_HEADERS
        graphic/adapters/dx12/DX12RenderDevice.h
    )
    message(STATUS "Adding DirectX12 render device")
endif()

# OpenGL (跨平台)
if(PRISMA_ENABLE_RENDER_OPENGL)
    list(APPEND ENGINE_GRAPHIC_SOURCES
        graphic/adapters/opengl/RenderDeviceOpenGL.cpp
    )
    list(APPEND ENGINE_GRAPHIC_HEADERS
        graphic/adapters/opengl/RenderDeviceOpenGL.h
    )
    message(STATUS "Adding OpenGL render device")
endif()

# Vulkan (跨平台)
if(PRISMA_ENABLE_RENDER_VULKAN)
    list(APPEND ENGINE_GRAPHIC_SOURCES
        graphic/adapters/vulkan/RenderDeviceVulkan.cpp
    )
    list(APPEND ENGINE_GRAPHIC_HEADERS
        graphic/adapters/vulkan/RenderDeviceVulkan.h
    )
    message(STATUS "Adding Vulkan render device")
endif()

# ========== 创建静态库 ==========

add_library(Engine STATIC
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    pch.h
    pch.cpp
    packing/pipeline.h
)

# ========== 依赖项配置 ==========

# 音频设备依赖
if(PRISMA_ENABLE_AUDIO_XAUDIO2)
    if(WIN32)
        target_link_libraries(Engine PUBLIC xaudio2.lib)
    endif()
endif()

if(PRISMA_ENABLE_AUDIO_OPENAL)
    find_package(OpenAL QUIET)
    if(OpenAL_FOUND)
        target_include_directories(Engine PRIVATE ${OpenAL_INCLUDE_DIRS})
        target_link_libraries(Engine PRIVATE ${OpenAL_LIBRARIES})
    else()
        message(WARNING "OpenAL not found, but device is enabled")
    endif()
endif()

if(PRISMA_ENABLE_AUDIO_SDL3)
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        target_include_directories(Engine PRIVATE ${SDL3_INCLUDE_DIRS})
        target_link_libraries(Engine PRIVATE SDL3::SDL3)
    else()
        message(WARNING "SDL3 not found, but device is enabled")
    endif()
endif()

# 渲染设备依赖
if(PRISMA_ENABLE_RENDER_VULKAN)
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        target_include_directories(Engine PRIVATE ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(Engine PRIVATE ${Vulkan_LIBRARIES})
    else()
        message(WARNING "Vulkan not found, but backend is enabled")
    endif()
endif()

if(PRISMA_ENABLE_RENDER_OPENGL)
    find_package(OpenGL QUIET)
    if(OPENGL_FOUND)
        target_include_directories(Engine PRIVATE ${OPENGL_INCLUDE_DIRS})
        target_link_libraries(Engine PRIVATE ${OPENGL_LIBRARIES})
    else()
        message(WARNING "OpenGL not found, but backend is enabled")
    endif()

    find_package(glad QUIET)
    if(glad_FOUND)
        target_include_directories(Engine PRIVATE ${glad_INCLUDE_DIRS})
        target_link_libraries(Engine PRIVATE ${glad_LIBRARIES})
    endif()

    find_package(glfw3 QUIET)
    if(GLFW3_FOUND)
        target_include_directories(Engine PRIVATE ${glfw3_INCLUDE_DIRS})
        target_link_libraries(Engine PRIVATE glfw)
    endif()
endif()

# ========== 设置包含目录 ==========

target_include_directories(Engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/resource
        ${CMAKE_CURRENT_SOURCE_DIR}/audio
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/config
)

# ========== 编译定义 ==========

# 根据启用的设备定义宏
target_compile_definitions(Engine PRIVATE
    PRISMA_DEFAULT_AUDIO_DEVICE=${PRISMA_DEFAULT_AUDIO_BACKEND}
    PRISMA_DEFAULT_RENDER_DEVICE=${PRISMA_DEFAULT_RENDER_BACKEND}
)