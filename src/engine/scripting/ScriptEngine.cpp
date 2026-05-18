#include "ScriptEngine.h"
#include "CoreCLRHost.h"
#include "Logger.h"
#include "app/Engine.h"
#include "app/Application.h"
#include "input/InputManager.h"
#include "platform/Platform.h"
#include "core/EntityManager.h"
#include "graphic/2d/LightManager2D.h"
#include "graphic/Renderer2D.h"
#include <cstring>

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

bool ScriptEngine::Initialize(CoreCLRHost& host) {
    if (m_initialized) return true;
    m_host = &host;
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
    m_api.srpShutdown = SRP_Shutdown;

    const std::string& scriptsDir = host.GetScriptsDir();
    std::string assemblyPath = scriptsDir + "/GameScripts.dll";
    m_bootstrapFn = (void (*)(void*))host.GetFunctionPointer(assemblyPath, "GameScripts.ScriptEntry, GameScripts", "Bootstrap");
    m_onFrameFn = (void (*)(float))host.GetFunctionPointer(assemblyPath, "GameScripts.ScriptEntry, GameScripts", "OnFrame");
    m_srpRenderFn = (void (*)(float))host.GetFunctionPointer(assemblyPath, "GameScripts.ScriptEntry, GameScripts", "OnRender");

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
