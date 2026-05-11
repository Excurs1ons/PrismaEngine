#include "ScriptEngine.h"
#include "CoreCLRHost.h"
#include "Logger.h"
#include "app/Engine.h"        // for InputManager
#include "input/InputManager.h"

namespace Prisma {
namespace Scripting {

// ============================================================================
// PrismaAPI 静态实现
// 所有函数直接操作 ScriptEngine 单例的实体池
// ============================================================================

static ScriptEngine* s_self = nullptr;

// ---- Logging ----
void ScriptEngine::S_Log(const char* subsystem, const char* msg) {
    // 不递归调用 Logger (C# 侧可能也用了 logger)
    if (subsystem && msg) {
        printf("[%s] %s\n", subsystem, msg);
    }
}

// ---- Entity Lifecycle ----
uint32_t ScriptEngine::S_CreateEntity() {
    if (!s_self) return 0;
    auto& entities = s_self->m_entities;

    // 找空闲槽位
    for (uint32_t i = 0; i < (uint32_t)entities.size(); ++i) {
        if (!entities[i].active) {
            entities[i].active = true;
            return i;
        }
    }

    // 扩展
    if (entities.size() < kMaxEntities) {
        uint32_t id = (uint32_t)entities.size();
        entities.emplace_back();
        entities.back().active = true;
        return id;
    }

    LOG_ERROR("ScriptEngine", "Entity pool exhausted (max={0})", kMaxEntities);
    return 0;
}

void ScriptEngine::S_DestroyEntity(uint32_t id) {
    if (s_self && id < s_self->m_entities.size()) {
        s_self->m_entities[id].active = false;
    }
}

// ---- Transform ----
void ScriptEngine::S_SetPosition(uint32_t id, float x, float y) {
    if (s_self && id < s_self->m_entities.size()) {
        auto& e = s_self->m_entities[id];
        e.posX = x; e.posY = y;
    }
}

void ScriptEngine::S_GetPosition(uint32_t id, float* x, float* y) {
    if (s_self && id < s_self->m_entities.size() && x && y) {
        *x = s_self->m_entities[id].posX;
        *y = s_self->m_entities[id].posY;
    }
}

void ScriptEngine::S_SetRotation(uint32_t id, float deg) {
    if (s_self && id < s_self->m_entities.size()) {
        s_self->m_entities[id].rotation = deg;
    }
}

float ScriptEngine::S_GetRotation(uint32_t id) {
    if (s_self && id < s_self->m_entities.size()) {
        return s_self->m_entities[id].rotation;
    }
    return 0.0f;
}

void ScriptEngine::S_SetScale(uint32_t id, float x, float y) {
    if (s_self && id < s_self->m_entities.size()) {
        s_self->m_entities[id].scaleX = x;
        s_self->m_entities[id].scaleY = y;
    }
}

void ScriptEngine::S_GetScale(uint32_t id, float* x, float* y) {
    if (s_self && id < s_self->m_entities.size() && x && y) {
        *x = s_self->m_entities[id].scaleX;
        *y = s_self->m_entities[id].scaleY;
    }
}

// ---- Visual ----
void ScriptEngine::S_SetColor(uint32_t id, float r, float g, float b, float a) {
    if (s_self && id < s_self->m_entities.size()) {
        auto& e = s_self->m_entities[id];
        e.colorR = r; e.colorG = g; e.colorB = b; e.colorA = a;
    }
}

void ScriptEngine::S_GetColor(uint32_t id, float* r, float* g, float* b, float* a) {
    if (s_self && id < s_self->m_entities.size() && r && g && b && a) {
        *r = s_self->m_entities[id].colorR;
        *g = s_self->m_entities[id].colorG;
        *b = s_self->m_entities[id].colorB;
        *a = s_self->m_entities[id].colorA;
    }
}

void ScriptEngine::S_SetSize(uint32_t id, float w, float h) {
    if (s_self && id < s_self->m_entities.size()) {
        s_self->m_entities[id].sizeW = w;
        s_self->m_entities[id].sizeH = h;
    }
}

void ScriptEngine::S_GetSize(uint32_t id, float* w, float* h) {
    if (s_self && id < s_self->m_entities.size() && w && h) {
        *w = s_self->m_entities[id].sizeW;
        *h = s_self->m_entities[id].sizeH;
    }
}

// ---- Input ----
bool ScriptEngine::S_IsKeyDown(int key) {
    auto* mgr = Engine::Get().GetInputManager();
    return mgr ? mgr->IsKeyPressed(static_cast<Prisma::Input::KeyCode>(key)) : false;
}

float ScriptEngine::S_GetMouseX() {
    auto* mgr = Engine::Get().GetInputManager();
    return mgr ? mgr->GetMousePosition().x : 0.0f;
}

float ScriptEngine::S_GetMouseY() {
    auto* mgr = Engine::Get().GetInputManager();
    return mgr ? mgr->GetMousePosition().y : 0.0f;
}

float ScriptEngine::S_GetDeltaTime() {
    return 0.016f;
}

// ---- Camera ----
void ScriptEngine::S_SetCameraPos(float x, float y) {
    if (s_self) { s_self->m_cameraPosX = x; s_self->m_cameraPosY = y; }
}

void ScriptEngine::S_GetCameraPos(float* x, float* y) {
    if (s_self && x && y) { *x = s_self->m_cameraPosX; *y = s_self->m_cameraPosY; }
}

// ============================================================================
// ScriptEngine 实现
// ============================================================================

bool ScriptEngine::Initialize(CoreCLRHost& host) {
    if (m_initialized) {
        LOG_WARNING("ScriptEngine", "Already initialized");
        return true;
    }

    m_host = &host;
    s_self = this;

    // 1. 填充 PrismaAPI
    m_api.log           = S_Log;
    m_api.createEntity  = S_CreateEntity;
    m_api.destroyEntity = S_DestroyEntity;
    m_api.setPosition   = S_SetPosition;
    m_api.getPosition   = S_GetPosition;
    m_api.setRotation   = S_SetRotation;
    m_api.getRotation   = S_GetRotation;
    m_api.setScale      = S_SetScale;
    m_api.getScale      = S_GetScale;
    m_api.setColor      = S_SetColor;
    m_api.getColor      = S_GetColor;
    m_api.setSize       = S_SetSize;
    m_api.getSize       = S_GetSize;
    m_api.isKeyDown     = S_IsKeyDown;
    m_api.getMouseX     = S_GetMouseX;
    m_api.getMouseY     = S_GetMouseY;
    m_api.getDeltaTime  = S_GetDeltaTime;
    m_api.setCameraPos  = S_SetCameraPos;
    m_api.getCameraPos  = S_GetCameraPos;

    // 2. 获取 C# 入口函数指针
    const std::string& scriptsDir = host.GetScriptsDir();
    std::string assemblyPath = scriptsDir + "/GameScripts.dll";

    auto bootstrapRaw = host.GetFunctionPointer(
        assemblyPath, "GameScripts.ScriptEntry, GameScripts", "Bootstrap");
    m_bootstrapFn = (void (*)(void*))bootstrapRaw;

    m_onFrameFn = (void (*)(float))host.GetFunctionPointer(
        assemblyPath, "GameScripts.ScriptEntry, GameScripts", "OnFrame");

    if (!m_bootstrapFn || !m_onFrameFn) {
        LOG_ERROR("ScriptEngine", "Failed to resolve C# entry points");
        Shutdown();
        return false;
    }

    // 3. 调用 Bootstrap（传 PrismaAPI 指针）
    m_bootstrapFn(&m_api);

    m_initialized = true;
    LOG_INFO("ScriptEngine", "Script engine initialized");
    return true;
}

void ScriptEngine::Update(float dt) {
    if (!m_initialized || !m_onFrameFn) return;

    // 刷新 API 中的 deltaTime（因为它是一个函数指针，只能返回缓存的静态值）
    // OnFrame 由 C# 侧独立计时
    m_onFrameFn(dt);
}

void ScriptEngine::GetCameraPosition(float& x, float& y) const {
    x = m_cameraPosX;
    y = m_cameraPosY;
}

uint32_t ScriptEngine::GetEntityCount() const {
    return (uint32_t)m_entities.size();
}

EntityData* ScriptEngine::GetEntity(uint32_t id) {
    return (id < m_entities.size()) ? &m_entities[id] : nullptr;
}

const EntityData* ScriptEngine::GetEntity(uint32_t id) const {
    return (id < m_entities.size()) ? &m_entities[id] : nullptr;
}

void ScriptEngine::Shutdown() {
    m_initialized = false;
    m_bootstrapFn = nullptr;
    m_onFrameFn = nullptr;
    m_host = nullptr;
    m_entities.clear();
    m_nextEntityId = 1;

    if (s_self == this) s_self = nullptr;

    LOG_DEBUG("ScriptEngine", "Shut down");
}

// GetEntity inlined in header

} // namespace Scripting
} // namespace Prisma
