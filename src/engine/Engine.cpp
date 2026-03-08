#include "Engine.h"
#include "core/AssetManager.h"
#include "Logger.h"
#include "PhysicsSystem.h"
#include "Platform.h"
#include "graphic/RenderSystem.h"
#include "SceneManager.h"
#include "ThreadManager.h"
#include "DebugOverlay.h"

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

        // 1. 注册核心基础系统 (0 为成功)
        if (ThreadManager::GetInstance()->Initialize() != 0) {
            LOG_ERROR("Engine", "Failed to initialize ThreadManager");
            return -1;
        }
        m_systems.push_back(ThreadManager::GetInstance().get());

        if (PhysicsSystem::GetInstance()->Initialize() != 0) {
            LOG_ERROR("Engine", "Failed to initialize PhysicsSystem");
            return -1;
        }
        m_systems.push_back(PhysicsSystem::GetInstance().get());

        // 注意：RenderSystem 的初始化由编辑器手动调用带参数版本，这里只注册不初始化
        m_systems.push_back(::PrismaEngine::Graphic::RenderSystem::GetInstance().get());

        if (SceneManager::GetInstance()->Initialize() != 0) {
            LOG_ERROR("Engine", "Failed to initialize SceneManager");
            return -1;
        }
        m_systems.push_back(SceneManager::GetInstance().get());

        LOG_INFO("Engine", "Core systems registered successfully.");
        m_initialized = true;
        return 0;
    }

    bool EngineCore::RegisterSystem(ISubSystem* system) {
        if (!system) return false;
        m_systems.push_back(system);
        return system->Initialize() == 0;
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
        // 更新逻辑 (deltaTime 暂定)
        float deltaTime = 0.016f;
        for (ISubSystem* system : m_systems) {
            system->Update(deltaTime);
        }
    }

} // namespace PrismaEngine
