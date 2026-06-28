cmake_minimum_required(VERSION 3.20)

project(MLGIPipeline
    VERSION 1.0.0
    DESCRIPTION "Multi-Level GI Pipeline for PrismaEngine"
    LANGUAGES CXX
)

# ========== 编译选项 ==========

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 共享源码 ==========

set(MLGIPIPELINE_SOURCES
    src/MLGIPipelineApp.cpp
    src/MLGISystem.cpp
    src/SampleSource.cpp
    src/StatsOverlay.cpp
    src/HeadlessRunner.cpp
    src/MLGIProbeUpdatePass.cpp
    src/MLGITemporalPass.cpp
    src/MLGIScreenGatherPass.cpp
)

set(MLGIPIPELINE_HEADERS
    src/MLGIPipelineApp.h
    src/MLGISystem.h
    src/SampleSource.h
    src/StatsOverlay.h
    src/HeadlessRunner.h
    src/ProbeGrid.h
    src/ProbeSHBuffer.h
    src/MLGIProbeUpdatePass.h
    src/MLGITemporalPass.h
    src/MLGIScreenGatherPass.h
)

# ========== 共享包含目录 ==========

set(MLGIPIPELINE_INCLUDE_DIRS
    # Engine 包含目录由 target_link_libraries(Engine) 通过 PUBLIC 自动继承
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ========== 共享编译选项宏 ==========

macro(setup_target_defaults target)
    target_include_directories(${target} PRIVATE ${MLGIPIPELINE_INCLUDE_DIRS})
    target_link_libraries(${target} PRIVATE Engine)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /WX- /utf-8
            $<$<CONFIG:Release>:/O2 /Oi /Gy /Zi>
            $<$<CONFIG:MinSizeRel>:/O1 /Oi /Gy>
            $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Gy>
        )
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN
        )
        target_link_options(${target} PRIVATE $<$<CONFIG:Release>:/DEBUG>)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endmacro()

# ============================================================
# Target 1: MLGIPipeline (EXE)
# ============================================================

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${MLGIPIPELINE_SOURCES}
    ${MLGIPIPELINE_HEADERS}
)

setup_target_defaults(${PROJECT_NAME})

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>")
endif()

# ============================================================
# Target 2: MLGIPipeline_plugin (DLL)
# ============================================================

set(MLGIPIPELINE_PLUGIN_NAME "${PROJECT_NAME}_plugin")

add_library(${MLGIPIPELINE_PLUGIN_NAME} SHARED
    src/CreateApplication.cpp
    ${MLGIPIPELINE_SOURCES}
    ${MLGIPIPELINE_HEADERS}
)

set_target_properties(${MLGIPIPELINE_PLUGIN_NAME} PROPERTIES
    OUTPUT_NAME "MLGIPipeline.Core"
    DEBUG_POSTFIX ""
)

setup_target_defaults(${MLGIPIPELINE_PLUGIN_NAME})
target_compile_definitions(${MLGIPIPELINE_PLUGIN_NAME} PRIVATE MLGIPIPELINE_PLUGIN_EXPORTS=1)


# ============================================================
# GLSL → SPIR-V 自动编译
# ============================================================

find_program(GLSLANG_VALIDATOR glslangValidator)
if(GLSLANG_VALIDATOR)
    # 常规着色器（vert/frag/comp）
    file(GLOB SHADER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.comp"
    )
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPV_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPV_OUTPUT}
            COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${SPV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME} → SPIR-V"
        )
        list(APPEND GENERATED_SPVS ${SPV_OUTPUT})
    endforeach()

    # RT 着色器（需要 SPIR-V 1.4+）
    file(GLOB RT_SHADER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.rgen"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.rchit"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.rmiss"
    )
    foreach(SHADER ${RT_SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPV_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPV_OUTPUT}
            COMMAND ${GLSLANG_VALIDATOR} --target-env vulkan1.3 -V ${SHADER} -o ${SPV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME} → SPIR-V (Vulkan 1.3)"
        )
        list(APPEND GENERATED_SPVS ${SPV_OUTPUT})
    endforeach()

    # 将 SPIR-V 编译目标添加为显式依赖，确保在复制之前编译
    add_custom_target(CompileShaders_MLGIPipeline DEPENDS ${GENERATED_SPVS})
    add_dependencies(${PROJECT_NAME} CompileShaders_MLGIPipeline AssetShaders)
    add_dependencies(${MLGIPIPELINE_PLUGIN_NAME} CompileShaders_MLGIPipeline AssetShaders)
    message(STATUS "GLSL→SPIR-V 编译已启用 (glslangValidator: ${GLSLANG_VALIDATOR})")
else()
    message(WARNING "glslangValidator 未找到 — 着色器将使用已有的 .spv 文件")
endif()

# ============================================================
# 复制项目资源
# ============================================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/MLGIPipeline.jsonc
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/MLGIPipeline.jsonc
    COMMENT "Copying shaders and project assets"
)

add_custom_command(TARGET ${MLGIPIPELINE_PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${MLGIPIPELINE_PLUGIN_NAME}>/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${MLGIPIPELINE_PLUGIN_NAME}>/assets
    COMMENT "Copying shaders and project assets (plugin)"
)

# ============================================================
# C# 脚本构建与部署
# ============================================================

if(PRISMA_ENABLE_SCRIPTING)
    set(CS_PROJ "${CMAKE_CURRENT_SOURCE_DIR}/scripts/GameScripts.csproj")

    if(EXISTS "${CS_PROJ}")
        add_dependencies(${PROJECT_NAME} Engine PrismaEngineHost)
        add_dependencies(${MLGIPIPELINE_PLUGIN_NAME} Engine)

        find_program(DOTNET dotnet REQUIRED)

        set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
        set(SCRIPTS_OUT "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Building C# game scripts..."
            COMMAND ${DOTNET} build "${CS_PROJ}"
                    -c Release --nologo
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SCRIPTS_DIR}/bin/Release/net10.0/MLGIPipeline.Scripts.dll"
                    "${SCRIPTS_OUT}/MLGIPipeline.Scripts.dll"
            COMMENT "Building C# scripts (runtime by PrismaEngine.Host)"
        )
    else()
        message(STATUS "MLGIPipeline: C# script project not found at ${CS_PROJ}, skipping.")
    endif()
else()
    message(STATUS "MLGIPipeline: Scripting disabled, skipping C# dependencies.")
endif()
