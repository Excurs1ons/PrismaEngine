# TestingDeps.cmake
# Google Test 依赖管理
# 仅在 PRISMA_BUILD_TESTING=ON 时被 include

# 使用和 FetchThirdPartyDeps.cmake 相同的离线优先策略:
# 1. 先检查 .dependencies/googletest-src/ 是否存在
# 2. 如果存在: FetchContent_Declare 使用 SOURCE_DIR 指向本地
# 3. 如果不存在: GIT_REPOSITORY + GIT_TAG (v1.15.2) + GIT_SHALLOW ON

include(FetchContent)

# 禁用 gtest 自身的测试和示例
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_hide_internal_symbols ON CACHE BOOL "" FORCE)

# 离线优先声明
set(GTEST_SOURCE_DIR "${PRISMA_GLOBAL_DEPS_DIR}/googletest-src")
if(EXISTS "${GTEST_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "  [SHARED]  googletest -> ${GTEST_SOURCE_DIR}")
    FetchContent_Declare(googletest
        SOURCE_DIR "${GTEST_SOURCE_DIR}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/googletest-build"
    )
else()
    message(STATUS "  [FETCH]   googletest <- https://github.com/google/googletest.git (v1.15.2)")
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW ON
        SOURCE_DIR "${GTEST_SOURCE_DIR}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/googletest-build"
    )
endif()

# 加载 googletest
FetchContent_MakeAvailable(googletest)

# 创建 ALIAS 目标以便 cmake 3.20+ 版本兼容
if(NOT TARGET GTest::GTest)
    add_library(GTest::GTest ALIAS gtest)
endif()
if(NOT TARGET GTest::Main)
    add_library(GTest::Main ALIAS gtest_main)
endif()
if(NOT TARGET GMock::GMock)
    add_library(GMock::GMock ALIAS gmock)
endif()

# 压制 gtest 自身 MSVC 警告 (参考 src/engine/CMakeLists.txt:714)
if(MSVC)
    foreach(_gtest_target gtest gtest_main gmock gmock_main)
        if(TARGET ${_gtest_target})
            target_compile_options(${_gtest_target} PRIVATE
                /wd4702 /wd4324 /wd4251 /wd4100 /wd4189 /wd4244
            )
        endif()
    endforeach()
endif()
