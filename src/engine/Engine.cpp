#include "Engine.h"
#include "core/AssetManager.h"
#include "Logger.h"
#include "PhysicsSystem.h"
#include "Platform.h"
#include "graphic/RenderSystem.h"
#include "SceneManager.h"
#include "ThreadManager.h"
#include "DebugOverlay.h"

#if PRISMA_ENABLE_IMGUI_DEBUG
#include "imgui.h"
#endif

namespace PrismaEngine {
    EngineCore::EngineCore() : isRunning_(false) {
        if (!Logger::GetInstance().IsInitialized()) {
            Logger::GetInstance().Initialize();
        }
    }

    bool EngineCore::Initialize() {
        LOG_INFO("Engine", "Engine initialization started...");

        // Singleton::GetInstance() returns std::shared_ptr<T>
        if (!RegisterSystem(ThreadManager::GetInstance().get())) return false;
        if (!RegisterSystem(PhysicsSystem::GetInstance().get())) return false;
        if (!RegisterSystem(Graphic::RenderSystem::GetInstance().get())) return false;
        if (!RegisterSystem(SceneManager::GetInstance().get())) return false;

        LOG_INFO("Engine", "Core systems registered successfully.");
        m_initialized = true;
        return true;
    }

    bool EngineCore::RegisterSystem(ISubSystem* system) {
        if (!system) return false;
        m_systems.push_back(system);
        return system->Initialize();
    }

    void EngineCore::Shutdown() {
        LOG_INFO("Engine", "Engine shutting down...");
        m_initialized = false;
        isRunning_ = false;
    }

    int EngineCore::Run() {
        if (!m_initialized) return -1;
        isRunning_ = true;
        LOG_INFO("Engine", "Engine loop started.");
        while (isRunning_) {
            Update();
        }
        return 0;
    }

    void EngineCore::Update() {
        Platform::Update();
        float deltaTime = 0.016f;
        for (ISubSystem* system : m_systems) {
            system->Update(deltaTime);
        }
    }

} // namespace PrismaEngine