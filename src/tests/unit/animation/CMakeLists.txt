add_prisma_test(test_animation
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/SkeletonTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AnimationClipTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AnimStateMachineTest.cpp
    LINK_LIBS
        glm::glm-header-only
)
target_sources(test_animation PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/animation/Skeleton.cpp
)
