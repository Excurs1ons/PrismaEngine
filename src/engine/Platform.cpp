#include "Platform.h"

#include <chrono>
#include <thread>


float Time::DeltaTime = 0.0f;
float Time::TotalTime = 0.0f;
float Time::TimeScale = 1.0f;

float Time::GetTime() {
    using namespace std::chrono;
    static auto start = high_resolution_clock::now();
    auto now = high_resolution_clock::now();
    return duration<float>(now - start).count();
}

namespace Prisma {

void Platform::SleepMilliseconds(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

std::vector<const char*> Platform::GetVulkanInstanceExtensions() {
    return {};
}

bool Platform::CreateVulkanSurface(void* instance, WindowHandle windowHandle, void** outSurface) {
    (void)instance;
    (void)windowHandle;
    (void)outSurface;
    return false;
}

} // namespace Prisma
