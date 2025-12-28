#pragma once
#include <memory>

// 前向声明
namespace PrismaEngine::Graphic {
    class RenderCommandContext;
}

class SceneNode {
public:
    // 添加子节点
    void AddChild(std::shared_ptr<SceneNode> child) {
        child->m_parent = this;
        m_children.push_back(child);
    }
    // 移除子节点
    void RemoveChild(SceneNode* child) {
        m_children.erase(
            std::remove_if(m_children.begin(),
                           m_children.end(),
                           [child](const std::shared_ptr<SceneNode>& node) { return node.get() == child; }),
            m_children.end());
    }

    // 更新节点（递归调用子节点）
    virtual void Update(float deltaTime) {
        for (auto& child : m_children) {
            child->Update(deltaTime);
        }
    }

    // 渲染节点（递归调用子节点）
    virtual void Render(PrismaEngine::Graphic::RenderCommandContext* context) {
        // 应用当前节点的变换
        PushTransform();

        // 渲染当前节点内容
        OnRender(context);

        // 渲染所有子节点
        for (auto& child : m_children) {
            child->Render(context);
        }

        // 恢复变换
        PopTransform();
    }

protected:
    virtual void OnRender(PrismaEngine::Graphic::RenderCommandContext* context) {
        // 默认不做任何渲染
    }

private:
    SceneNode* m_parent = nullptr;
    std::vector<std::shared_ptr<SceneNode>> m_children;

    void PushTransform() {
        // 将当前节点的变换压入矩阵栈
        // 实际实现取决于使用的图形API
    }

    void PopTransform() {
        // 从矩阵栈弹出变换
    }
};
