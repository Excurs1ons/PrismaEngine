# ProjectTargets.cmake
# 项目目标和资源复制配置

# 此文件必须在所有子目录添加后调用，因为它依赖于 Engine/Editor/Runtime/Game 目标

# ========== Prisma 聚合目标 ==========

# 创建一个聚合目标，确保 CLion 能正确识别项目
if(PRISMA_BUILD_EDITOR)
    add_custom_target(Prisma ALL
        DEPENDS Engine Editor Runtime Game
    )
else()
    add_custom_target(Prisma ALL
        DEPENDS Engine Runtime Game
    )
endif()

# ========== 资源复制目标 ==========

# 为 Runtime 复制资源
add_custom_target(copy-runtime-resources ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Copying resources to runtime directories..."
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Runtime>/Assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Runtime>/Assets
    COMMENT "Copying resources to runtime directories"
)

# 为 Editor 复制资源（如果 Editor 被构建）
if(PRISMA_BUILD_EDITOR)
    add_custom_target(copy-editor-resources ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Copying resources to Editor directory..."
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Editor>/Assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Editor>/Assets
        COMMENT "Copying resources to Editor directory"
    )
    add_dependencies(copy-runtime-resources copy-editor-resources)
endif()

# 确保在构建后复制资源
add_dependencies(copy-runtime-resources Prisma)

# ========== 清理目标 ==========

# 添加 clean-all 目标，清理所有生成的文件
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/install
    COMMENT "Cleaning all build artifacts"
)
