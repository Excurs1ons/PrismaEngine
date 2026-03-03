# SDKConfig.cmake
# SDK 配置和导出
# 此文件定义 SDK 打包和导出配置

# ========== SDK 选项 ==========

option(PRISMA_BUILD_SDK "构建 SDK" OFF)
option(PRISMA_SDK_INCLUDE_SAMPLES "SDK 包含示例项目" ON)
option(PRISMA_SDK_INCLUDE_DOCS "SDK 包含文档" ON)
option(PRISMA_SDK_INCLUDE_SRC "SDK 包含源代码" OFF)

# ========== SDK 版本配置 ==========

set(PRISMA_SDK_VERSION ${PRISMA_ENGINE_VERSION})
set(PRISMA_SDK_VERSION_MAJOR ${PRISMA_ENGINE_VERSION_MAJOR})
set(PRISMA_SDK_VERSION_MINOR ${PRISMA_ENGINE_VERSION_MINOR})
set(PRISMA_SDK_VERSION_PATCH ${PRISMA_ENGINE_VERSION_PATCH})

# ========== SDK 目录结构 ==========

set(PRISMA_SDK_DIR "${CMAKE_BINARY_DIR}/sdk" CACHE PATH "SDK 输出目录")
set(PRISMA_SDK_INCLUDE_DIR "${PRISMA_SDK_DIR}/include")
set(PRISMA_SDK_LIB_DIR "${PRISMA_SDK_DIR}/lib")
set(PRISMA_SDK_BIN_DIR "${PRISMA_SDK_DIR}/bin")
set(PRISMA_SDK_CMAKE_DIR "${PRISMA_SDK_DIR}/cmake")
set(PRISMA_SDK_SAMPLES_DIR "${PRISMA_SDK_DIR}/samples")
set(PRISMA_SDK_DOCS_DIR "${PRISMA_SDK_DIR}/docs")
set(PRISMA_SDK_SHADERS_DIR "${PRISMA_SDK_DIR}/shaders")

# ========== SDK 打包目标 ==========

if(PRISMA_BUILD_SDK)
    message(STATUS "SDK: 构建已启用 (版本: ${PRISMA_SDK_VERSION})")

    # 创建 SDK 目录
    file(MAKE_DIRECTORY ${PRISMA_SDK_INCLUDE_DIR})
    file(MAKE_DIRECTORY ${PRISMA_SDK_LIB_DIR})
    file(MAKE_DIRECTORY ${PRISMA_SDK_BIN_DIR})
    file(MAKE_DIRECTORY ${PRISMA_SDK_CMAKE_DIR})
    file(MAKE_DIRECTORY ${PRISMA_SDK_SHADERS_DIR})

    if(PRISMA_SDK_INCLUDE_SAMPLES)
        file(MAKE_DIRECTORY ${PRISMA_SDK_SAMPLES_DIR})
    endif()

    if(PRISMA_SDK_INCLUDE_DOCS)
        file(MAKE_DIRECTORY ${PRISMA_SDK_DOCS_DIR})
    endif()

    # SDK 打包目标
    add_custom_target(sdk-package
        COMMAND ${CMAKE_COMMAND} -E echo "正在打包 PrismaEngine SDK ${PRISMA_SDK_VERSION}..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PRISMA_SDK_DIR}
        DEPENDS Engine
        COMMENT "打包 PrismaEngine SDK"
    )

    # SDK 组件
    set(PRISMA_SDK_COMPONENTS
        "Headers"       # 头文件
        "Libraries"     # 库文件
        "Shaders"       # 着色器
        "CMake"         # CMake 配置
    )

    if(PRISMA_SDK_INCLUDE_SAMPLES)
        list(APPEND PRISMA_SDK_COMPONENTS "Samples")
    endif()

    if(PRISMA_SDK_INCLUDE_DOCS)
        list(APPEND PRISMA_SDK_COMPONENTS "Docs")
    endif()

    if(PRISMA_SDK_INCLUDE_SRC)
        list(APPEND PRISMA_SDK_COMPONENTS "Source")
    endif()

    message(STATUS "SDK 组件: ${PRISMA_SDK_COMPONENTS}")

else()
    message(STATUS "SDK: 构建已禁用")
endif()

# ========== SDK 头文件复制 ==========

if(PRISMA_BUILD_SDK)
    add_custom_command(TARGET sdk-package POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "复制头文件..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/engine
            ${PRISMA_SDK_INCLUDE_DIR}/PrismaEngine
        COMMAND ${CMAKE_COMMAND} -E echo "  - ECS 核心系统"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/engine/core
            ${PRISMA_SDK_INCLUDE_DIR}/PrismaEngine/core
        COMMAND ${CMAKE_COMMAND} -E echo "  - 物理系统"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/engine/physics
            ${PRISMA_SDK_INCLUDE_DIR}/PrismaEngine/physics
        COMMAND ${CMAKE_COMMAND} -E echo "  - 渲染接口"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/engine/graphic/interfaces
            ${PRISMA_SDK_INCLUDE_DIR}/PrismaEngine/graphic/interfaces
        COMMENT "复制 SDK 头文件"
    )

    # 清理不需要的头文件
    add_custom_command(TARGET sdk-package POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "清理不需要的头文件..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            ${PRISMA_SDK_INCLUDE_DIR}/PrismaEngine/graphic/adapters
        COMMENT "清理 SDK 头文件"
    )
endif()

# ========== SDK 库文件复制 ==========

if(PRISMA_BUILD_SDK)
    add_custom_command(TARGET sdk-package POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "复制库文件..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Engine>
            ${PRISMA_SDK_LIB_DIR}/
        COMMENT "复制 SDK 库文件"
    )
endif()

# ========== SDK 着色器复制 ==========

if(PRISMA_BUILD_SDK)
    add_custom_command(TARGET sdk-package POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "复制着色器..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources/common/shaders
            ${PRISMA_SDK_SHADERS_DIR}
        COMMENT "复制 SDK 着色器"
    )
endif()

# ========== SDK CMake 配置生成 ==========

if(PRISMA_BUILD_SDK)
    # 配置 PrismaEngineConfig.cmake
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/PrismaEngineConfig.cmake.in
        ${PRISMA_SDK_CMAKE_DIR}/PrismaEngineConfig.cmake
        @ONLY
    )

    # 配置版本文件
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/PrismaEngineConfigVersion.cmake.in
        ${PRISMA_SDK_CMAKE_DIR}/PrismaEngineConfigVersion.cmake
        @ONLY
    )

    add_custom_command(TARGET sdk-package POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "生成 CMake 配置..."
        COMMENT "生成 SDK CMake 配置"
    )
endif()

# ========== SDK 示例项目 ==========

if(PRISMA_BUILD_SDK AND PRISMA_SDK_INCLUDE_SAMPLES)
    # 基础示例项目
    set(PRISMA_SDK_SAMPLES
        "BasicTriangle"      # 基础三角形
        "CubeGame"           # 立方体游戏
        "BlockGame"          # 方块游戏示例
        "PrismaCraftStarter" # PrismaCraft 启动示例
    )

    foreach(SAMPLE ${PRISMA_SDK_SAMPLES})
        add_custom_command(TARGET sdk-package POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "复制示例: ${SAMPLE}..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/sdk/samples/${SAMPLE}
                ${PRISMA_SDK_SAMPLES_DIR}/${SAMPLE}
            COMMENT "复制 SDK 示例: ${SAMPLE}"
        )
    endforeach()
endif()

# ========== SDK 文档 ==========

if(PRISMA_BUILD_SDK AND PRISMA_SDK_INCLUDE_DOCS)
    # 复制文档
    add_custom_command(TARGET sdk-package POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "复制文档..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/docs
            ${PRISMA_SDK_DOCS_DIR}
        COMMENT "复制 SDK 文档"
    )

    # 生成 API 文档（如果 Doxygen 可用）
    find_program(DOXYGEN_EXECUTABLE doxygen)
    if(DOXYGEN_EXECUTABLE)
        add_custom_command(TARGET sdk-package POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "生成 API 文档..."
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "生成 SDK API 文档"
        )
    endif()
endif()

# ========== SDK 源代码（可选）==========

if(PRISMA_BUILD_SDK AND PRISMA_SDK_INCLUDE_SRC)
    add_custom_command(TARGET sdk-package POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "复制源代码..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/engine
            ${PRISMA_SDK_DIR}/src/engine
        COMMENT "复制 SDK 源代码"
    )
endif()

# ========== SDK 打包（压缩）==========

if(PRISMA_BUILD_SDK)
    # 根据平台选择打包格式
    if(WIN32)
        set(SDK_ARCHIVE_FORMAT "ZIP")
        set(SDK_ARCHIVE_FILE "${CMAKE_BINARY_DIR}/PrismaEngine-SDK-${PRISMA_SDK_VERSION}-win64.zip")
    elseif(APPLE)
        set(SDK_ARCHIVE_FORMAT "TGZ")
        set(SDK_ARCHIVE_FILE "${CMAKE_BINARY_DIR}/PrismaEngine-SDK-${PRISMA_SDK_VERSION}-macos.tar.gz")
    else()
        set(SDK_ARCHIVE_FORMAT "TGZ")
        set(SDK_ARCHIVE_FILE "${CMAKE_BINARY_DIR}/PrismaEngine-SDK-${PRISMA_SDK_VERSION}-linux.tar.gz")
    endif()

    # 创建打包目标
    add_custom_target(sdk-archive
        COMMAND ${CMAKE_COMMAND} -E echo "正在创建 SDK 归档..."
        COMMAND ${CMAKE_COMMAND} -E tar cfv ${SDK_ARCHIVE_FILE}
            --format=${SDK_ARCHIVE_FORMAT}
            --files-from=/dev/null  # Placeholder
        WORKING_DIRECTORY ${PRISMA_SDK_DIR}
        DEPENDS sdk-package
        COMMENT "创建 SDK 归档文件"
    )

    # 使用 CPack 打包（更好的跨平台支持）
    add_custom_target(sdk-cpack
        COMMAND ${CMAKE_COMMAND} -E echo "使用 CPack 打包 SDK..."
        COMMAND ${CMAKE_COMMAND} --build . --target package
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS sdk-package
        COMMENT "使用 CPack 打包 SDK"
    )
endif()

# ========== SDK 安装规则 ==========

if(PRISMA_BUILD_SDK)
    # 安装 SDK 到指定位置
    install(DIRECTORY ${PRISMA_SDK_DIR}/
        DESTINATION sdk
        USE_SOURCE_PERMISSIONS
    )

    # 创建 SDK 脚本
    install(FILES
        ${CMAKE_SOURCE_DIR}/scripts/package-sdk.sh
        DESTINATION sdk
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()
