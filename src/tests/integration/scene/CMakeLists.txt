# Scene 系统集成测试
add_prisma_integration_test(test_scene
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/SceneTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SceneManagerTest.cpp
)
