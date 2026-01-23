# Utils.cmake
# 通用 CMake 工具函数

# 函数: 复制 DLL 到目标输出目录
# 用法: copy_dlls_to_target(TargetName "path/to/dll1" "path/to/dll2" ...)
function(copy_dlls_to_target TARGET_NAME)
    if(WIN32)
        # 获取剩余参数作为 DLL 列表
        set(DLL_PATHS ${ARGN})

        foreach(DLL_PATH ${DLL_PATHS})
            # 获取文件名
            get_filename_component(DLL_NAME ${DLL_PATH} NAME)

            # 添加构建后命令
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_PATH}"
                $<TARGET_FILE_DIR:${TARGET_NAME}>/${DLL_NAME}
                COMMENT "Copying ${DLL_NAME} to output directory..."
            )
        endforeach()
    endif()
endfunction()
