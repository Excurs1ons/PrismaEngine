set(Python3_EXECUTABLE
        "C:/Users/JasonGu/AppData/Local/Programs/Python/Python314/python.exe"
        CACHE PATH "Python 3 interpreter")
set(ENGINE_INCLUDE_PATH "F:/repos/GameEngine/include")
set(ENGINE_SOURCE_PATH "F:/repos/GameEngine/src")
# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)
# 1. 引入FetchContent模块
include(FetchContent)
# 使用 FetchContent 自动化管理依赖
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        main # 使用一个稳定的发布版本
)
FetchContent_Declare(
        SDL3_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG        main
)
FetchContent_MakeAvailable(SDL3 SDL3_ttf)

# Declares the project name.
project("myapplication")
# 包含 SDL3 子目录
# add_subdirectory(external/SDL3)
# add_subdirectory(external/SDL3_ttf)
# add_subdirectory(external/DiligentEngine)

# 使用 file(GLOB ...) 查找所有 src 目录下的 .cpp 文件
file(GLOB_RECURSE APP_SOURCES "src/*.cpp")

file(GLOB_RECURSE ENGINE_SOURCES ${ENGINE_SOURCE_PATH})
# 创建并命名库，包含 main.cpp 和上面找到的所有源文件
add_library(${CMAKE_PROJECT_NAME} SHARED
        main.cpp
        ${APP_SOURCES}
        # ${IMGUI_SOURCES} #imgui
        ${ENGINE_SOURCES}
)
# 将 ImGui 的核心文件和后端文件添加进来
#set(IMGUI_SOURCES
 #       ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
  #      ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
   #     ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    #    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
     #   # ImGui 的 SDL3 平台后端
      #  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_sdl3.cpp
       # # ImGui 的 SDL3 Renderer 渲染后端
        #${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_sdlrenderer3.cpp
#)
# 设置包含目录
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE include
        # ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
        # ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
        ${ENGINE_INCLUDE_PATH}
)
if(ANDROID)
    # 定义对齐大小
    set(ALIGNMENT_SIZE 16384)
    # 设置链接器选项以实现16KB对齐
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,-z,max-page-size=${ALIGNMENT_SIZE}")
endif()
if(ANDROID)
    # 链接库
    target_link_libraries(${CMAKE_PROJECT_NAME}
            android
            SDL3::SDL3
            SDL3_ttf::SDL3_ttf
            log
    )
else ()
    # 链接库
    target_link_libraries(${CMAKE_PROJECT_NAME}
            SDL3::SDL3
            SDL3_ttf::SDL3_ttf
    )
endif()
