cmake_minimum_required(VERSION 3.20)
project(PrismaCraft VERSION 1.0.0 DESCRIPTION "PrismaCraft - C#-based Minecraft game with SRP" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PRISMACRAFT_SOURCES src/PrismaCraftApp.cpp)
set(PRISMACRAFT_HEADERS src/PrismaCraftApp.h)

set(PRISMACRAFT_INCLUDE_DIRS
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
    ${CMAKE_SOURCE_DIR}/src/engine/utils
)

macro(setup_target_defaults target)
    target_include_directories(${target} PRIVATE ${PRISMACRAFT_INCLUDE_DIRS})
    target_link_libraries(${target} PRIVATE Engine)
    if(NOT MSVC)
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endmacro()

# EXE
add_executable(${PROJECT_NAME} src/main.cpp ${PRISMACRAFT_SOURCES} ${PRISMACRAFT_HEADERS})
setup_target_defaults(${PROJECT_NAME})

# DLL Plugin
set(PLUGIN "${PROJECT_NAME}_plugin")
add_library(${PLUGIN} SHARED src/CreateApplication.cpp ${PRISMACRAFT_SOURCES} ${PRISMACRAFT_HEADERS})
set_target_properties(${PLUGIN} PROPERTIES OUTPUT_NAME "PrismaCraft" DEBUG_POSTFIX "")
setup_target_defaults(${PLUGIN})
target_compile_definitions(${PLUGIN} PRIVATE PRISMACRAFT_PLUGIN_EXPORTS=1)

# C# Scripts
if(PRISMA_ENABLE_SCRIPTING)
    add_dependencies(${PROJECT_NAME} PrismaCore)
    add_dependencies(${PLUGIN} PrismaCore)
    find_program(DOTNET dotnet REQUIRED)
    if(WIN32)
        set(CS_RID "win-x64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CS_RID "linux-arm64")
    else()
        set(CS_RID "linux-x64")
    endif()
    set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
    set(SCRIPTS_OUT "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Publishing C# scripts for RID: ${CS_RID}..."
        COMMAND ${DOTNET} publish -c Release -r ${CS_RID} --self-contained true
                -p:DebugType=portable -p:DebugSymbols=true
                --nologo "${SCRIPTS_DIR}/GameScripts/GameScripts.csproj"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/${CS_RID}/publish/" "${SCRIPTS_OUT}/"
        COMMENT "Building + publishing C# scripts (RID: ${CS_RID})"
    )
    message(STATUS "PrismaCraft configured for RID: ${CS_RID}")
endif()
