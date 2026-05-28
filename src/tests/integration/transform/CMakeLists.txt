# Transform 系统集成测试
# 注意: CameraController 未重写 GetComponentId() 纯虚函数，不可实例化，
# 其移动/旋转逻辑已通过 CameraTest 覆盖。
add_prisma_integration_test(test_transform
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/TransformTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CameraTest.cpp
)
