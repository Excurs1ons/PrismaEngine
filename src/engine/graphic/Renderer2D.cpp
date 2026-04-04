#include "Renderer2D.h"
#include "OrthographicCamera.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Material.h"
#include "RenderResourceManager.h"
#include "interfaces/IBuffer.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceFactory.h"
#include "Engine.h"
#include "RenderSystem.h"
#include "Logger.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace Prisma::Graphic {

struct Renderer2D::Renderer2DData {
    Renderer2D::Statistics Stats;
    std::shared_ptr<Mesh> QuadMesh;
    std::shared_ptr<Material> DefaultMaterial;
    std::vector<std::shared_ptr<Material>> FrameMaterials;
    PrismaMath::mat4 ViewProjection;
};

Renderer2D::Renderer2DData* Renderer2D::s_Data = nullptr;

void Renderer2D::Initialize() {
    if (s_Data) return;
    s_Data = new Renderer2DData();

    LOG_INFO("Renderer2D", "正在初始化 2D 渲染器...");

    // 创建一个简单的 Quad Mesh
    s_Data->QuadMesh = std::make_shared<Mesh>();
    
    // 顶点数据 (Position, Color, UV)
    std::vector<Vertex> vertices = {
        { { -0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 0.0f } },
        { {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 0.0f } }
    };

    std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

    auto renderSystem = Engine::Get().GetRenderSystem();
    auto device = renderSystem ? renderSystem->GetDevice() : nullptr;
    
    if (device && device->GetResourceFactory()) {
        BufferDesc vDesc;
        vDesc.type = BufferType::Vertex;
        vDesc.size = vertices.size() * sizeof(Vertex);
        vDesc.initialData = vertices.data();
        auto vbo = device->GetResourceFactory()->CreateBufferImpl(vDesc);

        BufferDesc iDesc;
        iDesc.type = BufferType::Index;
        iDesc.size = indices.size() * sizeof(uint32_t);
        iDesc.initialData = indices.data();
        auto ibo = device->GetResourceFactory()->CreateBufferImpl(iDesc);

        if (!vbo || !ibo) {
            LOG_ERROR("Renderer2D", "创建 2D Quad 缓冲区失败。");
            s_Data->QuadMesh.reset();
            s_Data->DefaultMaterial = Material::CreateDefault();
            return;
        }

        SubMeshBuffer subMesh;
        subMesh.vertexBuffer = std::shared_ptr<IBuffer>(std::move(vbo));
        subMesh.indexBuffer = std::shared_ptr<IBuffer>(std::move(ibo));
        subMesh.indexCount = 6;
        subMesh.vertexCount = 4;
        subMesh.use16BitIndices = false;
        
        s_Data->QuadMesh->AddSubMesh(subMesh);
        s_Data->QuadMesh->SetBoundingBox(BoundingBox(
            Prisma::Vector3(-0.5f, -0.5f, 0.0f),
            Prisma::Vector3(0.5f, 0.5f, 0.0f)
        ));
        LOG_INFO("Renderer2D", "已创建 2D Quad 网格资源。");
    }

    s_Data->DefaultMaterial = Material::CreateDefault();
    LOG_INFO("Renderer2D", "2D 渲染器初始化完成。");
}

void Renderer2D::Shutdown() {
    LOG_INFO("Renderer2D", "正在关闭 2D 渲染器...");
    delete s_Data;
    s_Data = nullptr;
}

void Renderer2D::BeginScene(const OrthographicCamera& camera) {
    if (!s_Data) return;
    s_Data->ViewProjection = camera.GetViewProjectionMatrix();
    
    CameraData cameraData;
    cameraData.viewMatrix = camera.GetViewMatrix();
    cameraData.projectionMatrix = camera.GetProjectionMatrix();
    cameraData.position = camera.GetPosition();
    cameraData.nearPlane = camera.GetNearPlane();
    cameraData.farPlane = camera.GetFarPlane();
    
    Renderer::BeginScene(cameraData);
    
    StartBatch();
}

void Renderer2D::EndScene() {
    Flush();
    Renderer::EndScene();
    
    if (s_Data && s_Data->Stats.QuadCount > 0) {
        static bool firstRenderReported = false;
        if (!firstRenderReported) {
            LOG_INFO("Renderer2D", "首次渲染成功触发，本帧渲染了 {} 个 Quad。", s_Data->Stats.QuadCount);
            firstRenderReported = true;
        }

        // 每一百帧打印一次统计，避免刷屏
        static int frameCount = 0;
        if (++frameCount % 100 == 0) {
            LOG_DEBUG("Renderer2D", "渲染统计: {} 个 Quad。", s_Data->Stats.QuadCount);
        }
    }
}

void Renderer2D::Flush() {
}

void Renderer2D::StartBatch() {
    if (!s_Data) return;
    s_Data->Stats.QuadCount = 0;
    s_Data->Stats.DrawCalls = 0;
    s_Data->FrameMaterials.clear();
}

void Renderer2D::NextBatch() {
    Flush();
    StartBatch();
}

void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Prisma::Color& color) {
    Matrix4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) * 
                        glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
    DrawQuad(transform, color);
}

void Renderer2D::DrawQuad(const Matrix4& transform, const Prisma::Color& color) {
    if (!s_Data || !s_Data->QuadMesh || !s_Data->DefaultMaterial) return;

    // TODO: 为了支持批处理和不同属性，后续应该使用更高效的材质管理
    // 暂时为了性能和稳定性，复用默认材质
    Renderer::Submit(s_Data->QuadMesh.get(), s_Data->DefaultMaterial.get(), transform, color);
    s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Prisma::Color& tintColor) {
    Matrix4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) * 
                        glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
    DrawQuad(transform, texture, tintColor);
}

void Renderer2D::DrawQuad(const Matrix4& transform, const std::shared_ptr<ITexture>& texture, const Prisma::Color& tintColor) {
    if (!s_Data || !s_Data->QuadMesh || !s_Data->DefaultMaterial) return;
    
    // 如果没有纹理，直接调用基础版本
    if (!texture) {
        DrawQuad(transform, tintColor);
        return;
    }

    // 含有纹理的情况，目前依然需要临时材质，但后续应优化
    auto material = std::make_shared<Material>(s_Data->DefaultMaterial->GetShader());
    material->SetParam("AlbedoMap", texture);
    s_Data->FrameMaterials.push_back(material);
    
    Renderer::Submit(s_Data->QuadMesh.get(), material.get(), transform, tintColor);
    s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], const Prisma::Color& tintColor) {
    DrawQuad(position, size, texture, tintColor);
}

void Renderer2D::DrawQuad(const Matrix4& transform, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], const Prisma::Color& tintColor) {
    DrawQuad(transform, texture, tintColor);
}

void Renderer2D::ResetStats() {
    if (s_Data) s_Data->Stats = Statistics();
}

Renderer2D::Statistics Renderer2D::GetStats() {
    return s_Data ? s_Data->Stats : Statistics();
}

} // namespace Prisma::Graphic
