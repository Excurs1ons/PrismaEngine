#include "Graphics2D.h"
#include "graphic/OrthographicCamera.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/RenderResourceManager.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "Logger.h"
#include <algorithm>
#include <map>
#include <glm/gtc/matrix_transform.hpp>

namespace Prisma::Graphic {

static constexpr uint32_t MAX_BATCH_QUADS = 50000;
static constexpr uint32_t MAX_BATCH_VERTICES = MAX_BATCH_QUADS * 4;
static constexpr uint32_t MAX_BATCH_INDICES = MAX_BATCH_QUADS * 6;
static constexpr uint32_t FRAME_SLOTS = 3;

static const unsigned char g_FontData[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00}, {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62}, {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00}, {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x08, 0x2A, 0x1C, 0x2A, 0x08}, {0x08, 0x08, 0x3E, 0x08, 0x08}, {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00}, {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00}, {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03}, {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00}, {0x00, 0x56, 0x36, 0x00, 0x00}, {0x00, 0x08, 0x14, 0x22, 0x41}, {0x14, 0x14, 0x14, 0x14, 0x14}, {0x41, 0x22, 0x14, 0x08, 0x00}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E}, {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x01, 0x01}, {0x3E, 0x41, 0x41, 0x51, 0x32}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40}, {0x7F, 0x02, 0x04, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x07, 0x18, 0x60, 0x18, 0x07}, {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63}, {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x00, 0x7F, 0x41, 0x41}, {0x02, 0x04, 0x08, 0x10, 0x20}, {0x41, 0x41, 0x7F, 0x00, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04}, {0x40, 0x40, 0x40, 0x40, 0x40}, {0x00, 0x01, 0x02, 0x05, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78}, {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, {0x38, 0x44, 0x44, 0x48, 0x7F}, {0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x08, 0x14, 0x54, 0x54, 0x3C}, {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x20, 0x40, 0x44, 0x3D, 0x00}, {0x00, 0x7F, 0x10, 0x28, 0x44}, {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78}, {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0x7C, 0x14, 0x14, 0x14, 0x08}, {0x08, 0x14, 0x14, 0x18, 0x7C}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20}, {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C}, {0x3C, 0x40, 0x30, 0x40, 0x3C}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C}, {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x7F, 0x00, 0x00}, {0x00, 0x41, 0x36, 0x08, 0x00}, {0x08, 0x08, 0x2A, 0x1C, 0x08}
};

struct Graphics2D::Renderer2DData {
    struct FrameResource {
        std::shared_ptr<IBuffer> VBO;
        std::shared_ptr<IBuffer> IBO;
        std::shared_ptr<Mesh> MeshObj;
        Vertex* VertexBufferBase = nullptr; 
        Vertex* VertexBufferPtr = nullptr;  
        uint32_t QuadCount = 0;
    };

    struct RenderBatch {
        std::shared_ptr<ITexture> Texture;
        uint32_t VertexOffset;
        uint32_t IndexCount;
        int SortingOrder;
    };

    FrameResource Frames[FRAME_SLOTS];
    FrameResource UIFrames[FRAME_SLOTS];
    std::vector<RenderBatch> BatchQueue;

    uint32_t CurrentFrameSlot = 0;
    std::shared_ptr<ITexture> CurrentTexture = nullptr;
    std::shared_ptr<ITexture> LightTexture = nullptr;
    std::shared_ptr<Material> DefaultMaterial;
    
    Graphics2D::Statistics Stats;
    bool InUIMode = false;
    PrismaMath::mat4 ViewProjection;
};

Graphics2D::Renderer2DData* Graphics2D::s_Data = nullptr;

void Graphics2D::Initialize() {
    if (s_Data) return;
    s_Data = new Renderer2DData();
    LOG_INFO("Graphics2D", "正在初始化 2D 绘图系统...");
    
    auto rs = Engine::Get().GetRenderSystem();
    auto rf = rs ? rs->GetDevice()->GetResourceFactory() : nullptr;

    if (rf) {
        auto setupFrames = [&](Renderer2DData::FrameResource* frames) {
            for (uint32_t i = 0; i < FRAME_SLOTS; ++i) {
                auto& f = frames[i];
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
                f.MeshObj->AddSubMesh({ "CanvasBatch", 0, 0, 0, 0, 0, f.VBO, f.IBO, false });
            }
        };
        setupFrames(s_Data->Frames);
        setupFrames(s_Data->UIFrames);
    }

    auto rm = Engine::Get().GetRenderResourceManager();
    if (rm) {
        auto sS = rm->LoadShaderSync("assets/shaders/LitSprite.frag.spv");
        if (sS) s_Data->DefaultMaterial = std::make_shared<Material>(sS);
    }
    if (!s_Data->DefaultMaterial) s_Data->DefaultMaterial = Material::CreateDefault();
}

void Graphics2D::Shutdown() {
    if (s_Data) {
        for (int i = 0; i < FRAME_SLOTS; ++i) {
            delete[] s_Data->Frames[i].VertexBufferBase;
            delete[] s_Data->UIFrames[i].VertexBufferBase;
        }
        delete s_Data; s_Data = nullptr;
    }
}

void Graphics2D::Begin(const OrthographicCamera& camera) {
    if (!s_Data) return;
    s_Data->ViewProjection = camera.GetViewProjectionMatrix();
    s_Data->CurrentFrameSlot = (s_Data->CurrentFrameSlot + 1) % FRAME_SLOTS;
    s_Data->Stats = {};
    s_Data->BatchQueue.clear();
    s_Data->InUIMode = false;
    
    auto& f = s_Data->Frames[s_Data->CurrentFrameSlot];
    f.VertexBufferPtr = f.VertexBufferBase;
    f.QuadCount = 0;
}

void Graphics2D::End() {
    if (!s_Data) return;
    if (s_Data->Frames[s_Data->CurrentFrameSlot].QuadCount > 0) {
        NextBatch();
    }
}

void Graphics2D::BeginUI() {
    if (!s_Data) return;
    s_Data->InUIMode = true;
    auto& f = s_Data->UIFrames[s_Data->CurrentFrameSlot];
    f.VertexBufferPtr = f.VertexBufferBase;
    f.QuadCount = 0;
}

void Graphics2D::EndUI() {
    if (!s_Data) return;
    if (s_Data->UIFrames[s_Data->CurrentFrameSlot].QuadCount > 0) {
        NextBatch();
    }
    s_Data->InUIMode = false;
}

void Graphics2D::StartBatch() {
    // 自动合批模式下，StartBatch 通常在 Begin 内部调用
}

void Graphics2D::NextBatch() {
    auto& f = s_Data->InUIMode ? s_Data->UIFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (f.QuadCount == 0) return;

    Renderer2DData::RenderBatch batch;
    batch.Texture = s_Data->CurrentTexture;
    batch.VertexOffset = (uint32_t)(f.VertexBufferPtr - f.VertexBufferBase) - (f.QuadCount * 4);
    batch.IndexCount = f.QuadCount * 6;
    batch.SortingOrder = 0; 
    
    s_Data->BatchQueue.push_back(batch);
}

void Graphics2D::DrawQuad(const Vector2& pos, const Vector2& size, const Prisma::Color& color, int sortingOrder) {
    auto& f = s_Data->InUIMode ? s_Data->UIFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (s_Data->CurrentTexture != nullptr || f.QuadCount >= MAX_BATCH_QUADS) {
        NextBatch();
        s_Data->CurrentTexture = nullptr;
    }
    float hw = size.x * 0.5f, hh = size.y * 0.5f;
    PrismaMath::vec4 tint = {color.r, color.g, color.b, color.a};
    float z = (float)sortingOrder * 0.001f;
    Vertex* v = f.VertexBufferPtr;
    v[0] = { {pos.x - hw, pos.y - hh, z, 1}, tint, {0, 0, 0, 0} }; // TL
    v[1] = { {pos.x + hw, pos.y - hh, z, 1}, tint, {1, 0, 0, 0} }; // TR
    v[2] = { {pos.x + hw, pos.y + hh, z, 1}, tint, {1, 1, 0, 0} }; // BR
    v[3] = { {pos.x - hw, pos.y + hh, z, 1}, tint, {0, 1, 0, 0} }; // BL
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Graphics2D::DrawQuad(const Matrix4& trans, const Prisma::Color& color, int sortingOrder) {
    auto& f = s_Data->InUIMode ? s_Data->UIFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (s_Data->CurrentTexture != nullptr || f.QuadCount >= MAX_BATCH_QUADS) {
        NextBatch();
        s_Data->CurrentTexture = nullptr;
    }
    PrismaMath::vec4 tint = {color.r, color.g, color.b, color.a};
    float z = (float)sortingOrder * 0.001f;
    Vertex* v = f.VertexBufferPtr;
    v[0] = { trans * PrismaMath::vec4(-0.5f, -0.5f, z, 1), tint, {0, 0, 0, 0} };
    v[1] = { trans * PrismaMath::vec4( 0.5f, -0.5f, z, 1), tint, {1, 0, 0, 0} };
    v[2] = { trans * PrismaMath::vec4( 0.5f,  0.5f, z, 1), tint, {1, 1, 0, 0} };
    v[3] = { trans * PrismaMath::vec4(-0.5f,  0.5f, z, 1), tint, {0, 1, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Graphics2D::DrawSprite(const Vector2& pos, const Vector2& size, const std::shared_ptr<ITexture>& tex, int sortingOrder, const Prisma::Color& tint) {
    auto& f = s_Data->InUIMode ? s_Data->UIFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (tex != s_Data->CurrentTexture || f.QuadCount >= MAX_BATCH_QUADS) {
        NextBatch();
        s_Data->CurrentTexture = tex;
    }
    float hw = size.x * 0.5f, hh = size.y * 0.5f;
    PrismaMath::vec4 col = {tint.r, tint.g, tint.b, tint.a};
    float z = (float)sortingOrder * 0.001f;
    Vertex* v = f.VertexBufferPtr;
    v[0] = { {pos.x - hw, pos.y - hh, z, 1}, col, {0, 0, 0, 0} };
    v[1] = { {pos.x + hw, pos.y - hh, z, 1}, col, {1, 0, 0, 0} };
    v[2] = { {pos.x + hw, pos.y + hh, z, 1}, col, {1, 1, 0, 0} };
    v[3] = { {pos.x - hw, pos.y + hh, z, 1}, col, {0, 1, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Graphics2D::DrawSprite(const Matrix4& trans, const std::shared_ptr<ITexture>& tex, int sortingOrder, const Prisma::Color& tint) {
    auto& f = s_Data->InUIMode ? s_Data->UIFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (tex != s_Data->CurrentTexture || f.QuadCount >= MAX_BATCH_QUADS) {
        NextBatch();
        s_Data->CurrentTexture = tex;
    }
    PrismaMath::vec4 col = {tint.r, tint.g, tint.b, tint.a};
    float z = (float)sortingOrder * 0.001f;
    Vertex* v = f.VertexBufferPtr;
    v[0] = { trans * PrismaMath::vec4(-0.5f, -0.5f, z, 1), col, {0, 0, 0, 0} };
    v[1] = { trans * PrismaMath::vec4( 0.5f, -0.5f, z, 1), col, {1, 0, 0, 0} };
    v[2] = { trans * PrismaMath::vec4( 0.5f,  0.5f, z, 1), col, {1, 1, 0, 0} };
    v[3] = { trans * PrismaMath::vec4(-0.5f,  0.5f, z, 1), col, {0, 1, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Graphics2D::DrawSprite(const Vector2& pos, const Vector2& size, const std::shared_ptr<ITexture>& tex, const Vector2 uv[4], int sortingOrder, const Prisma::Color& tint) {
    auto& f = s_Data->InUIMode ? s_Data->UIFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (tex != s_Data->CurrentTexture || f.QuadCount >= MAX_BATCH_QUADS) {
        NextBatch();
        s_Data->CurrentTexture = tex;
    }
    float hw = size.x * 0.5f, hh = size.y * 0.5f;
    PrismaMath::vec4 col = {tint.r, tint.g, tint.b, tint.a};
    float z = (float)sortingOrder * 0.001f;
    Vertex* v = f.VertexBufferPtr;
    v[0] = { {pos.x - hw, pos.y - hh, z, 1}, col, {uv[0].x, uv[0].y, 0, 0} };
    v[1] = { {pos.x + hw, pos.y - hh, z, 1}, col, {uv[1].x, uv[1].y, 0, 0} };
    v[2] = { {pos.x + hw, pos.y + hh, z, 1}, col, {uv[2].x, uv[2].y, 0, 0} };
    v[3] = { {pos.x - hw, pos.y + hh, z, 1}, col, {uv[3].x, uv[3].y, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Graphics2D::DrawSprite(const Matrix4& trans, const std::shared_ptr<ITexture>& tex, const Vector2 uv[4], int sortingOrder, const Prisma::Color& tint) {
    auto& f = s_Data->InUIMode ? s_Data->UIFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (tex != s_Data->CurrentTexture || f.QuadCount >= MAX_BATCH_QUADS) {
        NextBatch();
        s_Data->CurrentTexture = tex;
    }
    PrismaMath::vec4 col = {tint.r, tint.g, tint.b, tint.a};
    float z = (float)sortingOrder * 0.001f;
    Vertex* v = f.VertexBufferPtr;
    v[0] = { trans * PrismaMath::vec4(-0.5f, -0.5f, z, 1), col, {uv[0].x, uv[0].y, 0, 0} };
    v[1] = { trans * PrismaMath::vec4( 0.5f, -0.5f, z, 1), col, {uv[1].x, uv[1].y, 0, 0} };
    v[2] = { trans * PrismaMath::vec4( 0.5f,  0.5f, z, 1), col, {uv[2].x, uv[2].y, 0, 0} };
    v[3] = { trans * PrismaMath::vec4(-0.5f,  0.5f, z, 1), col, {uv[3].x, uv[3].y, 0, 0} };
    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
}

void Graphics2D::DrawString(const std::string& text, const Vector2& pos, float scale, const Prisma::Color& color, int sortingOrder) {
    if (text.empty()) return;
    auto& f = s_Data->InUIMode ? s_Data->UIFrames[s_Data->CurrentFrameSlot] : s_Data->Frames[s_Data->CurrentFrameSlot];
    if (s_Data->CurrentTexture != nullptr) { NextBatch(); s_Data->CurrentTexture = nullptr; }
    
    float charSpacing = 6.0f * scale, pixelSize = 1.0f * scale;
    PrismaMath::vec4 tint = {color.r, color.g, color.b, color.a};
    float z = (float)sortingOrder * 0.001f;
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
                    v[0] = {{px, py, z, 1}, tint, {0, 0, 0, 0}};
                    v[1] = {{px + pixelSize, py, z, 1}, tint, {1, 0, 0, 0}};
                    v[2] = {{px + pixelSize, py + pixelSize, z, 1}, tint, {1, 1, 0, 0}};
                    v[3] = {{px, py + pixelSize, z, 1}, tint, {0, 1, 0, 0}};
                    f.VertexBufferPtr += 4; f.QuadCount++; s_Data->Stats.QuadCount++;
                }
            }
        }
        cur.x += charSpacing;
    }
}

float Graphics2D::GetStringWidth(const std::string& text, float scale) {
    return (float)text.length() * 6.0f * scale;
}

void Graphics2D::Execute(ICommandBuffer* cmd, IRenderDevice* device) {
    if (!s_Data || s_Data->BatchQueue.empty()) return;

    auto& f = s_Data->Frames[s_Data->CurrentFrameSlot];
    auto& uif = s_Data->UIFrames[s_Data->CurrentFrameSlot];
    
    if (f.QuadCount > 0)
        f.VBO->UpdateData(f.VertexBufferBase, (uint32_t)(f.VertexBufferPtr - f.VertexBufferBase) * sizeof(Vertex), 0);
    if (uif.QuadCount > 0)
        uif.VBO->UpdateData(uif.VertexBufferBase, (uint32_t)(uif.VertexBufferPtr - uif.VertexBufferBase) * sizeof(Vertex), 0);
}

Graphics2D::Statistics Graphics2D::GetStats() { return s_Data ? s_Data->Stats : Statistics(); }
void Graphics2D::SetLightTexture(const std::shared_ptr<ITexture>& tex) { if (s_Data) s_Data->LightTexture = tex; }

} // namespace Prisma::Graphic
