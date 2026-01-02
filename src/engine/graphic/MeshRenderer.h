#pragma once

#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "RenderComponent.h"
#include <memory>
#include <utility>
#include "Model.h"
using namespace PrismaEngine;

class MeshRenderer :public RenderComponent
{
public:
    MeshRenderer(std::shared_ptr<Model> model) : model_(model) {}

    std::shared_ptr<Model> getModel() const { return model_; }

private:
    std::shared_ptr<Model> model_;
private:
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<PrismaEngine::Material> m_material;

protected:
    void DrawMesh(PrismaEngine::Graphic::RenderCommandContext* context, std::shared_ptr<Mesh> mesh);

public:
    // 移除了内联的Render实现，在cpp文件中实现

    void Render(PrismaEngine::Graphic::RenderCommandContext* context) override;

    void SetMesh(std::shared_ptr<Mesh> mesh) {
        m_mesh = std::move(mesh);
    }
    
    void SetMaterial(std::shared_ptr<PrismaEngine::Material> material) override {
        m_material = material;
    }

    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const {
        return m_mesh;
    }

    [[nodiscard]] std::shared_ptr<PrismaEngine::Material> GetMaterial() const override {
        return m_material;
    }
    void Update(float deltaTime) override;
    MeshRenderer();
    ~MeshRenderer() override;
    void Initialize() override;
    void Shutdown() override;
};
