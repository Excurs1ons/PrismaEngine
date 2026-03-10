# FetchContent.cmake
# 使用 CMake FetchContent 替代 vcpkg 管理依赖

include(FetchContent)
include(${CMAKE_CURRENT_LIST_DIR}/DependencyVersions.cmake OPTIONAL)

# ========== 全局流量保护与依赖共享逻辑 ==========

set(PRISMA_GLOBAL_DEPS_DIR "${PROJECT_SOURCE_DIR}/.dependencies")
set(FETCHCONTENT_BASE_DIR "${PRISMA_GLOBAL_DEPS_DIR}" CACHE PATH "全局依赖缓存目录" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

# 静音 FetchContent 自身的输出
set(FETCHCONTENT_QUIET ON CACHE BOOL "" FORCE)

# 辅助宏：声明依赖项，实现真正的离线优先
macro(Prisma_Declare_Dependency NAME REPO TAG)
    set(DEP_SOURCE_DIR "${PRISMA_GLOBAL_DEPS_DIR}/${NAME}-src")
    set(DEP_SOURCE_DIR_ALT "${PRISMA_GLOBAL_DEPS_DIR}/${NAME}")
    
    set(ACTUAL_SOURCE_DIR "")
    set(REPO_MARKERS "CMakeLists.txt" "LICENSE" "LICENSE.txt" "README.md" "README" "include")

    foreach(marker ${REPO_MARKERS})
        if(EXISTS "${DEP_SOURCE_DIR}/${marker}")
            set(ACTUAL_SOURCE_DIR "${DEP_SOURCE_DIR}")
            break()
        elseif(EXISTS "${DEP_SOURCE_DIR_ALT}/${marker}")
            set(ACTUAL_SOURCE_DIR "${DEP_SOURCE_DIR_ALT}")
            break()
        endif()
    endforeach()

    if(ACTUAL_SOURCE_DIR)
        message(STATUS "  [SHARED]  ${NAME} -> ${ACTUAL_SOURCE_DIR}")
        FetchContent_Declare(${NAME} SOURCE_DIR "${ACTUAL_SOURCE_DIR}" BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/${NAME}-build")
    else()
        message(STATUS "  [FETCH]   ${NAME} <- ${REPO}")
        FetchContent_Declare(${NAME} GIT_REPOSITORY ${REPO} GIT_TAG ${TAG} GIT_SHALLOW ON SOURCE_DIR "${DEP_SOURCE_DIR}" BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/${NAME}-build")
    endif()
endmacro()

# ========== 依赖项声明 ==========

Prisma_Declare_Dependency(glm https://github.com/g-truc/glm.git ${PRISMA_DEP_GLM_VERSION})
Prisma_Declare_Dependency(nlohmann_json https://github.com/nlohmann/json.git ${PRISMA_DEP_NLOHMANN_JSON_VERSION})
Prisma_Declare_Dependency(stb https://github.com/nothings/stb.git ${PRISMA_DEP_STB_VERSION})
set(TINYXML2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
Prisma_Declare_Dependency(tinyxml2 https://github.com/leethomason/tinyxml2.git ${PRISMA_DEP_TINYXML2_VERSION})
Prisma_Declare_Dependency(zstd https://github.com/facebook/zstd.git ${PRISMA_DEP_ZSTD_VERSION})
Prisma_Declare_Dependency(SDL3 https://github.com/libsdl-org/SDL.git ${PRISMA_DEP_SDL3_VERSION})
Prisma_Declare_Dependency(Vulkan-Headers https://github.com/KhronosGroup/Vulkan-Headers.git ${PRISMA_DEP_VULKAN_HEADERS_VERSION})
Prisma_Declare_Dependency(vma https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git ${PRISMA_DEP_VMA_VERSION})
Prisma_Declare_Dependency(vk-bootstrap https://github.com/charles-lunarg/vk-bootstrap.git ${PRISMA_DEP_VK_BOOTSTRAP_VERSION})

if(WIN32)
    Prisma_Declare_Dependency(DirectX-Headers https://github.com/microsoft/DirectX-Headers.git ${PRISMA_DEP_DIRECTX_HEADERS_VERSION})
endif()

Prisma_Declare_Dependency(imgui https://github.com/ocornut/imgui.git ${PRISMA_DEP_IMGUI_VERSION})

if(WIN32 AND PRISMA_BUILD_EDITOR)
    Prisma_Declare_Dependency(libdeflate https://github.com/ebiggers/libdeflate.git ${PRISMA_DEP_LIBDEFLATE_VERSION})
    Prisma_Declare_Dependency(openfbx https://github.com/nem0/OpenFBX.git ${PRISMA_DEP_OPENFBX_VERSION})
endif()

# ========== 依赖项加载与配置 ==========

# 禁用所有不需要的测试、示例和程序
set(GLM_QUIET ON CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NLOHMANN_ADD_NATVIS OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)

# 针对 tinyxml2 和 libdeflate 的专项屏蔽
set(TINYXML2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
# 强制 tinyxml2 使用 -fPIC 编译（链接成共享库需要）
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(LIBDEFLATE_BUILD_GZIP OFF CACHE BOOL "" FORCE)
set(LIBDEFLATE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LIBDEFLATE_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)

# 暂时提升消息等级以压制第三方库的繁杂输出
set(OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
set(CMAKE_MESSAGE_LOG_LEVEL WARNING)
set(CMAKE_WARN_DEPRECATED OFF)

# 加载依赖 (使用 EXCLUDE_FROM_ALL 进一步隔离不需要的 target)
FetchContent_MakeAvailable(glm nlohmann_json stb tinyxml2 zstd)

# STB 总是作为接口库处理
if(NOT TARGET stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE "${stb_SOURCE_DIR}")
endif()
if(NOT TARGET stb::stb)
    add_library(stb::stb ALIAS stb)
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

# ImGui 静态库创建
if(PRISMA_BUILD_EDITOR OR PRISMA_ENABLE_IMGUI_DEBUG)
    FetchContent_MakeAvailable(imgui)
    
    set(IMGUI_CORE_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
    )

    # Windows 后端
    if(WIN32)
        list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp)
        if(PRISMA_ENABLE_RENDER_DX12)
            list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp)
        endif()
    endif()

    # Vulkan 后端 - 跨平台
    if(PRISMA_ENABLE_RENDER_VULKAN)
        list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
    endif()

    # SDL3 后端 - 跨平台（包括 Windows）
    if(EXISTS ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
        list(APPEND IMGUI_CORE_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
    endif()

    if(NOT TARGET imgui)
        add_library(imgui STATIC ${IMGUI_CORE_SOURCES})
        target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
        
        # 强力注入包含路径
        target_include_directories(imgui SYSTEM PUBLIC 
            "${PRISMA_GLOBAL_DEPS_DIR}/SDL3-src/include"
            "${PRISMA_GLOBAL_DEPS_DIR}/Vulkan-Headers-src/include"
        )

        add_library(imgui::imgui ALIAS imgui)
    endif()
endif()

if(WIN32 AND PRISMA_BUILD_EDITOR)
    FetchContent_MakeAvailable(libdeflate openfbx)
endif()

# 确保 GLM ALIAS targets 存在
if(TARGET glm AND NOT TARGET glm::glm)
    add_library(glm::glm ALIAS glm)
endif()
if(TARGET glm AND NOT TARGET glm::glm-header-only)
    add_library(glm::glm-header-only ALIAS glm)
endif()

# 恢复消息等级
set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})
set(CMAKE_WARN_DEPRECATED ON)

# 确保 VMA ALIAS targets 存在（类似 GLM）
if(TARGET vma AND NOT TARGET vma::VulkanMemoryAllocator)
    add_library(vma::VulkanMemoryAllocator ALIAS vma)
endif()
if(TARGET vma AND NOT TARGET vma::VMA)
    add_library(vma::VMA ALIAS vma)
endif()

# 添加 VMA include 目录
if(TARGET vma)
    get_target_property(VMA_INCLUDE_DIRS vma INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT VMA_INCLUDE_DIRS)
        # VMA 可能没有正确设置 include 目录，手动添加
        get_target_property(VMA_SOURCE_DIR vma SOURCE_DIR)
        if(VMA_SOURCE_DIR)
            if(EXISTS "${VMA_SOURCE_DIR}/include")
                target_include_directories(vma INTERFACE "${VMA_SOURCE_DIR}/include")
            endif()
        endif()
    endif()
endif()
