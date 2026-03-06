# PrismaEngine Projects CMake Configuration
cmake_minimum_required(VERSION 3.20)

message(STATUS "=== Loading Projects ===")

# 1. PrismaCraft 核心逻辑库
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/PrismaCraft/CMakeLists.txt)
    add_subdirectory(PrismaCraft)
    message(STATUS "  Added project: PrismaCraft")
endif()

# 2. 其他平台特定项目 (可选)
# if(WIN32 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/windows/Game/CMakeLists.txt)
#     add_subdirectory(windows/Game)
# endif()

message(STATUS "=========================")
