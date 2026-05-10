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
    src/main.cpp
    src/Template2DApp.cpp
)

set(TEST_HEADERS
    src/Template2DApp.h
)

# ========== 可执行文件 ==========

add_executable(${PROJECT_NAME}
    ${TEST_SOURCES}
    ${TEST_HEADERS}
)

# ========== 链接引擎 ==========

target_link_libraries(${PROJECT_NAME} PRIVATE
    Engine
    SDL3::SDL3
    glaze::glaze
)

# ========== 包含目录 ==========

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# ========== 编译选项 ==========

if(MSVC)
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            /W4
            /WX-
            /utf-8
    )
else()
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

# ========== 复制引擎着色器 ==========

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders
    COMMENT "Copying engine shaders to project output"
)

# ========== 复制项目资源 ==========

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying project assets to project output"
)

# ========== C# 脚本构建与部署 ==========

find_program(DOTNET dotnet REQUIRED)

# 运行时获取 dotnet 版本和 hostfxr 路径
execute_process(
    COMMAND ${DOTNET} --version
    OUTPUT_VARIABLE DOTNET_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(SCRIPTS_OUT "$<TARGET_FILE_DIR:${PROJECT_NAME}>/scripts")

# 配置时：在 host/fxr/ 下查找最高版本的 hostfxr.dll
set(HOSTFXR_BASE "C:/Program Files/dotnet/host/fxr")
file(GLOB HOSTFXR_VERSIONS RELATIVE "${HOSTFXR_BASE}" "${HOSTFXR_BASE}/*")
list(SORT HOSTFXR_VERSIONS COMPARE NATURAL)
list(REVERSE HOSTFXR_VERSIONS)  # 最高版本排第一
foreach(V ${HOSTFXR_VERSIONS})
    set(HOSTFXR_CANDIDATE "${HOSTFXR_BASE}/${V}/hostfxr.dll")
    if(EXISTS "${HOSTFXR_CANDIDATE}")
        set(HOSTFXR_DLL "${HOSTFXR_CANDIDATE}")
        break()
    endif()
endforeach()
if(HOSTFXR_DLL)
    message(STATUS "hostfxr.dll: ${HOSTFXR_DLL}")
endif()

# 构建时：自包含发布 C# 脚本 + 部署到输出目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${DOTNET} publish -c Release -r win-x64 --self-contained true
            -p:DebugType=none -p:DebugSymbols=false
            --nologo -v:q
            "${SCRIPTS_DIR}/GameScripts/GameScripts.csproj"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/win-x64/publish/"
            "${SCRIPTS_OUT}/"
    COMMENT "Building + publishing C# scripts (self-contained)"
)

# 手动触发：生产部署（自包含，无需依赖系统 .NET）
add_custom_target(DeployScripts
    COMMAND ${DOTNET} publish -c Release -r win-x64 --self-contained true
            -p:DebugType=none -p:DebugSymbols=false
            --nologo -v:q "${SCRIPTS_DIR}/GameScripts/GameScripts.csproj"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SCRIPTS_DIR}/GameScripts/bin/Release/net10.0/win-x64/publish/"
            "${SCRIPTS_OUT}/"
    COMMENT "Deploying C# scripts (self-contained, production)"
)

message(STATUS "Template2D configured")
