# EditorPostTargets.cmake
# 编辑器目标创建后的配置
# 此文件必须在 add_subdirectory(src/editor) 之后调用

if(PRISMA_BUILD_EDITOR AND TARGET Editor)
    message(STATUS "Editor: 应用目标后配置")

    # ========== 平台特定配置 ==========

    # Windows 编辑器配置
    if(WIN32)
        # Windows 子系统设置
        set(PRISMA_EDITOR_WINDOWS_SUBSYSTEM "CONSOLE" CACHE STRING "Windows 子系统")
        set_property(CACHE PRISMA_EDITOR_WINDOWS_SUBSYSTEM PROPERTY STRINGS "CONSOLE" "WINDOWS")

        # 注意: PrismaEditor 是可执行文件，Editor 是库
        if(TARGET PrismaEditor)
            if(PRISMA_EDITOR_WINDOWS_SUBSYSTEM STREQUAL "WINDOWS")
                target_link_options(PrismaEditor PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
            endif()

            # DPI 感知
            target_compile_definitions(PrismaEditor PRIVATE
                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2=1
            )
        endif()

        # Windows 崩溃转储
        option(PRISMA_EDITOR_ENABLE_MINIDUMP "启用编辑器崩溃转储" ON)
        if(PRISMA_EDITOR_ENABLE_MINIDUMP)
            if(TARGET PrismaEditor)
                target_compile_definitions(PrismaEditor PRIVATE
                    PRISMA_ENABLE_MINIDUMP=1
                )
                target_link_libraries(PrismaEditor PRIVATE
                    dbghelp.lib
                )
            endif()
        endif()
    endif()

    # Linux 编辑器配置
    if(LINUX)
        # Linux 桌面文件
        install(FILES
            resources/linux/prismaeditor.desktop
            DESTINATION share/applications
        )

        # Linux 图标
        install(FILES
            resources/linux/icons/prismaeditor.png
            DESTINATION share/icons/hicolor/256x256/apps
        )
    endif()

    # ========== 编辑器测试支持 ==========

    option(PRISMA_EDITOR_BUILD_TESTS "构建编辑器测试" OFF)
    if(PRISMA_EDITOR_BUILD_TESTS)
        enable_testing()

        # 测试目标
        add_custom_target(test-editor
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            DEPENDS PrismaEditor
            COMMENT "运行编辑器测试"
        )
    endif()

    # ========== 编辑器文档生成 ==========

    option(PRISMA_EDITOR_BUILD_DOCS "构建编辑器文档" OFF)
    if(PRISMA_EDITOR_BUILD_DOCS)
        find_program(DOXYGEN_EXECUTABLE doxygen)
        if(DOXYGEN_EXECUTABLE)
            add_custom_target(editor-docs
                COMMAND ${DOXYGEN_EXECUTABLE} docs/editor/Doxyfile
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "生成编辑器文档"
            )
        else()
            message(WARNING "Doxygen 未找到，跳过文档生成")
        endif()
    endif()

endif()
