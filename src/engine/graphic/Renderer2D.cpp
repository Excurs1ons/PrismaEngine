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
    // 创建 Quad 网格
    // 顶点格式: Position(vec4), Color(vec4), UV(vec4)
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

// 5x7 嵌入式点阵字体 (每个字符占用 5 个 uint8_t 字节)
static const unsigned char g_FontData[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x00, 0x7F, 0x41, 0x41}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // \\ (backslash)
    {0x41, 0x41, 0x7F, 0x00, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x05, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x00, 0x7F, 0x10, 0x28, 0x44}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // ~
};

void Renderer2D::DrawString(const std::string& text, const Vector2& position, float scale, const Prisma::Color& color) {
    if (!s_Data) return;

    Vector2 currentPos = position;
    float charSpacing = 6.0f * scale; // 每个字符宽 5 + 1
    float pixelSize = 1.0f * scale;

    for (char c : text) {
        if (c < 32 || c > 126) {
            currentPos.x += charSpacing;
            continue;
        }

        int fontIdx = c - 32;
        for (int col = 0; col < 5; ++col) {
            unsigned char colData = g_FontData[fontIdx][col];
            for (int row = 0; row < 8; ++row) {
                if (colData & (1 << row)) {
                    // 绘制一个像素点
                    Vector2 pixelPos = currentPos + Vector2(col * pixelSize, row * pixelSize);
                    DrawQuad(pixelPos, Vector2(pixelSize, pixelSize), color);
                }
            }
        }
        currentPos.x += charSpacing;
    }
}

float Renderer2D::GetStringWidth(const std::string& text, float scale) {
    return static_cast<float>(text.length()) * 6.0f * scale;
}

void Renderer2D::ResetStats() {
    if (s_Data) s_Data->Stats = Statistics();
}

Renderer2D::Statistics Renderer2D::GetStats() {
    return s_Data ? s_Data->Stats : Statistics();
}

} // namespace Prisma::Graphic
