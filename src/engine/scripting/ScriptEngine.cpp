#include "ScriptEngine.h"
#include "CoreCLRHost.h"
#include "Logger.h"
#include "app/Engine.h"
#include "app/Application.h"
#include <filesystem>
#include "input/InputManager.h"
#include "platform/Platform.h"
#include "core/EntityManager.h"
#include "graphic/2d/LightManager2D.h"
#include "graphic/Renderer2D.h"
#include "graphic/PerspectiveCamera.h"
#include "graphic/RenderSystem.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include <cstring>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>

#ifdef _MSC_VER
#include <windows.h>
#endif

namespace Prisma {
namespace Scripting {

// ============================================================
// 辅助函数：带 SEH 保护的 Bootstrap 调用
// 必须在独立函数中（无 C++ 析构对象），否则 MSVC 禁止 __try/__except
// ============================================================
#ifdef _MSC_VER
static bool TryBootstrap(void (*fn)(void*), void* api, DWORD& outExceptionCode) noexcept {
    __try {
        fn(api);
        return true;
    } __except (outExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
#endif

static thread_local ScriptEngine* s_activeEngine = nullptr;

ScriptEngine::ScriptEngine() {
}

ScriptEngine::~ScriptEngine() {
    Shutdown();
}

uint32_t ScriptEngine::S_CreateEntity() { 
    return EntityManager::Get().CreateNode().handle; 
}

void ScriptEngine::S_DestroyEntity(uint32_t h) { 
    EntityManager::Get().DestroyNode(h); 
}

TransformDataLayout* ScriptEngine::S_GetTransformA() { 
    // C++ Write 缓冲区：C# 脚本往这里写
    return EntityManager::Get().GetTransformWrite();
}

TransformDataLayout* ScriptEngine::S_GetTransformB() { 
    // C++ Read 缓冲区：C# 从这里读取上一帧的已提交数据
    return EntityManager::Get().GetTransformRead();
}

RenderDataLayout* ScriptEngine::S_GetRenderData() { 
    return EntityManager::Get().GetRenderData(); 
}

uint32_t ScriptEngine::S_GetEntityCapacity() { 
    return EntityManager::Get().GetAliveCount(); 
}

void ScriptEngine::S_SetCameraPos(float x, float y) {
    if (s_activeEngine) {
        s_activeEngine->m_cameraPosX = x;
        s_activeEngine->m_cameraPosY = y;
    }
}

void ScriptEngine::S_GetCameraPos(float* x, float* y) { 
    if (s_activeEngine && x && y) { 
        *x = s_activeEngine->m_cameraPosX; 
        *y = s_activeEngine->m_cameraPosY; 
    } 
}

static bool S_IsKeyDown(int k) { auto* m = Engine::Get().GetInputManager(); return m ? m->IsKeyPressed((Prisma::Input::KeyCode)k) : false; }
static float S_GetMouseX() { auto* m = Engine::Get().GetInputManager(); return m ? m->GetMousePosition().x : 0; }
static float S_GetMouseY() {
    auto* m = Engine::Get().GetInputManager();
    if (!m) return 0;
    // [修复] 翻转 Y 轴：SDL 鼠标 Y=0 在顶部，引擎坐标 Y=0 在底部
    auto& spec = Application::Get().GetSpecification();
    return (float)spec.Height - m->GetMousePosition().y;
}
static float S_GetDeltaTime() { return 0.016f; }
static void S_Log(const char* levelStr, const char* msg) {
    if (!levelStr || !msg) return;
    Prisma::LogLevel level = Prisma::LogLevel::Info;
    if (std::strcmp(levelStr, "WARN") == 0)
        level = Prisma::LogLevel::Warning;
    else if (std::strcmp(levelStr, "ERR") == 0 || std::strcmp(levelStr, "ERROR") == 0)
        level = Prisma::LogLevel::Error;
    Prisma::Logger::Get().LogInternal(level, "Script", msg, Prisma::SourceLocation("", 0, ""));
}

// ========== 2D Lighting API ==========

static uint32_t S_CreateLight(int type) {
    return Graphic::LightManager2D::Get().CreateLight(static_cast<Graphic::Light2D::Type>(type));
}

static void S_DestroyLight(uint32_t handle) {
    Graphic::LightManager2D::Get().DestroyLight(handle);
}

static void S_SetLightPos(uint32_t handle, float x, float y) {
    auto* light = Graphic::LightManager2D::Get().GetLight(handle);
    if (light) light->SetPosition({ x, y });
}

static void S_SetLightColor(uint32_t handle, float r, float g, float b) {
    auto* light = Graphic::LightManager2D::Get().GetLight(handle);
    if (light) light->SetColor({ r, g, b });
}

static void S_SetLightIntensity(uint32_t handle, float intensity) {
    auto* light = Graphic::LightManager2D::Get().GetLight(handle);
    if (light) light->SetIntensity(intensity);
}

static void S_SetLightRadius(uint32_t handle, float radius) {
    auto* light = Graphic::LightManager2D::Get().GetLight(handle);
    if (light) light->SetRadius(radius);
}

static void S_SetLightFalloff(uint32_t handle, float falloff) {
    auto* light = Graphic::LightManager2D::Get().GetLight(handle);
    if (light) light->SetFalloffCurve(falloff);
}

static void S_SetLightOrder(uint32_t handle, int order) {
    auto* light = Graphic::LightManager2D::Get().GetLight(handle);
    if (light) light->SetLightOrder(order);
}

static void S_SetLightBlendMode(uint32_t handle, int mode) {
    auto* light = Graphic::LightManager2D::Get().GetLight(handle);
    if (light) light->SetBlendMode(static_cast<Graphic::Light2D::BlendMode>(mode));
}

static void S_DrawGizmoLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a) { Graphic::Renderer2D::DrawLine({x1, y1}, {x2, y2}, {r, g, b, a}); }
static void S_DrawGizmoRect(float x, float y, float w, float h, float r, float g, float b, float a) { Graphic::Renderer2D::DrawRect({x, y}, {w, h}, {r, g, b, a}); }
static void S_DrawGizmoString(const char* t, float x, float y, float s, float r, float g, float b, float a) { if (t) Graphic::Renderer2D::DrawString(t, {x, y}, s, {r, g, b, a}); }

static void S_SetAmbientLight(float r, float g, float b) {
    Graphic::LightManager2D::Get().SetAmbientColor({r, g, b});
}

// ==================== 3D Camera API ====================

static Graphic::PerspectiveCamera* GetMainCamera3D() {
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    if (!scene) return nullptr;
    auto cam = scene->GetMainCamera();
    if (!cam) return nullptr;
    return dynamic_cast<Graphic::PerspectiveCamera*>(cam.get());
}

static void S_SetCamera3DPos(float x, float y, float z) {
    auto* pCam = GetMainCamera3D();
    if (pCam) pCam->SetPosition(glm::vec3(x, y, z));
}

static void S_GetCamera3DPos(float* x, float* y, float* z) {
    if (!x || !y || !z) return;
    auto* pCam = GetMainCamera3D();
    if (pCam) {
        auto pos = pCam->GetPosition();
        *x = pos.x; *y = pos.y; *z = pos.z;
    } else {
        *x = *y = *z = 0.0f;
    }
}

static void S_SetCameraRotation(float pitch, float yaw) {
    auto* pCam = GetMainCamera3D();
    if (!pCam) return;

    // Reconstruct current rotation from camera basis vectors
    glm::vec3 fwd = pCam->GetForward();
    glm::vec3 rgt = pCam->GetRight();
    glm::vec3 upv = pCam->GetUp();
    glm::mat3 rotMat(rgt.x, rgt.y, rgt.z, upv.x, upv.y, upv.z, fwd.x, fwd.y, fwd.z);
    glm::quat currentRot = glm::quat_cast(rotMat);

    // FPS-style: pitch around local X, yaw around world Y
    glm::quat qPitch = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat qYaw   = glm::angleAxis(yaw,   glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat newRot = qYaw * currentRot * qPitch;

    pCam->SetRotation(newRot);
}

static void S_MoveCameraLocal(float forward, float right, float up) {
    auto* pCam = GetMainCamera3D();
    if (!pCam) return;
    glm::vec3 worldDelta = pCam->GetForward() * forward + pCam->GetRight() * right + pCam->GetUp() * up;
    pCam->MoveWorld(worldDelta);
}

// ==================== Enhanced Input API ====================

// Scroll tracking state — SDL doesn't expose scroll as persistent state,
// so we accumulate it during the event loop in ScriptEngine::Update().
static float s_mouseScrollX = 0.0f;
static float s_mouseScrollY = 0.0f;

static float S_GetMouseDeltaX() {
    auto* m = Engine::Get().GetInputManager();
    return m ? m->GetMouseDelta().x : 0.0f;
}

static float S_GetMouseDeltaY() {
    auto* m = Engine::Get().GetInputManager();
    return m ? m->GetMouseDelta().y : 0.0f;
}

static float S_GetMouseScrollX() { return s_mouseScrollX; }
static float S_GetMouseScrollY() { return s_mouseScrollY; }

static void S_SetMouseCapture(bool capture) {
    auto& window = Engine::Get().GetWindow();
    if (window.m_Window) {
        SDL_SetWindowRelativeMouseMode(window.m_Window, capture);
    }
}

static bool S_IsKeyJustPressed(int key) {
    auto* m = Engine::Get().GetInputManager();
    return m ? m->IsKeyJustPressed(static_cast<Prisma::Input::KeyCode>(key)) : false;
}

// ==================== Path Tracing Pipeline Control ====================

static std::shared_ptr<Graphic::PathTracingPipeline> GetPathTracingPipeline() {
    auto* rs = Engine::Get().GetRenderSystem();
    if (!rs) return nullptr;
    return rs->GetMainPipelineAs<Graphic::PathTracingPipeline>();
}

static bool s_ptNEEEnabled = false; // mirror of pipeline's internal m_enableNEE

static void S_PTSetMaxSamples(uint32_t samples) {
    auto pipeline = GetPathTracingPipeline();
    if (pipeline) pipeline->SetMaxSamples(samples);
}

static uint32_t S_PTGetFrameCount() {
    auto pipeline = GetPathTracingPipeline();
    return pipeline ? pipeline->GetFrameCount() : 0;
}

static void S_PTResetAccumulation() {
    auto pipeline = GetPathTracingPipeline();
    if (pipeline) pipeline->ResetAccumulation();
}

static void S_PTSetNEE(bool enabled) {
    s_ptNEEEnabled = enabled;
    auto pipeline = GetPathTracingPipeline();
    if (pipeline) pipeline->EnableNEE(enabled);
}

static bool S_PTGetNEE() { return s_ptNEEEnabled; }

static void S_PTCycleMode() {
    auto pipeline = GetPathTracingPipeline();
    if (pipeline) pipeline->CycleMode();
}

static void S_PTGetModeName(char* buffer, uint32_t bufferSize) {
    if (!buffer || bufferSize == 0) return;
    auto pipeline = GetPathTracingPipeline();
    const char* name = pipeline ? pipeline->GetModeName() : "None";
    size_t len = std::strlen(name);
    size_t copyLen = (len < bufferSize - 1) ? len : (bufferSize - 1);
    std::memcpy(buffer, name, copyLen);
    buffer[copyLen] = '\0';
}

static bool S_PTIsConverged() {
    auto pipeline = GetPathTracingPipeline();
    return pipeline ? pipeline->IsConverged() : false;
}

static uint32_t S_PTGetMaxSamples() {
    auto pipeline = GetPathTracingPipeline();
    return pipeline ? pipeline->GetMaxSamples() : 0;
}

bool ScriptEngine::Initialize(CoreCLRHost& host, const std::string& gameDir) {
    if (m_initialized) return true;
    m_host = &host;
    m_gameDir = gameDir;
    s_activeEngine = this;

    m_api.log = S_Log;
    m_api.createEntity = S_CreateEntity;
    m_api.destroyEntity = S_DestroyEntity;
    m_api.getTransformA = S_GetTransformA;
    m_api.getTransformB = S_GetTransformB;
    m_api.getRenderData = S_GetRenderData;
    m_api.isKeyDown = S_IsKeyDown;
    m_api.getMouseX = S_GetMouseX;
    m_api.getMouseY = S_GetMouseY;
    m_api.getDeltaTime = S_GetDeltaTime;
    m_api.setCameraPos = S_SetCameraPos;
    m_api.getCameraPos = S_GetCameraPos;
    m_api.getEntityCapacity = S_GetEntityCapacity;

    m_api.drawGizmoLine = S_DrawGizmoLine;
    m_api.drawGizmoRect = S_DrawGizmoRect;
    m_api.drawGizmoString = S_DrawGizmoString;

    // Lighting
    m_api.createLight = S_CreateLight;
    m_api.destroyLight = S_DestroyLight;
    m_api.setLightPos = S_SetLightPos;
    m_api.setLightColor = S_SetLightColor;
    m_api.setLightIntensity = S_SetLightIntensity;
    m_api.setLightRadius = S_SetLightRadius;
    m_api.setLightFalloff = S_SetLightFalloff;
    m_api.setLightOrder = S_SetLightOrder;
    m_api.setLightBlendMode = S_SetLightBlendMode;
    m_api.setAmbientLight = S_SetAmbientLight;

    // SRP Graphics API
    m_api.srpCreateShader = SRP_CreateShader;
    m_api.srpDestroyShader = SRP_DestroyShader;
    m_api.srpCreatePipeline = SRP_CreatePipeline;
    m_api.srpDestroyPipeline = SRP_DestroyPipeline;
    m_api.srpCreateRenderTarget = SRP_CreateRenderTarget;
    m_api.srpCreateDepthTarget = SRP_CreateDepthTarget;
    m_api.srpDestroyRenderTarget = SRP_DestroyRenderTarget;
    m_api.srpDestroyDepthTarget = SRP_DestroyDepthTarget;
    m_api.srpCreateVertexBuffer = SRP_CreateVertexBuffer;
    m_api.srpCreateIndexBuffer = SRP_CreateIndexBuffer;
    m_api.srpDestroyBuffer = SRP_DestroyBuffer;
    m_api.srpCreateTexture2D = SRP_CreateTexture2D;
    m_api.srpDestroyTexture = SRP_DestroyTexture;
    m_api.srpCreateSampler = SRP_CreateSampler;
    m_api.srpDestroySampler = SRP_DestroySampler;
    m_api.srpCmdBindTexture = SRP_CmdBindTexture;
    m_api.srpCmdBlitRenderTarget = SRP_CmdBlitRenderTarget;
    m_api.srpBeginFrame = SRP_BeginFrame;
    m_api.srpEndFrame = SRP_EndFrame;
    m_api.srpCmdBeginRenderPass = SRP_CmdBeginRenderPass;
    m_api.srpCmdEndRenderPass = SRP_CmdEndRenderPass;
    m_api.srpCmdBindPipeline = SRP_CmdBindPipeline;
    m_api.srpCmdBindVertexBuffer = SRP_CmdBindVertexBuffer;
    m_api.srpCmdBindIndexBuffer = SRP_CmdBindIndexBuffer;
    m_api.srpCmdSetViewport = SRP_CmdSetViewport;
    m_api.srpCmdSetScissor = SRP_CmdSetScissor;
    m_api.srpCmdPushConstants = SRP_CmdPushConstants;
    m_api.srpCmdDraw = SRP_CmdDraw;
    m_api.srpCmdDrawIndexed = SRP_CmdDrawIndexed;
    m_api.srpCmdDrawFullScreenQuad = SRP_CmdDrawFullScreenQuad;

    // Compute pipeline
    m_api.srpCreateComputePipeline = SRP_CreateComputePipeline;
    m_api.srpDestroyComputePipeline = SRP_DestroyComputePipeline;
    m_api.srpCmdBindComputePipeline = SRP_CmdBindComputePipeline;
    m_api.srpCmdDispatch = SRP_CmdDispatch;
    m_api.srpCmdBindComputeTexture = SRP_CmdBindComputeTexture;
    m_api.srpCmdBindStorageImage = SRP_CmdBindStorageImage;
    m_api.srpCmdBindStorageBuffer = SRP_CmdBindStorageBuffer;

    m_api.srpShutdown = SRP_Shutdown;

    // 3D Camera API
    m_api.setCamera3DPos = S_SetCamera3DPos;
    m_api.getCamera3DPos = S_GetCamera3DPos;
    m_api.setCameraRotation = S_SetCameraRotation;
    m_api.moveCameraLocal = S_MoveCameraLocal;

    // Enhanced Input API
    m_api.getMouseDeltaX = S_GetMouseDeltaX;
    m_api.getMouseDeltaY = S_GetMouseDeltaY;
    m_api.getMouseScrollX = S_GetMouseScrollX;
    m_api.getMouseScrollY = S_GetMouseScrollY;
    m_api.setMouseCapture = S_SetMouseCapture;
    m_api.isKeyJustPressed = S_IsKeyJustPressed;

    // Path Tracing Pipeline Control
    m_api.ptSetMaxSamples = S_PTSetMaxSamples;
    m_api.ptGetFrameCount = S_PTGetFrameCount;
    m_api.ptResetAccumulation = S_PTResetAccumulation;
    m_api.ptSetNEE = S_PTSetNEE;
    m_api.ptGetNEE = S_PTGetNEE;
    m_api.ptCycleMode = S_PTCycleMode;
    m_api.ptGetModeName = S_PTGetModeName;
    m_api.ptIsConverged = S_PTIsConverged;
    m_api.ptGetMaxSamples = S_PTGetMaxSamples;

    const std::string& hostDir = host.GetScriptsDir();

    // 搜索 *_Managed.dll（每个项目命名不同：Prisma2D_Managed.dll / SRP2D_Managed.dll 等）
    std::string gameDll;
    if (std::filesystem::exists(gameDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(gameDir)) {
            auto name = entry.path().filename().string();
            if (name.ends_with("_Managed.dll")) {
                gameDll = name;
                break;
            }
        }
    }

    std::string assemblyPath = gameDir + "/" + gameDll;
    auto dotPos = gameDll.rfind(".dll");
    std::string assemblyName = (dotPos != std::string::npos) ? gameDll.substr(0, dotPos) : gameDll;
    auto managedPos = assemblyName.rfind("_Managed");
    std::string projectPrefix = (managedPos != std::string::npos) ? assemblyName.substr(0, managedPos) : assemblyName;

    auto tryGetFn = [&](const std::string& typePrefix) -> bool {
        std::string type = typePrefix + ".ScriptEntry, " + assemblyName;
        m_bootstrapFn = (void (*)(void*))host.GetFunctionPointer(assemblyPath, type, "Bootstrap");
        m_onFrameFn = (void (*)(float))host.GetFunctionPointer(assemblyPath, type, "OnFrame");
        m_srpRenderFn = (void (*)(float))host.GetFunctionPointer(assemblyPath, type, "OnRender");
        return m_bootstrapFn && m_onFrameFn;
    };

    if (!tryGetFn(projectPrefix) && projectPrefix != "GameScripts")
        tryGetFn("GameScripts");

    if (!m_bootstrapFn || !m_onFrameFn) return false;

    // [诊断] 设置结构体大小，C# 侧校验 C++/C# API 版本一致性
    m_api.structSize = sizeof(PrismaAPI);

#ifdef _MSC_VER
    DWORD exceptionCode = 0;
    if (!TryBootstrap(m_bootstrapFn, &m_api, exceptionCode)) {
        LOG_ERROR("ScriptEngine", "C# Bootstrap 崩溃！异常代码: 0x{0:08X}", exceptionCode);
        LOG_ERROR("ScriptEngine", "可能原因: Prisma.Core.dll 与 C++ PrismaAPI 结构体版本不一致");
        LOG_ERROR("ScriptEngine", "请确保 C++ (ScriptEngine.h) 与 C# (EngineAPI.cs) PrismaAPI 字段完全匹配");
        LOG_ERROR("ScriptEngine", "C++ structSize={0}, 请对比 C# sizeof(PrismaAPI)", sizeof(PrismaAPI));
        LOG_ERROR("ScriptEngine", "然后重新编译 Prisma.Core 与 GameScripts 并部署到输出目录");
        s_activeEngine = nullptr;
        return false;
    }
#else
    m_bootstrapFn(&m_api);
#endif

    m_initialized = true;
    s_activeEngine = nullptr;
    return true;
}

void ScriptEngine::Update(float dt) {
    if (!m_initialized || !m_onFrameFn) return;
    s_activeEngine = this;

    // 每帧重置鼠标滚轮累积，使用 SDL_PumpEvents + SDL_PeepEvents 获取自上次 PollEvent 以来的新滚轮事件
    s_mouseScrollX = 0.0f;
    s_mouseScrollY = 0.0f;
    SDL_PumpEvents();
    SDL_Event ev;
    while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_EVENT_MOUSE_WHEEL, SDL_EVENT_MOUSE_WHEEL) > 0) {
        s_mouseScrollX += ev.wheel.x;
        s_mouseScrollY += ev.wheel.y;
    }
    m_onFrameFn(dt);
    s_activeEngine = nullptr;
}

void ScriptEngine::Render(float dt) {
    if (!m_initialized || !m_srpRenderFn) return;
    s_activeEngine = this;
    m_srpRenderFn(dt);
    s_activeEngine = nullptr;
}

void ScriptEngine::Shutdown() { m_initialized = false; }

} // namespace Scripting
} // namespace Prisma
