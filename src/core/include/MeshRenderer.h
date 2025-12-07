#pragma once

#include <memory>
#include "Mesh.h"
#include "Material.h"
#include "RendererComponent.h"
#include "Transform.h"

class MeshRenderer :public RendererComponent
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
    virtual void Update(float deltaTime) override;
    MeshRenderer();
    virtual ~MeshRenderer() override;
    virtual void Initialize() override;
    virtual void Shutdown() override;
};
