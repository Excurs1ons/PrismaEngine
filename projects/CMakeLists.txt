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

# 3. Scene2DTest (默认 2D 场景模板)
option(PRISMA_BUILD_PROJECT_SCENE2DTEST "Build Scene2DTest project" ON)
if(PRISMA_BUILD_PROJECT_SCENE2DTEST AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Scene2DTest/CMakeLists.txt)
    add_subdirectory(Scene2DTest)
    message(STATUS "  Added project: Scene2DTest")
elseif(NOT PRISMA_BUILD_PROJECT_SCENE2DTEST)
    message(STATUS "  Skipped project: Scene2DTest")
endif()

# 2. 其他平台特定项目 (可选)
# if(WIN32 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/windows/Game/CMakeLists.txt)
#     add_subdirectory(windows/Game)
# endif()

message(STATUS "=========================")
