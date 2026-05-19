#include "Template3DApp.h"
#include "FullscreenVertSPIRV.h"
#include "PresentFragSPIRV.h"
#include "PathtraceCompSPIRV.h"

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
#include "Logger.h"

#include <SDL3/SDL_scancode.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glaze/glaze.hpp>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>

// JSON scene structs for Glaze deserialization (global scope)
struct CameraConfig {
    std::array<double, 3> position = {0.0, 0.0, 2.5};
    double fov = 70.0;
};

struct ObjectConfig {
    std::string type = "plane";
    std::array<double, 3> point = {0.0, 0.0, 0.0};
    std::array<double, 3> normal = {0.0, 1.0, 0.0};
    std::array<double, 4> bounds = {-1.0, -1.0, 1.0, 1.0};
    std::array<double, 3> center = {0.0, 0.0, 0.0};
    std::array<double, 3> halfSize = {0.2, 0.2, 0.2};
    double radius = 0.25;
    double rotation = 0.0;
    std::array<double, 3> axis = {0.0, 1.0, 0.0};
    std::array<double, 3> color = {0.5, 0.5, 0.5};
    double emissive = 0.0;
};

struct SceneConfig {
    int version = 1;
    CameraConfig camera;
    std::vector<ObjectConfig> objects;
};

namespace glz {
template<> struct meta<CameraConfig> {
    using T = CameraConfig;
    static constexpr auto value = object(
        &T::position, &T::fov
    );
};

template<> struct meta<ObjectConfig> {
    using T = ObjectConfig;
    static constexpr auto value = object(
        &T::type, &T::point, &T::normal, &T::bounds,
        &T::center, &T::halfSize, &T::radius, &T::rotation,
        &T::axis, &T::color, &T::emissive
    );
};

template<> struct meta<SceneConfig> {
    using T = SceneConfig;
    static constexpr auto value = object(
        &T::version, &T::camera, &T::objects
    );
};
} // namespace glz

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

void Template3DApp::LoadSceneFromJSON(const std::string& path) {
    LOG_INFO("Template3D", "加载场景文件: {}", path);

    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_ERROR("Template3D", "无法打开场景文件: {}", path);
        return;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    std::string jsonStr = buf.str();

    SceneConfig cfg;
    auto ec = glz::read_json(cfg, jsonStr);
    if (ec) {
        LOG_ERROR("Template3D", "场景 JSON 解析失败: {}", glz::format_error(ec, jsonStr));
        return;
    }

    m_camera.position = glm::vec3(
        (float)cfg.camera.position[0],
        (float)cfg.camera.position[1],
        (float)cfg.camera.position[2]
    );
    m_camera.fov = (float)cfg.camera.fov;
    LOG_INFO("Template3D", "  相机: pos=({:.1f},{:.1f},{:.1f}) fov={:.1f}",
             m_camera.position.x, m_camera.position.y, m_camera.position.z, m_camera.fov);

    SceneDataSSBO ssboData{};
    uint32_t objCount = std::min((uint32_t)cfg.objects.size(), (uint32_t)32);
    ssboData.objectCount = (int)objCount;

    for (uint32_t i = 0; i < objCount; i++) {
        auto& obj = cfg.objects[i];
        auto& ptObj = ssboData.objects[i];

        if (obj.type == "plane") {
            ptObj.p0[0] = (float)obj.point[0];
            ptObj.p0[1] = (float)obj.point[1];
            ptObj.p0[2] = (float)obj.point[2];
            ptObj.p0[3] = 0.0f;
            ptObj.p1[0] = (float)obj.normal[0];
            ptObj.p1[1] = (float)obj.normal[1];
            ptObj.p1[2] = (float)obj.normal[2];
            ptObj.p1[3] = 0.0f;
            ptObj.p2[0] = (float)obj.bounds[0];
            ptObj.p2[1] = (float)obj.bounds[1];
            ptObj.p2[2] = (float)obj.bounds[2];
            ptObj.p2[3] = (float)obj.bounds[3];
        } else if (obj.type == "sphere") {
            ptObj.p0[0] = (float)obj.center[0];
            ptObj.p0[1] = (float)obj.center[1];
            ptObj.p0[2] = (float)obj.center[2];
            ptObj.p0[3] = 1.0f;
            ptObj.p1[0] = (float)obj.radius;
            ptObj.p1[1] = 0.0f;
            ptObj.p1[2] = 0.0f;
            ptObj.p1[3] = 0.0f;
        } else if (obj.type == "box") {
            ptObj.p0[0] = (float)obj.center[0];
            ptObj.p0[1] = (float)obj.center[1];
            ptObj.p0[2] = (float)obj.center[2];
            ptObj.p0[3] = 2.0f;
            ptObj.p1[0] = (float)obj.halfSize[0];
            ptObj.p1[1] = (float)obj.halfSize[1];
            ptObj.p1[2] = (float)obj.halfSize[2];
            ptObj.p1[3] = 0.0f;
            float angleRad = glm::radians((float)obj.rotation);
            ptObj.p2[0] = cos(angleRad);
            ptObj.p2[1] = sin(angleRad);
            ptObj.p2[2] = 0.0f;
            ptObj.p2[3] = 0.0f;
        } else {
            LOG_WARN("Template3D", "  未知类型 '{}'，跳过", obj.type);
            ssboData.objectCount--;
            continue;
        }

        ptObj.color[0] = (float)obj.color[0];
        ptObj.color[1] = (float)obj.color[1];
        ptObj.color[2] = (float)obj.color[2];
        ptObj.color[3] = (float)obj.emissive;
    }

    // 设置场景数据到管线
    if (m_ptPipeline) {
        Graphic::PathTracingSceneData ptData;
        ptData.objectCount = ssboData.objectCount;
        std::memcpy(ptData.objects, ssboData.objects, sizeof(PTSceneObject) * objCount);
        m_ptPipeline->SetSceneData(ptData);
    }

    m_sceneLoaded = true;
    LOG_INFO("Template3D", "场景加载完成，{} 个对象", objCount);
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

    // 创建路径追踪管线
    auto ptPipeline = std::make_shared<PathTracingPipeline>();

    // 传递着色器 SPIR-V 数据（嵌入在 SPIRV header 中）
    ptPipeline->SetComputeShaderSPIRV(PATHTRACE_COMP_SPV_SPV, PATHTRACE_COMP_SPV_SPV_SIZE * sizeof(uint32_t));
    ptPipeline->SetPresentShadersSPIRV(
        FULLSCREEN_VERT_SPV_SPV, FULLSCREEN_VERT_SPV_SPV_SIZE * sizeof(uint32_t),
        PRESENT_FRAG_SPV_SPV, PRESENT_FRAG_SPV_SPV_SIZE * sizeof(uint32_t)
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

    // 加载场景
    std::string scenePath = "assets/scenes/pt_scene.json";
    LOG_INFO("Template3D", "工作目录: {}", std::filesystem::current_path().string());
    LoadSceneFromJSON(scenePath);

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

    // 执行管线
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
    Renderer2D::DrawString("Mode: " + modeStr + "  [P] Switch  [R] Reset",
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
            LOG_INFO("Template3D", "头模式完成，保存输出...");
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
            m_renderMode = (m_renderMode == RenderMode::PathTracing)
                           ? RenderMode::Forward3D
                           : RenderMode::PathTracing;
            m_pathTracingDirty = true;
            if (m_ptPipeline) m_ptPipeline->ResetAccumulation();
            LOG_INFO("Template3D", "切换到 {} 模式",
                     m_renderMode == RenderMode::PathTracing ? "路径追踪" : "Forward3D");
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
