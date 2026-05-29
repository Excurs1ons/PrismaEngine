# Render 系统集成测试
add_prisma_integration_test(test_render
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/render_pass_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bloom_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/pbr_material_test.cpp
)
