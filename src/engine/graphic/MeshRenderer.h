#pragma once

#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "RenderComponent.h"
#include <memory>
#include <utility>

class MeshRenderer :public RenderComponent
{
private:
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Engine::Material> m_material;
    
protected:
    void DrawMesh(RenderCommandContext* context, std::shared_ptr<Mesh> mesh);
    
public:
    // 移除了内联的Render实现，在cpp文件中实现
    
    void Render(RenderCommandContext* context) override;

    void SetMesh(std::shared_ptr<Mesh> mesh) {
        m_mesh = std::move(mesh);
    }
    
    void SetMaterial(std::shared_ptr<Engine::Material> material) override {
        m_material = material;
    }

    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const {
        return m_mesh;
    }

    [[nodiscard]] std::shared_ptr<Engine::Material> GetMaterial() const override {
        return m_material;
    }
    void Update(float deltaTime) override;
    MeshRenderer();
    ~MeshRenderer() override;
    void Initialize() override;
    void Shutdown() override;
};
