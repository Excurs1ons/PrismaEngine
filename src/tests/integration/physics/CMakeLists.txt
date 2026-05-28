# PrismaEngine Physics 集成测试
add_prisma_integration_test(test_physics
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/CollisionSystemTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RigidBodyTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ConstraintSolverTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CCDSolverTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TriggerManagerTest.cpp
)
