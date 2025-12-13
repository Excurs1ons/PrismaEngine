#pragma once

#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "RenderComponent.h"
#include <memory>

class MeshRenderer :public RenderComponent
{
private:
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;
    
protected:
    void DrawMesh(RenderCommandContext* context, std::shared_ptr<Mesh> mesh);
    
public:
    // 移除了内联的Render实现，在cpp文件中实现
    
    void Render(RenderCommandContext* context) override;

    void SetMesh(std::shared_ptr<Mesh> mesh) {
        m_mesh = mesh;
    }
    
    void SetMaterial(std::shared_ptr<Material> material) {
        m_material = material;
    }

    std::shared_ptr<Mesh> GetMesh() const {
        return m_mesh;
    }

    std::shared_ptr<Material> GetMaterial() const {
        return m_material;
    }
    virtual void Update(float deltaTime) override;
    MeshRenderer();
    virtual ~MeshRenderer() override;
    virtual void Initialize() override;
    virtual void Shutdown() override;
};
