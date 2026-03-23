#include "Platform.h"
#include <chrono>
#include <thread>

namespace Prisma {

struct Time {
    static float DeltaTime;
    static float TotalTime;
    static float TimeScale;
    static float GetTime();
};

float Time::DeltaTime = 0.0f;
float Time::TotalTime = 0.0f;
float Time::TimeScale = 1.0f;

float Time::GetTime() {
    using namespace std::chrono;
    static auto start = high_resolution_clock::now();
    auto now = high_resolution_clock::now();
    return duration<float>(now - start).count();
}

void Platform::SleepMilliseconds(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// NOTE: Platform-specific implementations are in PlatformWindows.cpp or PlatformLinux.cpp

} // namespace Prisma
