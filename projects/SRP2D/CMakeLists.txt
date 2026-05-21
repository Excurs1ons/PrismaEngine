cmake_minimum_required(VERSION 3.20)
project(SRP2D VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引擎必须已构建
if(NOT TARGET Engine)
    message(FATAL_ERROR "SRP2D requires PrismaEngine to be built first. Add this project after the engine target.")
endif()

# ============================================================
# C# 脚本构建
# ============================================================

if(PRISMA_ENABLE_SCRIPTING)
    add_dependencies(${PROJECT_NAME} PrismaCore PrismaEngineHost)

    find_program(DOTNET dotnet REQUIRED)

    set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
    set(SCRIPTS_OUT "${CMAKE_BINARY_DIR}/bin")

    add_custom_target(SRP2D_Scripts ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Building SRP2D C# scripts..."
        COMMAND ${DOTNET} build "${SCRIPTS_DIR}/GameScripts/GameScripts.csproj"
                -c Release --nologo
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SCRIPTS_OUT}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/GameScripts.dll"
                "${SCRIPTS_OUT}/GameScripts.dll"
        COMMENT "SRP2D: Building C# scripts"
    )
endif()
