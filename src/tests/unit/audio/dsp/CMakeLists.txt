add_prisma_test(test_audio_dsp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AudioBufferTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AudioNodeTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AudioMathTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SpscQueueTest.cpp
)
