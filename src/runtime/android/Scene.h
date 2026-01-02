#pragma once
#include "GameObject.h"
#include "../../engine/graphic/ICamera.h"
#include <memory>
#include <vector>
using namespace PrismaEngine;

class Scene {
public:
    void addGameObject(std::shared_ptr<GameObject> go) {
        gameObjects_.push_back(go);

        // 自动查找并设置第一个 Camera 组件为主相机
        if (!mainCamera_) {
            auto camera = go->GetComponent<PrismaEngine::Graphic::ICamera>();
            if (camera) {
                mainCamera_ = camera;
            }
        }
    }

    const std::vector<std::shared_ptr<GameObject>>& getGameObjects() const {
        return gameObjects_;
    }

    /**
     * 更新场景中的所有游戏对象
     * 调用每个对象上的组件更新
     * @param deltaTime 时间增量（秒）
     */
    void update(float deltaTime) {
        for (auto& go : gameObjects_) {
            go->update(deltaTime);
        }
    }

    // 获取主相机
    std::shared_ptr<PrismaEngine::Graphic::ICamera> getMainCamera() const {
        return mainCamera_;
    }

    // 手动设置主相机
    void setMainCamera(std::shared_ptr<PrismaEngine::Graphic::ICamera> camera) {
        mainCamera_ = camera;
    }

private:
    std::vector<std::shared_ptr<GameObject>> gameObjects_;
    std::shared_ptr<PrismaEngine::Graphic::ICamera> mainCamera_;
};
