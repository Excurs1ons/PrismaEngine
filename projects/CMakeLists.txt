# PrismaEngine Projects CMake Configuration
cmake_minimum_required(VERSION 3.20)

message(STATUS "=== Loading Projects ===")

option(PRISMA_BUILD_PROJECT_PRISMACRAFT "Build PrismaCraft project" ON)
option(PRISMA_BUILD_PROJECT_PACMAN "Build PacManGame project" ON)

# 1. PrismaCraft 核心逻辑库
if(PRISMA_BUILD_PROJECT_PRISMACRAFT AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/PrismaCraft/CMakeLists.txt)
    add_subdirectory(PrismaCraft)
    message(STATUS "  Added project: PrismaCraft")
elseif(NOT PRISMA_BUILD_PROJECT_PRISMACRAFT)
    message(STATUS "  Skipped project: PrismaCraft")
endif()

# 2. PacManGame
if(PRISMA_BUILD_PROJECT_PACMAN AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/PacManGame/CMakeLists.txt)
    add_subdirectory(PacManGame)
    message(STATUS "  Added project: PacManGame")
elseif(NOT PRISMA_BUILD_PROJECT_PACMAN)
    message(STATUS "  Skipped project: PacManGame")
endif()

# 3. Template2D (默认 2D 场景模板)
option(PRISMA_BUILD_PROJECT_TEMPLATE2D "Build Template2D project" ON)
if(PRISMA_BUILD_PROJECT_TEMPLATE2D AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Template2D/CMakeLists.txt)
    add_subdirectory(Template2D)
    message(STATUS "  Added project: Template2D")
elseif(NOT PRISMA_BUILD_PROJECT_TEMPLATE2D)
    message(STATUS "  Skipped project: Template2D")
endif()

# 4. Template3D (3D 路径追踪模板)
option(PRISMA_BUILD_PROJECT_TEMPLATE3D "Build Template3D project" ON)
if(PRISMA_BUILD_PROJECT_TEMPLATE3D AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Template3D/CMakeLists.txt)
    add_subdirectory(Template3D)
    message(STATUS "  Added project: Template3D")
elseif(NOT PRISMA_BUILD_PROJECT_TEMPLATE3D)
    message(STATUS "  Skipped project: Template3D")
endif()

# 5. ClusteredForward3D (分块前向渲染)
option(PRISMA_BUILD_PROJECT_CLUSTEREDFORWARD3D "Build ClusteredForward3D project" ON)
if(PRISMA_BUILD_PROJECT_CLUSTEREDFORWARD3D AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ClusteredForward3D/CMakeLists.txt)
    add_subdirectory(ClusteredForward3D)
    message(STATUS "  Added project: ClusteredForward3D")
elseif(NOT PRISMA_BUILD_PROJECT_CLUSTEREDFORWARD3D)
    message(STATUS "  Skipped project: ClusteredForward3D")
endif()

# 6. Deferred3D (延迟渲染调试项目)
option(PRISMA_BUILD_PROJECT_DEFERRED3D "Build Deferred3D project" ON)
if(PRISMA_BUILD_PROJECT_DEFERRED3D AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Deferred3D/CMakeLists.txt)
    add_subdirectory(Deferred3D)
    message(STATUS "  Added project: Deferred3D")
elseif(NOT PRISMA_BUILD_PROJECT_DEFERRED3D)
    message(STATUS "  Skipped project: Deferred3D")
endif()

# 未激活的项目（保留供未来开发）:
# - SRP2D/  — C# SRP 2D 渲染测试项目 (C# only, 无 C++ 目标)
# if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/SRP2D/CMakeLists.txt)
#     add_subdirectory(SRP2D)
# endif()

message(STATUS "=========================")
