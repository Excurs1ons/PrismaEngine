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

    std::shared_ptr<EngineCore> EngineCore::GetInstance() {
        static std::shared_ptr<EngineCore> instance = std::make_shared<EngineCore>();
        return instance;
    }

    EngineCore::EngineCore() : isRunning_(false), m_initialized(false) {
        if (!Logger::GetInstance().IsInitialized()) {
            Logger::GetInstance().Initialize();
        }
    }

    int EngineCore::Initialize() {
        LOG_INFO("Engine", "Engine initialization started...");

        if (!RegisterSystem(ThreadManager::GetInstance().get())) {
            LOG_ERROR("Engine", "Failed to initialize ThreadManager");
            return -1;
        }
        if (!RegisterSystem(PhysicsSystem::GetInstance().get())) {
            LOG_ERROR("Engine", "Failed to initialize PhysicsSystem");
            return -1;
        }
        if (!RegisterSystem(::PrismaEngine::Graphic::RenderSystem::GetInstance().get())) {
            LOG_ERROR("Engine", "Failed to initialize RenderSystem");
            return -1;
        }
        if (!RegisterSystem(SceneManager::GetInstance().get())) {
            LOG_ERROR("Engine", "Failed to initialize SceneManager");
            return -1;
        }
        LOG_INFO("Engine", "Core systems registered successfully.");
        m_initialized = true;
        return 0;
    }

    bool EngineCore::RegisterSystem(ISubSystem* system) {
        if (!system) return false;
        m_systems.push_back(system);
        return system->Initialize();
    }

    void EngineCore::Shutdown() {
        LOG_INFO("Engine", "Engine shutting down...");
        for (auto it = m_systems.rbegin(); it != m_systems.rend(); ++it) {
            (*it)->Shutdown();
        }
        m_systems.clear();
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
