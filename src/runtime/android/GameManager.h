#pragma once

#include "Scene.h"
#include "GameObject.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>
#include <game-activity/native_app_glue/android_native_app_glue.h>

// 前向声明
struct VulkanContext;

namespace PrismaEngine {

/**
 * GameManager - 游戏逻辑和状态管理器
 *
 * 生命周期：独立于渲染窗口存在
 * - APP_CMD_INIT_WINDOW: 重建渲染器，但 GameManager 保持不变
 * - APP_CMD_TERM_WINDOW: 销毁渲染器，但 GameManager 保留游戏状态
 *
 * 职责：
 * 1. 持有场景和游戏对象（持久化）
 * 2. 管理游戏状态（位置、旋转、速度等）
 * 3. 提供场景创建和更新接口
 */
class GameManager {
public:
    static GameManager& GetInstance();

    // 初始化游戏（只调用一次）
    void Initialize(android_app* pApp);

    // 创建场景（仅首次调用时创建，包含 GameObject 但不包含渲染资源）
    void CreateScene();

    // 为场景中的对象创建渲染资源（需要 Vulkan 上下文，在窗口初始化时调用）
    void SetupRenderingResources(void* vulkanContext);

    // 获取场景
    std::shared_ptr<Scene> GetScene() { return scene_; }

    // 更新游戏逻辑
    void Update(float deltaTime);

    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }

    // 检查场景是否已创建
    bool IsSceneCreated() const { return sceneCreated_; }

    // 检查渲染资源是否已设置
    bool IsRenderingSetup() const { return renderingSetup_; }

    // 设置渲染资源状态
    void SetRenderingSetup(bool setup) { renderingSetup_ = setup; }

    // 设置场景（供外部设置 Scene）
    void SetScene(const std::shared_ptr<Scene>& scene) { scene_ = scene; }

    // 获取 AssetManager
    AAssetManager* GetAssetManager() { return assetManager_; }

private:
    GameManager() = default;
    ~GameManager() = default;
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    std::shared_ptr<Scene> scene_;
    AAssetManager* assetManager_ = nullptr;
    bool initialized_ = false;
    bool sceneCreated_ = false;
    bool renderingSetup_ = false;
};

} // namespace PrismaEngine
