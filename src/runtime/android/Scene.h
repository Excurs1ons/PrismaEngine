#pragma once
#include "GameObject.h"
#include <memory>
#include <vector>
using namespace PrismaEngine;
class Scene {
public:
    void addGameObject(std::shared_ptr<GameObject> go) {
        gameObjects_.push_back(go);
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

private:
    std::vector<std::shared_ptr<GameObject>> gameObjects_;
};
