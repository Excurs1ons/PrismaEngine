#include "Template3DApp.h"

#include "graphic/RenderSystem.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/RenderDesc.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "app/Engine.h"
#include "core/EntityManager.h"
#include "platform/Platform.h"
#include "utils/ImageUtils.h"
#include "core/AssetManager.h"
#include "Logger.h"
#include "graphic/MeshRenderer.h"
#include "transform/Transform.h"

#include <SDL3/SDL_scancode.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include "scene/Scene.h"
#include "scene/SceneManager.h"


namespace Prisma {
using namespace Graphic;

// Gizmo push constants
namespace {
struct alignas(16) GizmoPushConstants {
    PrismaMath::mat4 mvp;
    Prisma::Color color;
};
}

// ============================================================================
// Template3DApp
// ============================================================================

Template3DApp::Template3DApp()
    : Application({"Template3D", "", 1280, 720, false, true, Graphic::PresentMode::Mailbox, 0})
{
}

Template3DApp::~Template3DApp() {
}

void Template3DApp::InitGizmoResources() {
    auto rm = Engine::Get().GetRenderResourceManager();
    if (!rm || !m_device) return;

    auto* factory = m_device->GetResourceFactory();
    if (!factory) return;

    m_gizmoVertShader = rm->LoadShaderSync("assets/shaders/Renderer2D.vert.spv");
    m_gizmoFragShader = rm->LoadShaderSync("assets/shaders/UnlitVertex.frag.spv");
    if (!m_gizmoVertShader || !m_gizmoFragShader) {
        LOG_ERROR("Template3D", "gizmo shader 加载失败");
        return;
    }

    auto pso = factory->CreatePipelineStateImpl();
    pso->SetShader(ShaderType::Vertex, m_gizmoVertShader);
    pso->SetShader(ShaderType::Pixel, m_gizmoFragShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    RasterizerState rs{};
    rs.cullMode = CullMode::None;
    pso->SetRasterizerState(rs);
    if (pso->Create(m_device)) {
        m_gizmoPSO = std::shared_ptr<IPipelineState>(std::move(pso));
        LOG_INFO("Template3D", "gizmo PSO 创建成功");
    } else {
        LOG_ERROR("Template3D", "gizmo PSO 创建失败");
    }

    m_gizmoCamera = std::make_shared<OrthographicCamera>(
        0.0f, static_cast<float>(m_Spec.Width), 0.0f, static_cast<float>(m_Spec.Height)
    );
}

void Template3DApp::ProcessGizmoOverlay(ICommandBuffer* cmd) {
    const auto& gizmoCommands = Renderer::GetGizmoQueue();
    if (gizmoCommands.empty() || !m_gizmoPSO) return;

    cmd->SetPipelineState(m_gizmoPSO.get());

    float w = static_cast<float>(m_Spec.Width);
    float h = static_cast<float>(m_Spec.Height);
    cmd->SetViewport(Viewport{0.0f, 0.0f, w, h, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, static_cast<int>(w), static_cast<int>(h)});

    auto vp = m_gizmoCamera ? m_gizmoCamera->GetViewProjectionMatrix() : glm::mat4(1.0f);

    for (const auto& gc : gizmoCommands) {
        if (!gc.mesh) continue;
        GizmoPushConstants pc{};
        pc.mvp = vp * gc.transform;
        pc.color = gc.color;
        cmd->PushConstants(ShaderType::Vertex, &pc, sizeof(pc));
        cmd->PushConstants(ShaderType::Pixel, &pc, sizeof(pc));

        for (const auto& subMesh : gc.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                cmd->DrawIndexed(subMesh.indexCount);
            }
        }
    }

    Renderer::ClearGizmoQueue();
}

void Template3DApp::BuildPathTracingScene() {
    auto* sceneManager = Engine::Get().GetSceneManager();
    if (!sceneManager) {
        LOG_ERROR("Template3D", "无法获取 SceneManager");
        return;
    }
    auto* scene = sceneManager->GetCurrentScene();
    if (!scene) {
        LOG_ERROR("Template3D", "没有当前场景");
        return;
    }

    // 从场景相机配置更新相机
    const auto& camConfig = scene->GetCameraConfig();
    m_camera.position = glm::vec3(camConfig.position.x, camConfig.position.y, camConfig.position.z);
    m_camera.target   = glm::vec3(camConfig.target.x,   camConfig.target.y,   camConfig.target.z);
    m_camera.fov      = camConfig.fov;
    LOG_INFO("Template3D", "相机: pos=({:.1f},{:.1f},{:.1f}) target=({:.1f},{:.1f},{:.1f}) fov={:.1f}",
             m_camera.position.x, m_camera.position.y, m_camera.position.z,
             m_camera.target.x, m_camera.target.y, m_camera.target.z, m_camera.fov);

    SceneDataSSBO ssboData{};
    uint32_t objCount = 0;

    for (const auto& node : scene->GetNodes()) {
        if (objCount >= 32) {
            LOG_WARN("Template3D", "超出最大对象数(32)，忽略剩余节点");
            break;
        }

        auto transform = scene->GetComponent<Transform>(node);
        auto meshRenderer = scene->GetComponent<Graphic::MeshRenderer>(node);
        if (!transform || !meshRenderer) continue;

        auto mesh = meshRenderer->GetMesh();
        if (!mesh) continue;

        auto bb = mesh->GetBoundingBox();
        float meshHx = (bb.maxBounds.x - bb.minBounds.x) * 0.5f;
        float meshHy = (bb.maxBounds.y - bb.minBounds.y) * 0.5f;
        float meshHz = (bb.maxBounds.z - bb.minBounds.z) * 0.5f;

        auto renderData = meshRenderer->GetData();
        auto meshPath = renderData.meshPath;
        auto& ptObj = ssboData.objects[objCount];

        ptObj.color[0] = renderData.color[0];
        ptObj.color[1] = renderData.color[1];
        ptObj.color[2] = renderData.color[2];
        ptObj.color[3] = renderData.emissive[0]; // SSBO stores single emissive float

        auto pos = transform->GetPosition();
        auto scale = transform->GetScale();
        auto rot = transform->GetRotation();

        if (meshPath.find("plane") != std::string::npos) {
            ptObj.p0[0] = pos.x;
            ptObj.p0[1] = pos.y;
            ptObj.p0[2] = pos.z;
            ptObj.p0[3] = 0.0f; // type: plane

            glm::vec3 normal = rot * glm::vec3(0.0f, 1.0f, 0.0f);
            ptObj.p1[0] = normal.x;
            ptObj.p1[1] = normal.y;
            ptObj.p1[2] = normal.z;
            ptObj.p1[3] = 0.0f;

            ptObj.p2[0] = -meshHx * scale.x;
            ptObj.p2[1] = -meshHz * scale.z;
            ptObj.p2[2] =  meshHx * scale.x;
            ptObj.p2[3] =  meshHz * scale.z;
        } else if (meshPath.find("sphere") != std::string::npos) {
            ptObj.p0[0] = pos.x;
            ptObj.p0[1] = pos.y;
            ptObj.p0[2] = pos.z;
            ptObj.p0[3] = 1.0f; // type: sphere

            float radius = meshHx * scale.x;
            ptObj.p1[0] = radius;
            ptObj.p1[1] = 0.0f;
            ptObj.p1[2] = 0.0f;
            ptObj.p1[3] = 0.0f;

            ptObj.p2[0] = 0.0f;
            ptObj.p2[1] = 0.0f;
            ptObj.p2[2] = 0.0f;
            ptObj.p2[3] = 0.0f;
        } else if (meshPath.find("cube") != std::string::npos) {
            ptObj.p0[0] = pos.x;
            ptObj.p0[1] = pos.y;
            ptObj.p0[2] = pos.z;
            ptObj.p0[3] = 2.0f; // type: box

            ptObj.p1[0] = meshHx * scale.x;
            ptObj.p1[1] = meshHy * scale.y;
            ptObj.p1[2] = meshHz * scale.z;
            ptObj.p1[3] = 0.0f;

            // 从四元数提取 Y 轴旋转
            float angle = 2.0f * std::atan2(rot.y, rot.w);
            ptObj.p2[0] = std::cos(angle);
            ptObj.p2[1] = std::sin(angle);
            ptObj.p2[2] = 0.0f;
            ptObj.p2[3] = 0.0f;
        } else if (meshPath.find("cone") != std::string::npos) {
            ptObj.p0[0] = pos.x;
            ptObj.p0[1] = pos.y;
            ptObj.p0[2] = pos.z;
            ptObj.p0[3] = 3.0f; // type: cone

            ptObj.p1[0] = meshHx * scale.x; // radius (cone base)
            ptObj.p1[1] = meshHy * scale.y; // halfHeight
            ptObj.p1[2] = 0.0f;
            ptObj.p1[3] = 0.0f;

            // Y 轴旋转
            float angle = 2.0f * std::atan2(rot.y, rot.w);
            ptObj.p2[0] = std::cos(angle);
            ptObj.p2[1] = std::sin(angle);
            ptObj.p2[2] = 0.0f;
            ptObj.p2[3] = 0.0f;
        } else {
            LOG_WARN("Template3D", "未知网格类型 '{}'，跳过", meshPath);
            continue;
        }

        objCount++;
    }

    ssboData.objectCount = (int)objCount;

    // 设置场景数据到管线
    if (m_ptPipeline) {
        Graphic::PathTracingSceneData ptData;
        ptData.objectCount = ssboData.objectCount;
        std::memcpy(ptData.objects, ssboData.objects, sizeof(PTSceneObject) * objCount);
        m_ptPipeline->SetSceneData(ptData);
    }

    m_sceneLoaded = true;
    LOG_INFO("Template3D", "场景 GetNodes()={} 个节点, 找到 {} 个可渲染对象", scene->GetNodes().size(), objCount);
    if (objCount == 0) {
        LOG_ERROR("Template3D", "没有找到任何可渲染对象！可能原因：组件未正确附加或网格加载失败");
        for (const auto& n : scene->GetNodes()) {
            auto t = scene->GetComponent<Transform>(n);
            auto m = scene->GetComponent<Graphic::MeshRenderer>(n);
            LOG_INFO("Template3D", "  节点 '{}': Transform={}, MeshRenderer={}, Mesh={}",
                scene->GetNodeName(n),
                t ? "有" : "无",
                m ? "有" : "无",
                m && m->GetMesh() ? "有" : "无");
        }
    }
}

int Template3DApp::OnInitialize() {
    LOG_INFO("Template3D", "3D 模板初始化（路径追踪引擎管线版）");

    if (m_headlessCfg.enabled) {
        m_Spec.Width = m_headlessCfg.width;
        m_Spec.Height = m_headlessCfg.height;
        LOG_INFO("Template3D", "头模式分辨率: {}x{}", m_Spec.Width, m_Spec.Height);
    }

    m_device = Engine::Get().GetRenderSystem()->GetDevice();
    if (!m_device) {
        LOG_ERROR("Template3D", "无法获取渲染设备");
        return -1;
    }

    // 优先从文件加载最新的着色器 SPIR-V，实现热更新支持
    auto ptPipeline = std::make_shared<PathTracingPipeline>();

    auto loadShaderData = [&](const std::string& path) -> std::vector<uint32_t> {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<uint32_t> data(size / 4);
            file.read(reinterpret_cast<char*>(data.data()), size);
            LOG_INFO("Template3D", "已从文件加载着色器: {}", path);
            return data;
        }
        LOG_WARN("Template3D", "无法读取着色器文件 {}，回退到嵌入版本", path);
        throw;
    };

    std::vector<uint32_t> compSPV = loadShaderData("assets/shaders/pathtrace.comp.spv");
    std::vector<uint32_t> vertSPV = loadShaderData("assets/shaders/fullscreen.vert.spv");
    std::vector<uint32_t> fragSPV = loadShaderData("assets/shaders/present.frag.spv");

    ptPipeline->SetComputeShaderSPIRV(compSPV.data(), compSPV.size() * 4);
    ptPipeline->SetPresentShadersSPIRV(
        vertSPV.data(), vertSPV.size() * 4,
        fragSPV.data(), fragSPV.size() * 4
    );

    // 设置 overlay 回调（gizmo/HUD）
    ptPipeline->SetOverlayCallback([this](ICommandBuffer* cmd) {
        OnPresentOverlay(cmd);
    });

    // 初始化管线
    if (ptPipeline->Initialize(m_device) != 0) {
        LOG_ERROR("Template3D", "路径追踪管线初始化失败");
        return -1;
    }
    m_ptPipeline = ptPipeline;

    // 初始化 gizmo overlay
    InitGizmoResources();

    // Forward 资源（Cornell Box 网格，仍然保留为切换选项）
    InitForwardResources();

    // 通过 SceneManager 加载场景（新 ECS 场景系统）
    auto* sceneManager = Engine::Get().GetSceneManager();
    if (sceneManager) {
        sceneManager->LoadFromFile("assets/scenes/cornell_box.scene.json");
        BuildPathTracingScene();
    }

    m_ptPipeline->SetMaxSamples(m_ptMaxSamples);

    LOG_INFO("Template3D", "按 P 切换渲染模式，R 重置路径追踪累积");
    return 0;
}

void Template3DApp::InitForwardResources() {
    if (!m_device) return;
    auto* factory = m_device->GetResourceFactory();
    if (!factory) return;

    struct FaceInput {
        glm::vec3 v0, v1, v2, v3;
        glm::vec4 color;
    };

    std::vector<Graphic::Vertex> vertices;
    std::vector<uint16_t> indices;

    auto addFace = [&](const FaceInput& f) {
        uint16_t base = static_cast<uint16_t>(vertices.size());
        vertices.emplace_back(glm::vec4(f.v0, 1.0f), f.color, glm::vec4(0), glm::vec4(0), glm::vec4(0), glm::vec4(0));
        vertices.emplace_back(glm::vec4(f.v1, 1.0f), f.color, glm::vec4(0), glm::vec4(0), glm::vec4(0), glm::vec4(0));
        vertices.emplace_back(glm::vec4(f.v2, 1.0f), f.color, glm::vec4(0), glm::vec4(0), glm::vec4(0), glm::vec4(0));
        vertices.emplace_back(glm::vec4(f.v3, 1.0f), f.color, glm::vec4(0), glm::vec4(0), glm::vec4(0), glm::vec4(0));
        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
    };

    auto addBox = [&](const glm::vec3& min, const glm::vec3& max, const glm::vec4& color) {
        addFace({glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z),
                 glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, min.y, max.z), color});
        addFace({glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z),
                 glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, min.y, min.z), color});
        addFace({glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z),
                 glm::vec3(max.x, min.y, min.z), glm::vec3(min.x, min.y, min.z), color});
        addFace({glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z),
                 glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z), color});
        addFace({glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z),
                 glm::vec3(max.x, max.y, min.z), glm::vec3(min.x, max.y, min.z), color});
        addFace({glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z),
                 glm::vec3(min.x, max.y, max.z), glm::vec3(max.x, max.y, max.z), color});
    };

    glm::vec4 white(0.7f, 0.7f, 0.7f, 1.0f);
    glm::vec4 red(0.8f, 0.05f, 0.05f, 1.0f);
    glm::vec4 green(0.05f, 0.5f, 0.05f, 1.0f);
    glm::vec4 lightCol(15.0f, 15.0f, 15.0f, 1.0f);

    addFace({{-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1}, white});
    addFace({{-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1}, {-1, 1,-1}, red});
    addFace({{ 1,-1, 1}, { 1,-1,-1}, { 1, 1,-1}, { 1, 1, 1}, green});
    addFace({{-1, 1,-1}, { 1, 1,-1}, { 1, 1, 1}, {-1, 1, 1}, white});
    addFace({{-1,-1, 1}, { 1,-1, 1}, { 1,-1,-1}, {-1,-1,-1}, white});
    addFace({{-0.3f, 0.999f, -0.3f}, { 0.3f, 0.999f, -0.3f},
             { 0.3f, 0.999f,  0.3f}, {-0.3f, 0.999f,  0.3f}, lightCol});
    addBox(glm::vec3(-0.6f, -1.0f, -0.15f), glm::vec3(-0.3f, -0.4f, 0.15f), red);
    addBox(glm::vec3( 0.3f, -1.0f, -0.15f), glm::vec3( 0.6f, -0.7f, 0.15f), green);

    BufferDesc vbDesc{};
    vbDesc.type = BufferType::Vertex;
    vbDesc.size = static_cast<uint32_t>(vertices.size() * sizeof(Graphic::Vertex));
    vbDesc.usage = BufferUsage::Default;
    m_cornellBoxVB = factory->CreateBufferImpl(vbDesc);
    if (m_cornellBoxVB) m_cornellBoxVB->UpdateData(vertices.data(), vbDesc.size, 0);

    BufferDesc ibDesc{};
    ibDesc.type = BufferType::Index;
    ibDesc.size = static_cast<uint32_t>(indices.size() * sizeof(uint16_t));
    ibDesc.usage = BufferUsage::Default;
    m_cornellBoxIB = factory->CreateBufferImpl(ibDesc);
    if (m_cornellBoxIB) m_cornellBoxIB->UpdateData(indices.data(), ibDesc.size, 0);

    m_cornellBoxIndexCount = static_cast<uint32_t>(indices.size());
    LOG_INFO("Template3D", "Cornell Box 网格: {} 顶点, {} 索引", vertices.size(), indices.size());
}

void Template3DApp::OnRender() {
    if (m_renderMode == RenderMode::PathTracing && m_ptPipeline) {
        RenderPathTracing();
    } else {
        RenderForward3D();
    }
}

void Template3DApp::RenderPathTracing() {
    if (!m_ptPipeline || !m_device) return;

    // 头模式：累积到指定帧数后保存退出
    if (m_headlessCfg.enabled) {
        uint32_t frame = m_ptPipeline->GetFrameCount();
        if (frame >= m_headlessCfg.totalFrames) return;
    }

    // 构建渲染上下文
    RenderContext ctx;
    ctx.device = m_device;
    ctx.commandBuffer = m_device->GetCurrentCommandBuffer();
    ctx.width = m_Spec.Width;
    ctx.height = m_Spec.Height;
    ctx.frameIndex = m_device->GetCurrentFrameIndex();

    // 从 Template3D 的自定义相机计算 view matrix
    glm::vec3 dir = glm::normalize(m_camera.target - m_camera.position);
    glm::vec3 right = glm::normalize(glm::cross(dir, m_camera.up));
    glm::vec3 up = glm::normalize(glm::cross(right, dir));
    ctx.camera.viewMatrix = glm::lookAt(m_camera.position, m_camera.target, m_camera.up);
    ctx.camera.projectionMatrix = glm::perspective(
        glm::radians(m_camera.fov),
        (float)m_Spec.Width / (float)m_Spec.Height, 0.1f, 100.0f
    );
    ctx.camera.position = m_camera.position;
    ctx.camera.fov = m_camera.fov;

    // 排队 stats 文字四边形，由 OnPresentOverlay 回调在 swapchain RP 内渲染
    if (m_gizmoCamera) {
        Renderer2D::BeginGizmo(*m_gizmoCamera);
        DrawStatsOverlay();
        Renderer2D::EndGizmo();
    }

    // 执行管线（内部结束/重开 swapchain RP → 全屏四边形 → overlay 回调渲染 gizmo+文字）
    m_ptPipeline->Execute(ctx);
}

void Template3DApp::OnPresentOverlay(ICommandBuffer* cmd) {
    // 在 swapchain render pass 中绘制 gizmo/HUD
    ProcessGizmoOverlay(cmd);
}

void Template3DApp::SavePathTracingOutput() {
    if (m_ptPipeline) {
        m_ptPipeline->SaveOutput(m_headlessCfg.outputPath);
    }
}

void Template3DApp::DrawStatsOverlay() {
    float winW = static_cast<float>(m_Spec.Width);
    float winH = static_cast<float>(m_Spec.Height);

    static std::string timingInfo = "Calculating...";
    static std::string statusStr = "Loading...";
    static std::string resInfo = "";
    static Prisma::Color statusColor = {0.2f, 1.0f, 0.2f, 1.0f};
    static float refreshTimer = 0.0f;
    static double lastTime = 0.0;

    double now = Platform::GetTimeSeconds();
    float dt = (lastTime > 0.0) ? static_cast<float>(now - lastTime) : 0.016f;
    lastTime = now;

    uint32_t totalNodes = EntityManager::Get().GetAliveCount();
    const auto& st = Engine::Get().GetFrameStats();
    refreshTimer += dt;

    if (refreshTimer >= 1.0f) {
        char buf[256];
        snprintf(buf, sizeof(buf), "BF=%.2f Render=%.2f EF=%.2f Present=%.2f Total=%.2f (ms)",
                 st.BeginFrameTime, st.RenderTime, st.EndFrameTime, st.PresentTime, st.TotalTime);
        timingInfo = buf;

        double maxTime = st.BeginFrameTime;
        std::string leadStage = "BF";
        if (st.RenderTime > maxTime) { maxTime = st.RenderTime; leadStage = "Render(CPU)"; }
        if (st.EndFrameTime > maxTime) { maxTime = st.EndFrameTime; leadStage = "EF(GPU)"; }
        if (st.PresentTime > maxTime) { maxTime = st.PresentTime; leadStage = "Present"; }
        if (st.TotalTime < 2.0) {
            statusStr = "Status: Balanced (Lead: " + leadStage + ")";
            statusColor = {0.2f, 1.0f, 0.2f, 1.0f};
        } else {
            statusStr = "Status: LEAD " + leadStage;
            statusColor = (leadStage.find("CPU") != std::string::npos)
                              ? Prisma::Color{1.0f, 0.2f, 0.8f, 1.0f}
                              : Prisma::Color{1.0f, 0.2f, 0.2f, 1.0f};
        }

        resInfo = std::to_string(m_Spec.Width) + "x" + std::to_string(m_Spec.Height)
                + " @ " + std::to_string(static_cast<int>(Engine::Get().GetFPS())) + " FPS";
        refreshTimer = 0.0f;
    }

    Renderer2D::DrawString(timingInfo, {30.0f, winH - 45.0f}, 1.5f, {0.2f, 1.0f, 0.2f, 1.0f});
    Renderer2D::DrawString(statusStr, {30.0f, winH - 85.0f}, 1.5f, statusColor);

    float resW = Renderer2D::GetStringWidth(resInfo, 3.0f);
    Renderer2D::DrawString(resInfo, {winW - resW - 30.0f, winH - 50.0f}, 3.0f, {0.4f, 0.7f, 0.4f, 1.0f});

    std::string gpuName = Engine::Get().GetGPUName();
    if (!gpuName.empty()) {
        float gW = Renderer2D::GetStringWidth(gpuName, 2.0f);
        Renderer2D::DrawString(gpuName, {winW - gW - 30.0f, winH - 95.0f}, 2.0f, {0.5f, 0.5f, 0.5f, 1.0f});
    }

    float escW = Renderer2D::GetStringWidth("ESC to exit", 2.0f);
    Renderer2D::DrawString("ESC to exit", {winW - escW - 30.0f, 30.0f}, 2.0f, {0.4f, 0.4f, 0.4f, 1.0f});
    Renderer2D::DrawString("Template3D (Nodes: " + std::to_string(totalNodes) + ")",
                           {30.0f, 30.0f}, 2.0f, {0.6f, 0.6f, 0.6f, 1.0f});

    std::string modeStr = (m_renderMode == RenderMode::PathTracing) ? "PathTracing" : "Forward3D";
    Renderer2D::DrawString("Mode: " + modeStr + "  [P] Switch  [R] Reset  [ -Samples+ ]",
                           {30.0f, 65.0f}, 1.5f, {0.6f, 0.6f, 0.9f, 1.0f});
    Renderer2D::DrawString(
        "Cam: (" + std::to_string(static_cast<int>(m_camera.position.x)) + ", "
                 + std::to_string(static_cast<int>(m_camera.position.y)) + ", "
                 + std::to_string(static_cast<int>(m_camera.position.z)) + ")",
        {30.0f, 95.0f}, 1.5f, {0.6f, 0.6f, 0.9f, 1.0f});

    if (m_renderMode == RenderMode::PathTracing && m_ptPipeline) {
        uint32_t frameCount = m_ptPipeline->GetFrameCount();
        std::string ptInfo;
        glm::vec4 ptColor;
        if (m_ptPipeline->IsConverged()) {
            ptInfo = "Converged: " + std::to_string(frameCount)
                   + "/" + std::to_string(m_ptMaxSamples) + " samples"
                   + "  |  " + std::to_string(m_Spec.Width) + "x" + std::to_string(m_Spec.Height);
            ptColor = {0.2f, 1.0f, 0.2f, 1.0f};
        } else {
            std::string maxStr = m_ptMaxSamples > 0 ? "/" + std::to_string(m_ptMaxSamples) : "+";
            ptInfo = "PathTrace: " + std::to_string(frameCount)
                   + maxStr + " samples"
                   + "  |  " + std::to_string(m_Spec.Width) + "x" + std::to_string(m_Spec.Height);
            ptColor = {0.9f, 0.6f, 0.2f, 1.0f};
        }
        Renderer2D::DrawString(ptInfo, {30.0f, 130.0f}, 1.5f, ptColor);
    }
}

void Template3DApp::RenderForward3D() {
    if (!m_gizmoCamera) return;
    Renderer2D::BeginGizmo(*m_gizmoCamera);
    DrawStatsOverlay();
    Renderer2D::EndGizmo();
}

void Template3DApp::OnUpdate(Timestep ts) {
    (void)ts;
    if (m_headlessCfg.enabled && m_ptPipeline) {
        if (m_ptPipeline->GetFrameCount() >= m_headlessCfg.totalFrames) {
            LOG_INFO("Template3D", "headless模式完成，保存输出...");
            SavePathTracingOutput();
            Close();
        }
    }
}

void Template3DApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        OnWindowResize(ev.GetWidth(), ev.GetHeight());
        return false;
    });
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) {
            Close();
            return true;
        }
        if (ev.GetKeyCode() == SDL_SCANCODE_P && !ev.IsRepeat()) {
            // P 键仅在 PathTracing 模式重置累积（Forward3D 未实现完整渲染）
            if (m_renderMode == RenderMode::PathTracing && m_ptPipeline) {
                m_pathTracingDirty = true;
                m_ptPipeline->ResetAccumulation();
                LOG_INFO("Template3D", "重置路径追踪累积");
            }
            return true;
        }
        if (ev.GetKeyCode() == SDL_SCANCODE_R && !ev.IsRepeat()) {
            m_pathTracingDirty = true;
            if (m_ptPipeline) m_ptPipeline->ResetAccumulation();
            LOG_INFO("Template3D", "重置路径追踪累积");
            return true;
        }
        if (ev.GetKeyCode() == SDL_SCANCODE_N && !ev.IsRepeat() && m_renderMode == RenderMode::PathTracing) {
            m_enableNEE = !m_enableNEE;
            if (m_ptPipeline) {
                m_ptPipeline->EnableNEE(m_enableNEE);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("Template3D", "NEE {}", m_enableNEE ? "启用" : "禁用");
            return true;
        }
        // [ / ] 调整收敛帧数上限
        if (ev.GetKeyCode() == SDL_SCANCODE_LEFTBRACKET && !ev.IsRepeat()) {
            m_ptMaxSamples = (m_ptMaxSamples > 16) ? m_ptMaxSamples - 16 : 0;
            if (m_ptPipeline) {
                m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("Template3D", "最大采样帧数: {}", m_ptMaxSamples);
            return true;
        }
        if (ev.GetKeyCode() == SDL_SCANCODE_RIGHTBRACKET && !ev.IsRepeat()) {
            m_ptMaxSamples = std::min(m_ptMaxSamples + 16, 4096u);
            if (m_ptPipeline) {
                m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("Template3D", "最大采样帧数: {}", m_ptMaxSamples);
            return true;
        }
        return false;
    });
}

void Template3DApp::OnWindowResize(uint32_t w, uint32_t h) {
    m_Spec.Width = w;
    m_Spec.Height = h;

    if (m_gizmoCamera) {
        m_gizmoCamera->SetProjection(0.0f, static_cast<float>(w), 0.0f, static_cast<float>(h));
    }

    m_pathTracingDirty = true;
}

void Template3DApp::OnShutdown() {
    // 路径追踪管线由 RenderSystem 管理（如果是主管线）或 shared_ptr 自动析构
    m_ptPipeline.reset();
    m_gizmoPSO.reset();

    LOG_INFO("Template3D", "应用已关闭");
}

} // namespace Prisma
