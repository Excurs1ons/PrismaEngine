# FetchContent.cmake
# 使用 CMake FetchContent 替代 vcpkg 管理依赖

# 包含 CMake 内置的 FetchContent 模块
include(FetchContent)

# 包含版本锁定配置
include(${CMAKE_CURRENT_LIST_DIR}/DependencyVersions.cmake OPTIONAL)

# ========== 全局流量保护与依赖共享逻辑 ==========

# 定义全局依赖存放路径（项目根目录下的 .dependencies）
set(PRISMA_GLOBAL_DEPS_DIR "${PROJECT_SOURCE_DIR}/.dependencies")

# 强制告诉 FetchContent 使用这个全局目录
set(FETCHCONTENT_BASE_DIR "${PRISMA_GLOBAL_DEPS_DIR}" CACHE PATH "全局依赖缓存目录" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

# 辅助宏：声明依赖项，强制指向全局目录
macro(Prisma_Declare_Dependency NAME REPO TAG)
    set(DEP_SOURCE_DIR "${PRISMA_GLOBAL_DEPS_DIR}/${NAME}-src")
    
    # 强制锁定本地目录，不论是否存在（由 FetchContent 处理下载或复用）
    FetchContent_Declare(
        ${NAME}
        GIT_REPOSITORY ${REPO}
        GIT_TAG ${TAG}
        GIT_SHALLOW ON
        SOURCE_DIR "${DEP_SOURCE_DIR}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/${NAME}-build"
    )
    message(STATUS "  [依赖锁定] ${NAME} -> ${DEP_SOURCE_DIR}")
endmacro()

# ========== 依赖项声明 ==========

# GLM
Prisma_Declare_Dependency(glm https://github.com/g-truc/glm.git ${PRISMA_DEP_GLM_VERSION})

# nlohmann_json
Prisma_Declare_Dependency(nlohmann_json https://github.com/nlohmann/json.git ${PRISMA_DEP_NLOHMANN_JSON_VERSION})

# stb
Prisma_Declare_Dependency(stb https://github.com/nothings/stb.git ${PRISMA_DEP_STB_VERSION})

# tinyxml2
set(TINYXML2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
Prisma_Declare_Dependency(tinyxml2 https://github.com/leethomason/tinyxml2.git ${PRISMA_DEP_TINYXML2_VERSION})

# zstd
Prisma_Declare_Dependency(zstd https://github.com/facebook/zstd.git ${PRISMA_DEP_ZSTD_VERSION})

# SDL3
Prisma_Declare_Dependency(SDL3 https://github.com/libsdl-org/SDL.git ${PRISMA_DEP_SDL3_VERSION})

# Vulkan-Headers
Prisma_Declare_Dependency(Vulkan-Headers https://github.com/KhronosGroup/Vulkan-Headers.git ${PRISMA_DEP_VULKAN_HEADERS_VERSION})

# VMA
Prisma_Declare_Dependency(vma https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git ${PRISMA_DEP_VMA_VERSION})

# vk-bootstrap
Prisma_Declare_Dependency(vk-bootstrap https://github.com/charles-lunarg/vk-bootstrap.git ${PRISMA_DEP_VK_BOOTSTRAP_VERSION})

# DirectX-Headers (Windows)
if(WIN32)
    Prisma_Declare_Dependency(DirectX-Headers https://github.com/microsoft/DirectX-Headers.git ${PRISMA_DEP_DIRECTX_HEADERS_VERSION})
endif()

# ImGui
Prisma_Declare_Dependency(imgui https://github.com/ocornut/imgui.git ${PRISMA_DEP_IMGUI_VERSION})

# libdeflate (OpenFBX 依赖)
if(WIN32)
    Prisma_Declare_Dependency(libdeflate https://github.com/ebiggers/libdeflate.git ${PRISMA_DEP_LIBDEFLATE_VERSION})
endif()

# OpenFBX
if(WIN32)
    Prisma_Declare_Dependency(openfbx https://github.com/nem0/OpenFBX.git ${PRISMA_DEP_OPENFBX_VERSION})
endif()

# FSR SDK
if(PRISMA_ENABLE_UPSCALER_FSR)
    Prisma_Declare_Dependency(FidelityFX-SDK https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK.git ${PRISMA_DEP_FIDELITYFX_SDK_VERSION})
endif()

# Streamline SDK
if(PRISMA_ENABLE_UPSCALER_DLSS)
    Prisma_Declare_Dependency(Streamline https://github.com/NVIDIA-RTX/Streamline.git ${PRISMA_DEP_STREAMLINE_VERSION})
endif()

# ========== 依赖项加载与配置 ==========

# 禁用所有不需要的测试和示例
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NLOHMANN_ADD_NATVIS OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)

# 加载依赖
FetchContent_MakeAvailable(glm nlohmann_json stb tinyxml2 zstd)

if(TARGET stb AND NOT TARGET stb::stb)
    # stb 是 header-only，FetchContent 可能没创建目标
    add_library(stb INTERFACE IMPORTED GLOBAL)
    set_target_properties(stb PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${stb_SOURCE_DIR}")
endif()

if(PRISMA_BUILD_EDITOR OR PRISMA_ENABLE_RENDER_VULKAN)
    FetchContent_MakeAvailable(SDL3)
endif()

if(PRISMA_ENABLE_RENDER_VULKAN)
    FetchContent_MakeAvailable(Vulkan-Headers vma vk-bootstrap)
endif()

if(WIN32)
    FetchContent_MakeAvailable(DirectX-Headers)
    if(TARGET DirectX-Headers AND NOT TARGET Microsoft::DirectX-Headers)
        add_library(Microsoft::DirectX-Headers ALIAS DirectX-Headers)
    endif()
endif()

# ImGui 静态库创建与后端配置
if(PRISMA_BUILD_EDITOR OR PRISMA_ENABLE_IMGUI_DEBUG)
    FetchContent_MakeAvailable(imgui)
    
    set(IMGUI_CORE_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
    )

    if(WIN32)
        list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp)
    endif()

    if(PRISMA_ENABLE_RENDER_DX12 AND WIN32)
        list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp)
    endif()

    if(PRISMA_ENABLE_RENDER_VULKAN)
        list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
    endif()

    if(PRISMA_BUILD_EDITOR AND EXISTS ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
        list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
    endif()

    add_library(imgui STATIC ${IMGUI_CORE_SOURCES})
    set_property(TARGET imgui PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    add_library(imgui::imgui ALIAS imgui)
endif()

if(WIN32 AND PRISMA_BUILD_EDITOR)
    FetchContent_MakeAvailable(libdeflate openfbx)
endif()

if(PRISMA_ENABLE_UPSCALER_FSR)
    FetchContent_MakeAvailable(FidelityFX-SDK)
endif()

if(PRISMA_ENABLE_UPSCALER_DLSS)
    FetchContent_MakeAvailable(Streamline)
endif()
