#include "Game.h"
#include <PrismaEngine/Engine.h>
#include <PrismaEngine/input/InputManager.h>
#include <PrismaEngine/graphic/RenderSystemNew.h>
#include <glm/glm.hpp>

namespace BlockGame {

Game::Game()
    : m_running(false)
    , m_window(nullptr)
    , m_renderDevice(nullptr)
    , m_player(nullptr)
    , m_world(nullptr) {
}

Game::~Game() {
    Shutdown();
}

bool Game::Initialize() {
    LOG_INFO("Game", "初始化游戏系统");

    // 创建窗口
    m_window = CreateWindow(1280, 720, "BlockGame - PrismaEngine 示例");
    if (!m_window) {
        LOG_ERROR("Game", "窗口创建失败");
        return false;
    }

    // 初始化渲染设备
    m_renderDevice = CreateRenderDevice();
    if (!m_renderDevice) {
        LOG_ERROR("Game", "渲染设备创建失败");
        return false;
    }

    // 初始化输入系统
    auto* inputManager = Engine::GetInputManager();
    if (!inputManager) {
        LOG_ERROR("Game", "输入管理器未初始化");
        return false;
    }

    // 创建世界
    m_world = std::make_unique<World>();
    m_world->GenerateTerrain();

    // 创建玩家
    m_player = std::make_unique<Player>(m_world.get());
    m_player->SetPosition(glm::vec3(0, 65, 0)); // 生成在地面上方

    LOG_INFO("Game", "游戏系统初始化完成");
    return true;
}

int Game::Run() {
    LOG_INFO("Game", "启动游戏主循环");

    m_running = true;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_running) {
        // 计算增量时间
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        float dt = deltaTime.count();

        // 处理输入
        HandleInput();

        // 更新游戏逻辑
        Update(dt);

        // 渲染
        Render();

        // 更新窗口
        if (m_window) {
            m_window->Update();
            m_running = !m_window->ShouldClose();
        }
    }

    LOG_INFO("Game", "游戏主循环结束");
    return 0;
}

void Game::Shutdown() {
    LOG_INFO("Game", "关闭游戏系统");

    m_player.reset();
    m_world.reset();

    if (m_renderDevice) {
        delete m_renderDevice;
        m_renderDevice = nullptr;
    }

    if (m_window) {
        delete m_window;
        m_window = nullptr;
    }

    LOG_INFO("Game", "游戏系统已关闭");
}

void Game::HandleInput() {
    auto* inputManager = Engine::GetInputManager();
    if (!inputManager) return;

    // 检查退出键
    if (inputManager->IsKeyDown(KeyCode::Escape)) {
        m_running = false;
        return;
    }

    // 更新玩家输入
    if (m_player) {
        m_player->HandleInput(inputManager);
    }
}

void Game::Update(float deltaTime) {
    // 更新玩家
    if (m_player) {
        m_player->Update(deltaTime);
    }

    // 更新世界
    if (m_world) {
        m_world->Update(deltaTime);
    }
}

void Game::Render() {
    if (!m_renderDevice) return;

    // 清屏
    m_renderDevice->Clear();

    // TODO: 渲染世界和玩家

    // 呈现
    m_renderDevice->Present();
}

} // namespace BlockGame
