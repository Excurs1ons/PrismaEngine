#include "MeshRenderer.h"
#include "ComponentRegistry.h"
#include "Transform.h"
#include "Logger.h"
#include "RenderCommandContext.h"
#include "Renderer.h"
#include "app/Engine.h"
#include "core/AssetManager.h"
#include <glaze/glaze.hpp>
#include <cassert>
#include <cmath>

// ── Glaze 元数据 ──
template <>
struct glz::meta<Prisma::Graphic::MeshRenderer::Data> {
    static constexpr auto value = glz::object(
        "mesh",     &Prisma::Graphic::MeshRenderer::Data::meshPath,
        "color",    &Prisma::Graphic::MeshRenderer::Data::color,
        "emissive", &Prisma::Graphic::MeshRenderer::Data::emissive
    );
};

// ── 注册 ──
namespace {
    bool registered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::Graphic::MeshRenderer>("MeshRenderer");
        reg.RegisterSerializable("MeshRenderer",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::Graphic::MeshRenderer&>(comp);
                auto data = typed.GetData();
                std::string json;
                auto ec = glz::write_json(data, json);
                if (ec) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::Graphic::MeshRenderer&>(comp);
                Prisma::Graphic::MeshRenderer::Data data;
                auto ec = glz::read_json(data, json);
                if (!ec) typed.SetData(data);
            }
        );
        return true;
    }();
}

namespace Prisma::Graphic {

MeshRenderer::MeshRenderer() {}

MeshRenderer::~MeshRenderer() {}

Prisma::Graphic::MeshRenderer::Data MeshRenderer::GetData() const {
    Data d;
    d.meshPath = m_meshPath;
    auto c = GetColor();
    d.color = {c.r, c.g, c.b, c.a};
    d.emissive = {m_emissive.x, m_emissive.y, m_emissive.z};
    return d;
}

void MeshRenderer::SetData(const Data& d) {
    m_meshPath = d.meshPath;
    SetColor(d.color[0], d.color[1], d.color[2], d.color[3]);
    m_emissive = {d.emissive[0], d.emissive[1], d.emissive[2]};

    // 加载网格
    if (!m_meshPath.empty()) {
        auto* am = Prisma::Engine::Get().GetAssetManager();
        if (am) {
            auto handle = am->Load<Graphic::Mesh>(m_meshPath);
            m_mesh = handle.Get();
        }
    }
}

void MeshRenderer::DrawMesh(RenderCommandContext* context, std::shared_ptr<Mesh> mesh)
{
    if (!mesh || !context) {
        return;
    }

    for (const auto& subMesh : mesh->GetSubMeshes()) {
        if (subMesh.indexCount > 0) {
            context->DrawIndexed(subMesh.indexCount, 0, 0);
        } else if (subMesh.vertexCount > 0) {
            context->Draw(subMesh.vertexCount, 0);
        }
    }
}

void MeshRenderer::Render(RenderCommandContext* context)
{
    if (!m_mesh || !context) {
        return;
    }

    if (GetOwnerNode().IsValid()) {
        if (auto transform = GetTransform()) {
            Prisma::Matrix4x4 matrix = transform->GetMatrix();
            context->SetConstantBuffer("ObjectConstants", reinterpret_cast<const float*>(&matrix), 16);
        }
    }

    DrawMesh(context, m_mesh);
}

void MeshRenderer::Update(Timestep ts) {
    auto transform = GetTransform();
    if (m_material && transform) {
        const auto position = transform->GetPosition();
        const float pulse = 0.5f + 0.5f * std::sin(ts.GetSeconds());
        m_material->SetParam("ObjectPosition", PrismaMath::vec3(position.x, position.y, position.z));
        m_material->SetParam("FramePulse", pulse);

        // [新增] 提交渲染指令
        if (m_mesh) {
            Renderer::Submit(m_mesh.get(), m_material.get(), transform->GetMatrix());
        }
    }
}

void MeshRenderer::Initialize() {
    if (!GetOwnerNode().IsValid()) {
        LOG_ERROR("Renderer", "MeshRenderer 初始化时没有所有节点！");
        return;
    }
    
    if (!GetTransform()) {
        LOG_WARNING("Renderer", "MeshRenderer 的节点 '{0}' 没有变换 (Transform) 组件。渲染可能会失败。", GetNodeName());
    }
    
    LOG_TRACE("Renderer", "已为节点 '{0}' 初始化 MeshRenderer", GetNodeName());
}

void MeshRenderer::Shutdown() {
    LOG_TRACE("Renderer", "正在为节点 '{0}' 关闭 MeshRenderer", GetNodeName());
    m_mesh.reset();
    m_material.reset();
}

} // namespace Prisma::Graphic
