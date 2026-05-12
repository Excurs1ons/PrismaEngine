cmake_minimum_required(VERSION 3.20)

project(Template2D
    VERSION 1.0.0
    DESCRIPTION "2D Scene Test Template for PrismaEngine"
    LANGUAGES CXX
)

# ========== 编译选项 ==========

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 源文件 ==========

set(TEST_SOURCES
    src/CreateApplication.cpp
    src/Template2DApp.cpp
)

set(TEST_HEADERS
    src/Template2DApp.h
)

# ========== 插件 DLL（由 PrismaLauncher 运行时加载） ==========

add_library(${PROJECT_NAME} SHARED
    ${TEST_SOURCES}
    ${TEST_HEADERS}
)

# ========== 链接引擎 ==========

target_link_libraries(${PROJECT_NAME} PRIVATE
    Engine
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    TEMPLATE2D_EXPORTS=1
)

# ========== 包含目录 ==========

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/engine/app
        ${CMAKE_SOURCE_DIR}/src/engine/core
        ${CMAKE_SOURCE_DIR}/src/engine/graphic
        ${CMAKE_SOURCE_DIR}/src/engine/logger
        ${CMAKE_SOURCE_DIR}/src/engine/platform
        ${CMAKE_SOURCE_DIR}/src/engine/scripting
        ${CMAKE_SOURCE_DIR}/src/engine/window
        ${CMAKE_SOURCE_DIR}/src/engine/math
        ${CMAKE_SOURCE_DIR}/src/engine/resource
        ${CMAKE_SOURCE_DIR}/src/engine/audio
        ${CMAKE_SOURCE_DIR}/src/engine/ui
        ${CMAKE_SOURCE_DIR}/src/engine/ui/2d
        ${CMAKE_SOURCE_DIR}/src/engine/input
        ${CMAKE_SOURCE_DIR}/src/engine/tilemap
        ${CMAKE_SOURCE_DIR}/src/engine/scene
        ${CMAKE_SOURCE_DIR}/src/engine/object
        ${CMAKE_SOURCE_DIR}/src/engine/transform
        ${CMAKE_SOURCE_DIR}/src/engine/threading
        ${CMAKE_SOURCE_DIR}/src/engine/serialization
        ${CMAKE_SOURCE_DIR}/src/engine/physics
        ${CMAKE_SOURCE_DIR}/src/engine/config
        ${CMAKE_SOURCE_DIR}/src/engine/packing
        ${CMAKE_SOURCE_DIR}/src/engine/utils
)

# ========== VS 调试器配置（一键启动） ==========

if(MSVC)
    # Template2D 是 DLL 插件，F5 调试时自动启动 PrismaLauncher 并传入插件路径
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND "$<TARGET_FILE:PrismaLauncher>")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "$<TARGET_FILE:${PROJECT_NAME}>")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:PrismaLauncher>")
endif()

# ========== 编译选项 ==========

if(MSVC)
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            /W4
            /WX-
            /utf-8
            $<$<CONFIG:Release>:/O2 /Oi /Gy /Zi>
            $<$<CONFIG:MinSizeRel>:/O1 /Oi /Gy>
            $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Gy>
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/DEBUG>)
else()
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

# ========== 复制项目资源 ==========

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying shaders and project assets"
)

# ========== 依赖 C# 引擎核心库 ==========

# 依赖引擎 C# 核心库（PrismaCore 输出到 bin/scripts/，同时复制到 project 本地 lib）
add_dependencies(${PROJECT_NAME} PrismaCore)
set(CS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts/lib")

# ========== C# 脚本构建与部署 (跨平台) ==========

find_program(DOTNET dotnet REQUIRED)

# 动态确定运行时标识符 (RID)
if(WIN32)
    set(CS_RID "win-x64")
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CS_RID "linux-arm64")
    else()
        set(CS_RID "linux-x64")
    endif()
endif()

set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(SCRIPTS_OUT "$<TARGET_FILE_DIR:${PROJECT_NAME}>/scripts")

# 构建时：跨平台自包含发布
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Publishing C# scripts for RID: ${CS_RID}..."
    COMMAND ${DOTNET} publish -c Release -r ${CS_RID} --self-contained true
            -p:DebugType=portable -p:DebugSymbols=true
            -p:PrismaCoreRef="${CS_LIB_DIR}"
            --nologo
            "${SCRIPTS_DIR}/GameScripts/GameScripts.csproj"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/${CS_RID}/publish/"
            "${SCRIPTS_OUT}/"
    COMMENT "Building + publishing C# scripts (RID: ${CS_RID})"
)

message(STATUS "Template2D configured for RID: ${CS_RID}")
