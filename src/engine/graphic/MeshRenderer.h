#pragma once

#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "RenderComponent.h"
#include <memory>
#include <utility>
#include "Model.h"

namespace Prisma::Graphic {

class MeshRenderer : public RenderComponent
{
public:
    MeshRenderer();
    MeshRenderer(std::shared_ptr<Model> model) : model_(model) {}
    ~MeshRenderer() override;

    std::shared_ptr<Model> getModel() const { return model_; }

    void Render(RenderCommandContext* context) override;
    void Update(Timestep ts) override;
    void Initialize() override;
    void Shutdown() override;

    void SetMesh(std::shared_ptr<Mesh> mesh) {
        m_mesh = std::move(mesh);
    }
    
    void SetMaterial(std::shared_ptr<Material> material) override {
        m_material = material;
    }

    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const {
        return m_mesh;
    }

    [[nodiscard]] std::shared_ptr<Material> GetMaterial() const override {
        return m_material;
    }

protected:
    void DrawMesh(RenderCommandContext* context, std::shared_ptr<Mesh> mesh);

private:
    std::shared_ptr<Model> model_;
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;
};

} // namespace Prisma::Graphic
