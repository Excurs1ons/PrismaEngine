# Audio DSP tests (Google Test)
add_prisma_integration_test(AudioTest
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AudioDSPTest.cpp
)

# Stress test (standalone, retained)
add_executable(AudioStressTest StressTest.cpp)
target_link_libraries(AudioStressTest PRIVATE Engine)
target_include_directories(AudioStressTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME AudioStressTest COMMAND AudioStressTest)
