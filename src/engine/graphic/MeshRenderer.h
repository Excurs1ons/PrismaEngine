#pragma once

#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "RenderComponent.h"
#include <memory>
#include <utility>
#include <string>
#include <array>
#include "Model.h"

namespace Prisma::Graphic {

class ENGINE_API MeshRenderer : public RenderComponent
{
public:
    struct Data {
        std::string meshPath;
        std::array<float, 4> color = {0.7f, 0.7f, 0.7f, 1.0f};
        std::array<float, 3> emissive = {0.0f, 0.0f, 0.0f};
        std::string material; // .mat 材质文件路径（引用 material asset）
    };

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
    
    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const {
        return m_mesh;
    }

    // 序列化
    const char* GetComponentTypeName() const override { return "MeshRenderer"; }
    Data GetData() const;
    void SetData(const Data& d);

    // 渲染属性（序列化 + 路径追踪用）
    void SetEmissive(const PrismaMath::vec3& emissive) { m_emissive = emissive; }
    PrismaMath::vec3 GetEmissive() const { return m_emissive; }

protected:
    void DrawMesh(RenderCommandContext* context, std::shared_ptr<Mesh> mesh);

private:
    std::shared_ptr<Model> model_;
    std::shared_ptr<Mesh> m_mesh;
    std::string m_meshPath;
    std::string m_materialPath;
    PrismaMath::vec3 m_emissive = {0.0f, 0.0f, 0.0f};
};

} // namespace Prisma::Graphic
