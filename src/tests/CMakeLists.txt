# PrismaEngine 测试 CMakeLists.txt
# 仅在 PRISMA_BUILD_TESTING=ON 时被构建

# ========== 纯逻辑测试宏 ==========
# 直接编译源文件，不链接 Engine，无 Vulkan 依赖
macro(add_prisma_test TARGET_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES LINK_LIBS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${TARGET_NAME} ${ARG_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE
        GTest::GTest
        GTest::Main
        ${ARG_LINK_LIBS}
    )
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src/engine/core
        ${CMAKE_SOURCE_DIR}/src/engine/math
        ${CMAKE_SOURCE_DIR}/src/engine/memory
        ${CMAKE_SOURCE_DIR}/src/engine/logger
        ${CMAKE_SOURCE_DIR}/.dependencies/glm-src
    )
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endmacro()

# ========== 集成测试宏 ==========
# 链接 Engine 共享库，需要 Vulkan SDK
macro(add_prisma_integration_test TARGET_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES LINK_LIBS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${TARGET_NAME} ${ARG_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE
        Engine
        GTest::GTest
        GTest::Main
        ${ARG_LINK_LIBS}
    )
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/engine
    )
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endmacro()

# ========== 基础验证测试 ==========
add_prisma_test(test_sanity
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/SanityTest.cpp
)

# ========== AudioTest (保留，Task 3 将迁移) ==========
add_subdirectory(AudioTest)

# ========== 纯逻辑测试 ==========
add_subdirectory(unit)
