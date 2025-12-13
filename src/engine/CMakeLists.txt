# 引擎核心库

# 包含graphic子目录的CMakeLists.txt来定义变量
include(graphic/CMakeLists.txt)

# 收集源文件
set(ENGINE_SOURCES "")
set(ENGINE_HEADERS "")

# 添加core目录下的源文件（除了graphic目录）
file(GLOB_RECURSE CORE_SOURCES 
    "*.cpp"
)

list(FILTER CORE_SOURCES EXCLUDE REGEX "graphic/")

file(GLOB_RECURSE CORE_HEADERS
    "*.h"
)

list(FILTER CORE_HEADERS EXCLUDE REGEX "graphic/")

# 添加graphic目录下的源文件和头文件（使用在graphic/CMakeLists.txt中定义的变量）
set(ENGINE_SOURCES 
    ${CORE_SOURCES}
    ${ENGINE_GRAPHIC_SOURCES}
)

set(ENGINE_HEADERS
    ${CORE_HEADERS}
    ${ENGINE_GRAPHIC_HEADERS}
)

# 创建静态库
add_library(Engine STATIC 
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    packing/pipeline.h
)

# 设置包含目录
target_include_directories(Engine
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/resource
        ${CMAKE_CURRENT_SOURCE_DIR}/audio
        ${CMAKE_CURRENT_SOURCE_DIR}/graphic
        ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

# 设置编译定义
target_compile_definitions(Engine 
    PRIVATE ENGINE_EXPORTS
    PUBLIC _UNICODE UNICODE
)

# 设置编译选项
target_compile_options(Engine PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>
)

# 链接vcpkg管理的系统库
find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
#find_package(openfbx CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)

target_link_libraries(Engine PUBLIC
    SDL3::SDL3
    Vulkan::Vulkan
    nlohmann_json::nlohmann_json
    imgui::imgui
#    openfbx::openfbx
    Microsoft::DirectX-Headers
)

# 链接系统库
if(WIN32)
    target_link_libraries(Engine PRIVATE xaudio2
        d3d12
        dxgi
        d3dcompiler
    )
else (ANDROID)
    target_link_libraries(Engine PRIVATE
        log
    )

endif()

# 设置公共头文件
set_target_properties(Engine PROPERTIES
    PUBLIC_HEADER "${ENGINE_HEADERS}"
)

# 设置输出名称
set_target_properties(Engine PROPERTIES
    OUTPUT_NAME "PrismaEngine"
)

# 安装规则
install(TARGETS Engine
    EXPORT PrismaEngineTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/prisma/engine
)
