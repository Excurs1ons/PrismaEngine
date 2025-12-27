# Android Engine Library - 生成 libEngine.so
cmake_minimum_required(VERSION 3.31)

# ========== Android 平台配置 ==========

# Android 平台默认使用动态库（.so）
option(PRISMA_ENGINE_BUILD_SHARED "Build engine as shared library (so)" ON)

# 设置库类型变量
if(PRISMA_ENGINE_BUILD_SHARED)
    set(PRISMA_ENGINE_TYPE SHARED)
    set(PRISMA_ENGINE_LIBRARY_TYPE SHARED)
    message(STATUS "Android Engine: Building as SHARED library (.so)")
else()
    set(PRISMA_ENGINE_TYPE STATIC)
    set(PRISMA_ENGINE_LIBRARY_TYPE STATIC)
    message(STATUS "Android Engine: Building as STATIC library (.a)")
endif()

# 禁用 SDL3（Android 不需要 SDL3，使用 Vulkan + OpenAL）
set(PRISMA_ENABLE_AUDIO_SDL3 OFF CACHE BOOL "Enable SDL3 audio device" FORCE)
set(PRISMA_ENABLE_RENDER_SDL3 OFF CACHE BOOL "Enable SDL3 render device" FORCE)

# 包含父级目录的graphic配置
include(../graphic/CMakeLists.txt)

# 收集引擎源文件
set(ENGINE_SOURCES "")
set(ENGINE_HEADERS "")

# 添加core目录下的源文件（除了graphic目录）
file(GLOB_RECURSE CORE_SOURCES
    ../*.cpp
)

list(FILTER CORE_SOURCES EXCLUDE REGEX "graphic/")
list(FILTER CORE_SOURCES EXCLUDE REGEX "android/")
list(FILTER CORE_SOURCES EXCLUDE REGEX "platform/.*Windows.*")
list(FILTER CORE_SOURCES EXCLUDE REGEX "audio/AudioBackendXAudio2.*")

file(GLOB_RECURSE CORE_HEADERS
    ../*.h
)

list(FILTER CORE_HEADERS EXCLUDE REGEX "graphic/")
list(FILTER CORE_HEADERS EXCLUDE REGEX "android/")
list(FILTER CORE_HEADERS EXCLUDE REGEX "platform/.*Windows.*")
list(FILTER CORE_HEADERS EXCLUDE REGEX "audio/AudioBackendXAudio2.*")

# 添加graphic目录下的源文件和头文件
set(ENGINE_SOURCES
    ${CORE_SOURCES}
    ${ENGINE_GRAPHIC_SOURCES}
)

set(ENGINE_HEADERS
    ${CORE_HEADERS}
    ${ENGINE_GRAPHIC_HEADERS}
)

# 创建Android引擎库
add_library(engine ${PRISMA_ENGINE_TYPE}
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    ../pch.h
    ../pch.cpp
    ../packing/pipeline.h
)

# 设置包含目录
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
    PRIVATE
        ../resource
        ../audio
        ../graphic
        ../platform
)

# Android NDK头文件目录
if(ANDROID)
    if(DEFINED ANDROID_NDK)
        target_include_directories(engine PRIVATE
            ${ANDROID_NDK}/sysroot/usr/include
            ${ANDROID_NDK}/sources/android/native_app_glue
        )
    endif()
endif()

# 设置编译定义
target_compile_definitions(engine
    PUBLIC ANDROID
)

# ========== 导出符号配置 ==========

# 动态库需要导出宏，静态库不需要
if(PRISMA_ENGINE_BUILD_SHARED)
    target_compile_definitions(engine PRIVATE
        ENGINE_BUILD_SHARED=1
        ENGINE_EXPORTS=1
    )
else()
    # 静态库不需要导出宏
    target_compile_definitions(engine PRIVATE
        ENGINE_BUILD_STATIC=1
    )
endif()

# Android特定编译选项
target_compile_options(engine PRIVATE
    -fPIC
    -Wall
    -Wno-unused-parameter
)

# 查找依赖包
# SDL3 已禁用，不再查找
# set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as static library" FORCE)
# set(SDL_STATIC ON CACHE BOOL "Build SDL3 as static library" FORCE)
# find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# 链接依赖库
target_link_libraries(engine PUBLIC
    # SDL3::SDL3-static  # SDL3 已禁用
    Vulkan::Vulkan
    nlohmann_json::nlohmann_json
    imgui::imgui
)

# 链接Android系统库
target_link_libraries(engine PRIVATE
    android
    log
    EGL
    GLESv3
    jnigraphics
)

# SDL3 音频后端已在主 CMakeLists.txt 中通过 PRISMA_ENABLE_AUDIO_SDL3 控制
# target_sources(engine PRIVATE
#     ../audio/AudioBackendSDL3.cpp
# )

# 设置输出名称
set_target_properties(engine PROPERTIES
    OUTPUT_NAME "engine"
    VERSION 1.0.0
    SOVERSION 1
)

# 设置公共头文件
set_target_properties(engine PROPERTIES
    PUBLIC_HEADER "${ENGINE_HEADERS}"
)

# 安装规则
install(TARGETS engine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/prisma/engine
)