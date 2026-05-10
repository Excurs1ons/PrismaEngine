#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Export.h"

namespace Prisma {
namespace Scripting {

class CoreCLRHost;

// ============================================================================
// 实体数据池
// ============================================================================

constexpr uint32_t kMaxEntities = 2048;

struct EntityData {
    bool    active   = false;
    float   posX = 0, posY = 0;
    float   rotation = 0.0f;
    float   scaleX = 1.0f, scaleY = 1.0f;
    float   colorR = 1.0f, colorG = 1.0f, colorB = 1.0f, colorA = 1.0f;
    float   sizeW = 50.0f, sizeH = 50.0f;
};

// ============================================================================
// PrismaAPI — C++ → C# 服务层函数指针表
// ============================================================================

struct PrismaAPI {
    void (*log)(const char* subsystem, const char* msg);

    uint32_t (*createEntity)();
    void (*destroyEntity)(uint32_t id);

    void (*setPosition)(uint32_t id, float x, float y);
    void (*getPosition)(uint32_t id, float* x, float* y);
    void (*setRotation)(uint32_t id, float deg);
    float (*getRotation)(uint32_t id);
    void (*setScale)(uint32_t id, float x, float y);
    void (*getScale)(uint32_t id, float* x, float* y);

    void (*setColor)(uint32_t id, float r, float g, float b, float a);
    void (*getColor)(uint32_t id, float* r, float* g, float* b, float* a);
    void (*setSize)(uint32_t id, float w, float h);
    void (*getSize)(uint32_t id, float* w, float* h);

    bool (*isKeyDown)(int key);
    float (*getMouseX)();
    float (*getMouseY)();

    float (*getDeltaTime)();

    void (*setCameraPos)(float x, float y);
    void (*getCameraPos)(float* x, float* y);
};

// ============================================================================
// ScriptEngine — 脚本引擎
// ============================================================================

class ENGINE_API ScriptEngine {
public:
    ScriptEngine() = default;
    ~ScriptEngine() { Shutdown(); }

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    /** @brief 初始化：获取 C# 入口函数指针，填充 PrismaAPI */
    bool Initialize(CoreCLRHost& host);

    /** @brief 关闭脚本引擎 */
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

    /** @brief 每帧调用 C# OnFrame(dt) */
    void Update(float dt);

    /** @brief 获取 PrismaAPI 函数指针表（给 C# 传递用） */
    const PrismaAPI& GetAPI() const { return m_api; }

    /** @brief 获取 C# 控制的相机位置（用于同步到渲染相机） */
    void GetCameraPosition(float& x, float& y) const;

    // ---- 实体数据访问 ----
    uint32_t GetEntityCount() const;
    EntityData* GetEntity(uint32_t id);
    const EntityData* GetEntity(uint32_t id) const;

private:
    // ---- PrismaAPI 静态实现 ----
    static uint32_t S_CreateEntity();
    static void     S_DestroyEntity(uint32_t id);
    static void     S_SetPosition(uint32_t id, float x, float y);
    static void     S_GetPosition(uint32_t id, float* x, float* y);
    static void     S_SetRotation(uint32_t id, float deg);
    static float    S_GetRotation(uint32_t id);
    static void     S_SetScale(uint32_t id, float x, float y);
    static void     S_GetScale(uint32_t id, float* x, float* y);
    static void     S_SetColor(uint32_t id, float r, float g, float b, float a);
    static void     S_GetColor(uint32_t id, float* r, float* g, float* b, float* a);
    static void     S_SetSize(uint32_t id, float w, float h);
    static void     S_GetSize(uint32_t id, float* w, float* h);
    static bool     S_IsKeyDown(int key);
    static float    S_GetMouseX();
    static float    S_GetMouseY();
    static float    S_GetDeltaTime();
    static void     S_Log(const char* subsystem, const char* msg);
    static void     S_SetCameraPos(float x, float y);
    static void     S_GetCameraPos(float* x, float* y);

    CoreCLRHost* m_host = nullptr;
    bool  m_initialized = false;
    void (*m_bootstrapFn)(void* api) = nullptr;
    void (*m_onFrameFn)(float) = nullptr;

    PrismaAPI m_api = {};

    std::vector<EntityData> m_entities;
    uint32_t m_nextEntityId = 1; // 0 = invalid

    float m_cameraPosX = 0.0f, m_cameraPosY = 0.0f;
};

} // namespace Scripting
} // namespace Prisma
