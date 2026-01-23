# 运行时可执行程序
if(ANDROID)
    # 源文件
    set(RUNTIME_SOURCES android/AndroidRuntime.cpp)
elseif (WIN32)
    # 源文件
    set(RUNTIME_SOURCES windows/WindowsRuntime.cpp)
    set(RUNTIME_RESOURCES
            resources/windows/icons/Runtime.ico
            resources/windows/icons/small.ico)
elseif (APPLE)
    message(FATAL_ERROR "尚不支持此平台:APPLE")
elseif (LINUX)
    set(RUNTIME_SOURCES linux/LinuxRuntime.cpp)
endif ()

# 创建可执行程序
add_executable(Runtime
    ${RUNTIME_SOURCES}
#    ${RUNTIME_RESOURCES}
)

# 包含目录
#target_include_directories(Runtime
#    PRIVATE
#        ${CMAKE_CURRENT_SOURCE_DIR}/../engine
#)
# 需要在子目录
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../engine)
# 链接依赖
target_link_libraries(Runtime
    PRIVATE Engine
)

# 根据 PRISMA_BUILD_SHARED_LIBS 决定是否静态链接 Game
if(PRISMA_BUILD_SHARED_LIBS)
    # 动态库模式: 不链接 Game，运行时动态加载
    message(STATUS "Runtime: 动态库模式 - 运行时动态加载Game")
else()
    # 静态库模式: 静态链接 Game (和 Editor)
    target_link_libraries(Runtime PRIVATE Game)
    target_compile_definitions(Runtime PRIVATE PRISMA_STATIC_LINKED_GAME=1)
    message(STATUS "Runtime: 静态库模式 - 静态链接Game")
endif()

# 编译选项
target_compile_options(Runtime PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 编译定义
target_compile_definitions(Runtime
    PRIVATE _UNICODE UNICODE
)

# 设置输出名称
set_target_properties(Runtime PROPERTIES
    OUTPUT_NAME "PrismaRuntime"
)

# ========== 自动复制 DLL 配置 ==========
# 使用 cmake/Utils.cmake 中定义的 copy_dlls_to_target 函数
# 将指定的 DLL 复制到可执行文件生成目录 (bin/Debug 或 bin/Release)

# 如果你有特定的 DLL 需要拷贝，请写在这里：
if(WIN32)
    set(DLLS_TO_COPY "")

    # 检查 Game 目标是否存在
    if(TARGET Game)
        list(APPEND DLLS_TO_COPY $<TARGET_FILE:Game>)
    endif()

    # 检查 Editor 目标是否存在
    if(TARGET Editor)
        list(APPEND DLLS_TO_COPY $<TARGET_FILE:Editor>)
    endif()

    if(DLLS_TO_COPY)
        copy_dlls_to_target(Runtime ${DLLS_TO_COPY})
    endif()
endif()

# 安装规则
#install(TARGETS Runtime
#    RUNTIME DESTINATION bin
#)