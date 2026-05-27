add_executable(AudioTest main.cpp)
target_link_libraries(AudioTest PRIVATE Engine)
target_include_directories(AudioTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Stress test
add_executable(AudioStressTest StressTest.cpp)
target_link_libraries(AudioStressTest PRIVATE Engine)
target_include_directories(AudioStressTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME AudioStressTest COMMAND AudioStressTest)

# Deterministic test
add_executable(AudioDeterministicTest DeterministicTest.cpp)
target_link_libraries(AudioDeterministicTest PRIVATE Engine)
target_include_directories(AudioDeterministicTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME AudioDeterministicTest COMMAND AudioDeterministicTest)
