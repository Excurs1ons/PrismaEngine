#include "JobSystem.h"
namespace PrismaEngine {

bool JobSystem::Initialize() {
    return false;
}

void JobSystem::Shutdown() {}

void JobSystem::SubmitJob(Job job, uint32_t threadPoolIndex) {}

void JobSystem::WaitForAllJobs() {}

void JobSystem::ThreadPool::WorkerThread() {}
}  // namespace Engine