#include "Renderer2D.h"
#include "core/Node.h"
#include "core/EntityManager.h"
#include "OrthographicCamera.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Platform.h"
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

static uint32_t MAX_BATCH_QUADS = 50000;
static uint32_t MAX_BATCH_VERTICES = 50000 * 4;
static uint32_t MAX_BATCH_INDICES = 50000 * 6;
static constexpr uint32_t FRAME_SLOTS = 3;

struct Renderer2D::Renderer2DData {
    Renderer2D::Statistics Stats;
    Renderer2D::Statistics LastStats; 
    std::shared_ptr<Mesh> QuadMesh; 
    std::shared_ptr<Material> DefaultMaterial;
    std::vector<std::shared_ptr<Material>> FrameMaterials;
    PrismaMath::mat4 ViewProjection;
    CameraData LastCameraData; 

    struct FrameResource {
        std::shared_ptr<IBuffer> VBO;
        std::shared_ptr<IBuffer> IBO;
        std::shared_ptr<Mesh> MeshObj;
        std::vector<std::shared_ptr<Material>> FrameMaterials;
        Vertex* VertexBufferBase = nullptr; 
        Vertex* VertexBufferPtr = nullptr;  
        uint32_t QuadCount = 0;
    };
    FrameResource Frames[FRAME_SLOTS];      // 场景 VBO（受光照影响）
    FrameResource GizmoFrames[FRAME_SLOTS]; // Gizmo VBO（无光照，纯叠加）
    uint32_t CurrentFrameSlot = 0;

    std::shared_ptr<ITexture> CurrentTexture = nullptr;
    std::shared_ptr<ITexture> LightTexture = nullptr;
    std::shared_ptr<ITexture> WhiteTexture = nullptr; // 全局 1x1 白纹理
    bool BatchingEnabled = true;
    bool InGizmoMode = false; // 当前是否在 Gizmo 绘制模式
};

Renderer2D::Renderer2DData* Renderer2D::s_Data = nullptr;

static const unsigned char g_FontData[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00}, {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62}, {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00}, {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x08, 0x2A, 0x1C, 0x2A, 0x08}, {0x08, 0x08, 0x3E, 0x08, 0x08}, {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00}, {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00}, {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03}, {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00}, {0x00, 0x56, 0x36, 0x00, 0x00}, {0x00, 0x08, 0x14, 0x22, 0x41}, {0x14, 0x14, 0x14, 0x14, 0x14}, {0x41, 0x22, 0x14, 0x08, 0x00}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E}, {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x01, 0x01}, {0x3E, 0x41, 0x41, 0x51, 0x32}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40}, {0x7F, 0x02, 0x04, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x07, 0x18, 0x60, 0x18, 0x07}, {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63}, {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x00, 0x7F, 0x41, 0x41}, {0x02, 0x04, 0x08, 0x10, 0x20}, {0x41, 0x41, 0x7F, 0x00, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04}, {0x40, 0x40, 0x40, 0x40, 0x40}, {0x00, 0x01, 0x02, 0x05, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78}, {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, {0x38, 0x44, 0x44, 0x48, 0x7F}, {0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x08, 0x14, 0x54, 0x54, 0x3C}, {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x20, 0x40, 0x44, 0x3D, 0x00}, {0x00, 0x7F, 0x10, 0x28, 0x44}, {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78}, {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0x7C, 0x14, 0x14, 0x14, 0x08}, {0x08, 0x14, 0x14, 0x18, 0x7C}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20}, {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C}, {0x3C, 0x40, 0x30, 0x40, 0x3C}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C}, {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x7F, 0x00, 0x00}, {0x00, 0x41, 0x36, 0x08, 0x00}, {0x08, 0x08, 0x2A, 0x1C, 0x08}
};

void Renderer2D::Initialize() {
    if (s_Data) return;
    s_Data = new Renderer2DData();
    LOG_INFO("Renderer2D", "正在初始化 2D 渲染器...");
    auto rs = Engine::Get().GetRenderSystem();
    auto rf = rs ? rs->GetDevice()->GetResourceFactory() : nullptr;

    std::vector<Vertex> qV = {
        { { -0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f, 0.0f } },
        { {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } }
    };
    std::vector<uint32_t> qI = { 0, 1, 2, 2, 3, 0 };
    s_Data->QuadMesh = std::make_shared<Mesh>();

    if (rf) {
        BufferDesc vd; vd.type = BufferType::Vertex; vd.size = qV.size() * sizeof(Vertex); vd.initialData = qV.data(); vd.usage = BufferUsage::Immutable;
        auto vbo = rf->CreateBufferImpl(vd);
        BufferDesc id; id.type = BufferType::Index; id.size = qI.size() * sizeof(uint32_t); id.initialData = qI.data(); id.usage = BufferUsage::Immutable;
        auto ibo = rf->CreateBufferImpl(id);
        s_Data->QuadMesh->AddSubMesh({ "Quad", 0, 0, 0, 6, 4, std::move(vbo), std::move(ibo), false });

        // ── 场景 VBO (Frames) ──
        for (uint32_t i = 0; i < FRAME_SLOTS; ++i) {
            auto& f = s_Data->Frames[i];
            BufferDesc bvd; bvd.type = BufferType::Vertex; bvd.size = MAX_BATCH_VERTICES * sizeof(Vertex); bvd.usage = BufferUsage::Dynamic;
            f.VBO = rf->CreateBufferImpl(bvd);
            BufferDesc bid; bid.type = BufferType::Index; bid.size = MAX_BATCH_INDICES * sizeof(uint32_t); bid.usage = BufferUsage::Dynamic;
            f.IBO = rf->CreateBufferImpl(bid);
            f.MeshObj = std::make_shared<Mesh>();
            f.VertexBufferBase = new Vertex[MAX_BATCH_VERTICES];
            f.VertexBufferPtr = f.VertexBufferBase;
            std::vector<uint32_t> bI(MAX_BATCH_INDICES);
            uint32_t off = 0;
            for (uint32_t j = 0; j < MAX_BATCH_INDICES; j += 6) {
                bI[j+0]=off+0; bI[j+1]=off+1; bI[j+2]=off+2; bI[j+3]=off+2; bI[j+4]=off+3; bI[j+5]=off+0; off+=4;
            }
            f.IBO->UpdateData(bI.data(), (uint32_t)bI.size() * sizeof(uint32_t), 0);
            f.MeshObj->AddSubMesh({ "Batch", 0, 0, 0, 0, 0, f.VBO, f.IBO, false });
        }
        // ── Gizmo VBO (GizmoFrames) — 动态 Ring Buffer，每帧更新 ──
        for (uint32_t i = 0; i < FRAME_SLOTS; ++i) {
            auto& f = s_Data->GizmoFrames[i];
            BufferDesc bvd; bvd.type = BufferType::Vertex; bvd.size = MAX_BATCH_VERTICES * sizeof(Vertex); bvd.usage = BufferUsage::Dynamic;
            f.VBO = rf->CreateBufferImpl(bvd);
            BufferDesc bid; bid.type = BufferType::Index; bid.size = MAX_BATCH_INDICES * sizeof(uint32_t); bid.usage = BufferUsage::Dynamic;
            f.IBO = rf->CreateBufferImpl(bid);
            f.MeshObj = std::make_shared<Mesh>();
            f.VertexBufferBase = new Vertex[MAX_BATCH_VERTICES];
            f.VertexBufferPtr = f.VertexBufferBase;
            std::vector<uint32_t> bI(MAX_BATCH_INDICES);
            uint32_t off = 0;
            for (uint32_t j = 0; j < MAX_BATCH_INDICES; j += 6) {
                bI[j+0]=off+0; bI[j+1]=off+1; bI[j+2]=off+2; bI[j+3]=off+2; bI[j+4]=off+3; bI[j+5]=off+0; off+=4;
            }
            f.IBO->UpdateData(bI.data(), (uint32_t)bI.size() * sizeof(uint32_t), 0);
            f.MeshObj->AddSubMesh({ "GizmoBatch", 0, 0, 0, 0, 0, f.VBO, f.IBO, false });
        }
    }

    // [核心修复] 手动加载 2D 专用材质
    auto rm = Engine::Get().GetRenderResourceManager();
    if (rm) {
        uint32_t val = 0xFFFFFFFF; TextureDesc wD; wD.width = 1; wD.height = 1; wD.format = TextureFormat::RGBA8_UNorm;
        auto wT = rm->CreateTextureFromMemory(&val, sizeof(val), wD);
        s_Data->WhiteTexture = wT; // 全局白纹理（无纹理绘制 fallback）
        auto sS = rm->LoadShaderSync("assets/shaders/UnlitSprite.frag.spv");
        if (!sS) sS = rm->LoadShaderSync("DefaultPixel");
        if (sS) {
            s_Data->DefaultMaterial = std::make_shared<Material>(sS);
            s_Data->DefaultMaterial->SetParam("AlbedoMap", wT);
            s_Data->DefaultMaterial->SetParam("LightMap", wT); // 默认白纹理作为 LightMap fallback
        }
    }
    if (!s_Data->DefaultMaterial) s_Data->DefaultMaterial = Material::CreateDefault();
}

void Renderer2D::Shutdown() {
    if (s_Data) {
        for (int i = 0; i < FRAME_SLOTS; ++i) {
            delete[] s_Data->Frames[i].VertexBufferBase;
            delete[] s_Data->GizmoFrames[i].VertexBufferBase;
        }
        delete s_Data; s_Data = nullptr;
    }
}

void Renderer2D::SetMaxBatchQuads(uint32_t maxBatchQuads) {
    MAX_BATCH_QUADS   = maxBatchQuads;
    MAX_BATCH_VERTICES = maxBatchQuads * 4;
    MAX_BATCH_INDICES  = maxBatchQuads * 6;
}

void Renderer2D::BeginScene(const OrthographicCamera& camera) {
    if (!s_Data) return;
    s_Data->ViewProjection = camera.GetViewProjectionMatrix();
    CameraData cD; cD.viewMatrix = camera.GetViewMatrix(); cD.projectionMatrix = camera.GetProjectionMatrix();
    cD.position = camera.GetPosition(); cD.nearPlane = camera.GetNearPlane(); cD.farPlane = camera.GetFarPlane();
    s_Data->LastCameraData = cD;
    Renderer::BeginScene(cD);
    s_Data->CurrentFrameSlot = (s_Data->CurrentFrameSlot + 1) % FRAME_SLOTS;
    s_Data->Stats.QuadCount = 0; s_Data->Stats.DrawCalls = 0;
    StartBatch();
}

void Renderer2D::EndScene() {
    if (s_Data->BatchingEnabled) Flush();
    Renderer::EndScene();
    if (s_Data) s_Data->LastStats = s_Data->Stats;
}

void Renderer2D::BeginUI() {
    if (!s_Data) return;
    StartBatch();
    s_Data->Stats.QuadCount = 0;
}

void Renderer2D::EndUI() {
    if (!s_Data) return;
    Flush();
}

void Renderer2D::BeginGizmo() {
    if (!s_Data) return;
    // 确保前一批次已提交
    if (s_Data->BatchingEnabled) {
        if (s_Data->Frames[s_Data->CurrentFrameSlot].QuadCount > 0 ||
            s_Data->GizmoFrames[s_Data->CurrentFrameSlot].QuadCount > 0) {
            Flush();
        }
    }
    s_Data->InGizmoMode = true;
    s_Data->CurrentTexture = nullptr;
    s_Data->Stats.QuadCount = 0;
    auto& f = s_Data->GizmoFrames[s_Data->CurrentFrameSlot];
    f.VertexBufferPtr = f.VertexBufferBase;
    f.QuadCount = 0;
}

void Renderer2D::EndGizmo() {
    if (!s_Data) return;
    if (s_Data->BatchingEnabled) Flush();
    s_Data->InGizmoMode = false;
    s_Data->CurrentTexture = nullptr;
    // 统计归入 Gizmo 的 Overlay Pass
}

void Renderer2D::Flush() {
    if (!s_Data) return;
    auto& f = s_Data->InGizmoMode
        ? s_Data->GizmoFrames[s_Data->CurrentFrameSlot]
        : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (f.QuadCount == 0) return;
    f.VBO->UpdateData(f.VertexBufferBase, (uint32_t)(f.VertexBufferPtr - f.VertexBufferBase) * sizeof(Vertex), 0);
    auto& sub = const_cast<std::vector<SubMeshBuffer>&>(f.MeshObj->GetSubMeshes());
    if (!sub.empty()) { sub[0].indexCount = f.QuadCount * 6; sub[0].vertexCount = f.QuadCount * 4; }
    
    std::shared_ptr<Material> targetMaterial = s_Data->DefaultMaterial;
    
    // Gizmo 模式：始终用白色 LightMap（无光照效果），纯叠加
    if (s_Data->InGizmoMode) {
        if (s_Data->CurrentTexture) {
            targetMaterial = std::make_shared<Material>(s_Data->DefaultMaterial->GetShader());
            targetMaterial->SetParam("AlbedoMap", s_Data->CurrentTexture);
            // 白色 LightMap = 无光照
            auto* p = s_Data->DefaultMaterial->GetParam("LightMap");
            if (p) targetMaterial->SetParam("LightMap", *p);
            s_Data->FrameMaterials.push_back(targetMaterial);
        }
        Renderer::SubmitGizmo(f.MeshObj.get(), targetMaterial.get(),
            PrismaMath::mat4(1.0f), Prisma::Color(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        // 场景模式：使用真实光照纹理
        if (s_Data->CurrentTexture || s_Data->LightTexture) {
            targetMaterial = std::make_shared<Material>(s_Data->DefaultMaterial->GetShader());
            
            if (s_Data->CurrentTexture) {
                targetMaterial->SetParam("AlbedoMap", s_Data->CurrentTexture);
            } else {
                auto* p = s_Data->DefaultMaterial->GetParam("AlbedoMap");
                if (p) targetMaterial->SetParam("AlbedoMap", *p);
            }
            
            if (s_Data->LightTexture) {
                targetMaterial->SetParam("LightMap", s_Data->LightTexture);
            } else {
                auto* p = s_Data->DefaultMaterial->GetParam("LightMap");
                if (p) targetMaterial->SetParam("LightMap", *p);
            }
            
            s_Data->FrameMaterials.push_back(targetMaterial);
        }
        
        Renderer::Submit(f.MeshObj.get(), targetMaterial.get(),
            PrismaMath::mat4(1.0f), Prisma::Color(1.0f, 1.0f, 1.0f, 1.0f));
    }
    
    s_Data->Stats.DrawCalls++;
    f.VertexBufferPtr = f.VertexBufferBase; f.QuadCount = 0;
}

void Renderer2D::StartBatch() {
    if (!s_Data) return;
    auto& f = s_Data->Frames[s_Data->CurrentFrameSlot];
    f.VertexBufferPtr = f.VertexBufferBase; f.QuadCount = 0;
    s_Data->FrameMaterials.clear(); s_Data->CurrentTexture = nullptr;
}

void Renderer2D::NextBatch() {
    if (s_Data->BatchingEnabled) Flush();
}

void Renderer2D::DrawNodesSoA() {
    auto& em = EntityManager::Get();
    auto* rb = em.GetRenderData();
    auto* tb = em.GetTransformRead();
    uint32_t count = em.GetAliveCount();

    for (uint32_t i = 0; i < count; ++i) {
        if (!rb->active[i]) continue;

        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(tb->posX[i] + rb->sizeW[i] * 0.5f, tb->posY[i] + rb->sizeH[i] * 0.5f, 0.0f));
        if (std::abs(tb->rotation[i]) > 0.001f)
            t = glm::rotate(t, tb->rotation[i], glm::vec3(0, 0, 1));
        t = glm::scale(t, glm::vec3(rb->sizeW[i], rb->sizeH[i], 1.0f));

        DrawQuad(t, {rb->colorR[i], rb->colorG[i], rb->colorB[i], rb->colorA[i]});
    }
}

void Renderer2D::DrawNode(Node node, const Prisma::Color& tint) {
    if (!node.IsValid()) return;
    uint32_t i = node.GetIndex();
    auto& em = EntityManager::Get();
    auto* rb = em.GetRenderData();
    auto* tb = em.GetTransformRead();

    Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(tb->posX[i] + rb->sizeW[i] * 0.5f, tb->posY[i] + rb->sizeH[i] * 0.5f, 0.0f));
    if (std::abs(tb->rotation[i]) > 0.001f)
        t = glm::rotate(t, tb->rotation[i], glm::vec3(0, 0, 1));
    t = glm::scale(t, glm::vec3(rb->sizeW[i], rb->sizeH[i], 1.0f));

    Prisma::Color color = { rb->colorR[i] * tint.r, rb->colorG[i] * tint.g, rb->colorB[i] * tint.b, rb->colorA[i] * tint.a };
    DrawQuad(t, color);
}

void Renderer2D::DrawLine(const Vector2& start, const Vector2& end, const Prisma::Color& color, float thickness) {
    Vector2 dir = end - start;
    float length = glm::length(dir);
    if (length < 0.0001f) return;

    Vector2 center = start + dir * 0.5f;
    float angle = std::atan2(dir.y, dir.x);

    Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(length, thickness, 1.0f));
    DrawQuad(t, color);
}

void Renderer2D::DrawRect(const Vector2& pos, const Vector2& size, const Prisma::Color& color, float thickness) {
    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;

    // Top
    DrawLine({pos.x - hw, pos.y + hh}, {pos.x + hw, pos.y + hh}, color, thickness);
    // Bottom
    DrawLine({pos.x - hw, pos.y - hh}, {pos.x + hw, pos.y - hh}, color, thickness);
    // Left
    DrawLine({pos.x - hw, pos.y - hh}, {pos.x - hw, pos.y + hh}, color, thickness);
    // Right
    DrawLine({pos.x + hw, pos.y - hh}, {pos.x + hw, pos.y + hh}, color, thickness);
}

void Renderer2D::DrawQuad(const Vector2& pos, const Vector2& size, const Prisma::Color& col) {
    if (!s_Data) return;
    if (!s_Data->BatchingEnabled) {
        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
        Renderer::Submit(s_Data->QuadMesh.get(), s_Data->DefaultMaterial.get(), t, col);
        s_Data->Stats.QuadCount++; s_Data->Stats.DrawCalls++; return;
    }
    auto& f = s_Data->InGizmoMode ? s_Data->GizmoFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    auto* wt = s_Data->WhiteTexture.get();
    if (s_Data->CurrentTexture.get() != wt || f.QuadCount >= MAX_BATCH_QUADS) { NextBatch(); s_Data->CurrentTexture = s_Data->WhiteTexture; }
    float hw = size.x * 0.5f, hh = size.y * 0.5f; PrismaMath::vec4 tint = {col.r, col.g, col.b, col.a};
    Vertex* v = f.VertexBufferPtr;
    v[0] = { {pos.x - hw, pos.y - hh, 0, 1}, tint, {0, 1, 0, 0} };
    v[1] = { {pos.x + hw, pos.y - hh, 0, 1}, tint, {1, 1, 0, 0} };
    v[2] = { {pos.x + hw, pos.y + hh, 0, 1}, tint, {1, 0, 0, 0} };
    v[3] = { {pos.x - hw, pos.y + hh, 0, 1}, tint, {0, 0, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Matrix4& trans, const Prisma::Color& col) {
    if (!s_Data) return;
    if (!s_Data->BatchingEnabled) {
        Renderer::Submit(s_Data->QuadMesh.get(), s_Data->DefaultMaterial.get(), trans, col);
        s_Data->Stats.QuadCount++; s_Data->Stats.DrawCalls++; return;
    }
    auto& f = s_Data->InGizmoMode ? s_Data->GizmoFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (s_Data->CurrentTexture.get() != s_Data->WhiteTexture.get() || f.QuadCount >= MAX_BATCH_QUADS) { NextBatch(); s_Data->CurrentTexture = s_Data->WhiteTexture; }
    PrismaMath::vec4 tint = {col.r, col.g, col.b, col.a};
    Vertex* v = f.VertexBufferPtr;
    v[0] = { trans * PrismaMath::vec4(-0.5f, -0.5f, 0, 1), tint, {0, 1, 0, 0} };
    v[1] = { trans * PrismaMath::vec4( 0.5f, -0.5f, 0, 1), tint, {1, 1, 0, 0} };
    v[2] = { trans * PrismaMath::vec4( 0.5f,  0.5f, 0, 1), tint, {1, 0, 0, 0} };
    v[3] = { trans * PrismaMath::vec4(-0.5f,  0.5f, 0, 1), tint, {0, 0, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Vector2& pos, const Vector2& size, const std::shared_ptr<ITexture>& tex, const Prisma::Color& tC) {
    if (!s_Data) return;
    if (!s_Data->BatchingEnabled) {
        auto m = std::make_shared<Material>(s_Data->DefaultMaterial->GetShader()); m->SetParam("AlbedoMap", tex);
        s_Data->FrameMaterials.push_back(m);
        Matrix4 t = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
        Renderer::Submit(s_Data->QuadMesh.get(), m.get(), t, tC);
        s_Data->Stats.QuadCount++; s_Data->Stats.DrawCalls++; return;
    }
    bool texChanged = tex != s_Data->CurrentTexture;
    auto& f = s_Data->InGizmoMode ? s_Data->GizmoFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (texChanged || f.QuadCount >= MAX_BATCH_QUADS) { NextBatch(); s_Data->CurrentTexture = tex; }
    float hw = size.x * 0.5f, hh = size.y * 0.5f; PrismaMath::vec4 tint = {tC.r, tC.g, tC.b, tC.a};
    Vertex* v = f.VertexBufferPtr;
    v[0] = { {pos.x - hw, pos.y - hh, 0, 1}, tint, {0, 1, 0, 0} };
    v[1] = { {pos.x + hw, pos.y - hh, 0, 1}, tint, {1, 1, 0, 0} };
    v[2] = { {pos.x + hw, pos.y + hh, 0, 1}, tint, {1, 0, 0, 0} };
    v[3] = { {pos.x - hw, pos.y + hh, 0, 1}, tint, {0, 0, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Matrix4& trans, const std::shared_ptr<ITexture>& tex, const Prisma::Color& tC) {
    if (!s_Data) return;
    if (!s_Data->BatchingEnabled) {
        auto m = std::make_shared<Material>(s_Data->DefaultMaterial->GetShader()); m->SetParam("AlbedoMap", tex);
        s_Data->FrameMaterials.push_back(m);
        Renderer::Submit(s_Data->QuadMesh.get(), m.get(), trans, tC);
        s_Data->Stats.QuadCount++; s_Data->Stats.DrawCalls++; return;
    }
    bool texChanged = tex != s_Data->CurrentTexture;
    auto& f = s_Data->InGizmoMode ? s_Data->GizmoFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (texChanged || f.QuadCount >= MAX_BATCH_QUADS) { NextBatch(); s_Data->CurrentTexture = tex; }
    PrismaMath::vec4 tint = {tC.r, tC.g, tC.b, tC.a};
    Vertex* v = f.VertexBufferPtr;
    v[0] = { trans * PrismaMath::vec4(-0.5f, -0.5f, 0, 1), tint, {0, 1, 0, 0} };
    v[1] = { trans * PrismaMath::vec4( 0.5f, -0.5f, 0, 1), tint, {1, 1, 0, 0} };
    v[2] = { trans * PrismaMath::vec4( 0.5f,  0.5f, 0, 1), tint, {1, 0, 0, 0} };
    v[3] = { trans * PrismaMath::vec4(-0.5f,  0.5f, 0, 1), tint, {0, 0, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Vector2& pos, const Vector2& size, const std::shared_ptr<ITexture>& tex, const Vector2 uv[4], const Prisma::Color& tC) {
    if (!s_Data) return;
    if (!s_Data->BatchingEnabled) { DrawQuad(pos, size, tex, tC); return; }
    bool texChanged = tex != s_Data->CurrentTexture;
    auto& f = s_Data->InGizmoMode ? s_Data->GizmoFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (texChanged || f.QuadCount >= MAX_BATCH_QUADS) { NextBatch(); s_Data->CurrentTexture = tex; }
    float hw = size.x * 0.5f, hh = size.y * 0.5f; PrismaMath::vec4 tint = {tC.r, tC.g, tC.b, tC.a};
    Vertex* v = f.VertexBufferPtr;
    v[0] = { {pos.x - hw, pos.y - hh, 0, 1}, tint, {uv[0].x, uv[0].y, 0, 0} };
    v[1] = { {pos.x + hw, pos.y - hh, 0, 1}, tint, {uv[1].x, uv[1].y, 0, 0} };
    v[2] = { {pos.x + hw, pos.y + hh, 0, 1}, tint, {uv[2].x, uv[2].y, 0, 0} };
    v[3] = { {pos.x - hw, pos.y + hh, 0, 1}, tint, {uv[3].x, uv[3].y, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const Matrix4& trans, const std::shared_ptr<ITexture>& tex, const Vector2 uv[4], const Prisma::Color& tC) {
    if (!s_Data) return;
    if (!s_Data->BatchingEnabled) { DrawQuad(trans, tex, tC); return; }
    bool texChanged = tex != s_Data->CurrentTexture;
    auto& f = s_Data->InGizmoMode ? s_Data->GizmoFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (texChanged || f.QuadCount >= MAX_BATCH_QUADS) { NextBatch(); s_Data->CurrentTexture = tex; }
    PrismaMath::vec4 tint = {tC.r, tC.g, tC.b, tC.a};
    Vertex* v = f.VertexBufferPtr;
    v[0] = { trans * PrismaMath::vec4(-0.5f, -0.5f, 0, 1), tint, {uv[0].x, uv[0].y, 0, 0} };
    v[1] = { trans * PrismaMath::vec4( 0.5f, -0.5f, 0, 1), tint, {uv[1].x, uv[1].y, 0, 0} };
    v[2] = { trans * PrismaMath::vec4( 0.5f,  0.5f, 0, 1), tint, {uv[2].x, uv[2].y, 0, 0} };
    v[3] = { trans * PrismaMath::vec4(-0.5f,  0.5f, 0, 1), tint, {0, 0, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Renderer2D::DrawString(const std::string& text, const Vector2& pos, float scale, const Prisma::Color& color) {
    if (!s_Data || text.empty()) return;
    if (!s_Data->BatchingEnabled) {
        float cS = 6.0f * scale; Vector2 cP = pos;
        for (char c : text) {
            if (c < 32 || c > 126) { cP.x += cS; continue; }
            const unsigned char* g = g_FontData[c - 32];
            for (int col = 0; col < 5; ++col) {
                unsigned char d = g[col];
                for (int row = 0; row < 8; ++row) {
                    if (d & (1 << row)) {
                        DrawQuad(cP + Vector2(col * scale, (7 - row) * scale), Vector2(scale, scale), color);
                    }
                }
            }
            cP.x += cS;
        }
        return;
    }
    auto& f = s_Data->InGizmoMode ? s_Data->GizmoFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (s_Data->CurrentTexture != nullptr) { NextBatch(); s_Data->CurrentTexture = nullptr; }
    float charSpacing = 6.0f * scale, pixelSize = 1.0f * scale; PrismaMath::vec4 tint = {color.r, color.g, color.b, color.a};
    Vector2 cur = pos;
    for (char c : text) {
        if (c < 32 || c > 126) { cur.x += charSpacing; continue; }
        const unsigned char* glyph = g_FontData[c - 32];
        for (int col = 0; col < 5; ++col) {
            unsigned char data = glyph[col];
            for (int row = 0; row < 8; ++row) {
                if (data & (1 << row)) {
                    if (f.QuadCount >= MAX_BATCH_QUADS) { NextBatch(); s_Data->CurrentTexture = nullptr; }
                    float px = cur.x + col * pixelSize, py = cur.y + (7 - row) * pixelSize;
                    Vertex* v = f.VertexBufferPtr;
                    v[0] = {{px, py, 0, 1}, tint, {0, 1, 0, 0}};
                    v[1] = {{px + pixelSize, py, 0, 1}, tint, {1, 1, 0, 0}};
                    v[2] = {{px + pixelSize, py + pixelSize, 0, 1}, tint, {1, 0, 0, 0}};
                    v[3] = {{px, py + pixelSize, 0, 1}, tint, {0, 0, 0, 0}};
                    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
                }
            }
        }
        cur.x += charSpacing;
    }
}

float Renderer2D::GetStringWidth(const std::string& text, float scale) { return (float)text.length() * 6.0f * scale; }
void Renderer2D::ResetStats() { if (s_Data) s_Data->Stats = Statistics(); }
Renderer2D::Statistics Renderer2D::GetStats() { return s_Data ? s_Data->LastStats : Statistics(); }
void Renderer2D::SetBatchingEnabled(bool enabled) { if (s_Data) s_Data->BatchingEnabled = enabled; }
bool Renderer2D::IsBatchingEnabled() { return s_Data ? s_Data->BatchingEnabled : false; }

std::shared_ptr<ITexture> Renderer2D::GetWhiteTexture() {
    return s_Data ? s_Data->WhiteTexture : nullptr;
}

void Renderer2D::SetLightTexture(const std::shared_ptr<ITexture>& texture) {
    if (s_Data) s_Data->LightTexture = texture;
}

} // namespace Prisma::Graphic
