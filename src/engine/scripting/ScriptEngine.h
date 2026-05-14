#pragma once

#include <cstdint>
#include <string>
#include <deque>
#include <mutex>

#include "Export.h"
#include "core/Node.h"

namespace Prisma {
namespace Scripting {

class CoreCLRHost;

struct PrismaAPI {
    void (*log)(const char* subsystem, const char* msg);
    uint32_t (*createEntity)();
    void (*destroyEntity)(uint32_t handle);
    
    TransformDataLayout* (*getTransformA)();
    TransformDataLayout* (*getTransformB)();
    RenderDataLayout*    (*getRenderData)();
    
    bool (*isKeyDown)(int key);
    float (*getMouseX)();
    float (*getMouseY)();
    float (*getDeltaTime)();
    
    void (*setCameraPos)(float x, float y);
    void (*getCameraPos)(float* x, float* y);
    
    // Gizmos
    void (*drawGizmoLine)(float x1, float y1, float x2, float y2, float r, float g, float b, float a);
    void (*drawGizmoRect)(float x, float y, float w, float h, float r, float g, float b, float a);
    void (*drawGizmoString)(const char* text, float x, float y, float scale, float r, float g, float b, float a);

    // 已提交的实体槽位数（C# 用于边界检查）
    uint32_t (*getEntityCapacity)();

    // 2D 光照 API
    uint32_t (*createLight)(int type);
    void     (*destroyLight)(uint32_t handle);
    void     (*setLightPos)(uint32_t handle, float x, float y);
    void     (*setLightColor)(uint32_t handle, float r, float g, float b);
    void     (*setLightIntensity)(uint32_t handle, float intensity);
    void     (*setLightRadius)(uint32_t handle, float radius);
    void     (*setLightFalloff)(uint32_t handle, float falloff);
    void     (*setLightOrder)(uint32_t handle, int order);
    void     (*setLightBlendMode)(uint32_t handle, int mode);

    // 2D 环境光（调节场景基础照明级别）
    void (*setAmbientLight)(float r, float g, float b);

    // [诊断] 结构体大小，用于 C++/C# 版本校验
    // C++ 侧在 Initialize 中设置为 sizeof(PrismaAPI)
    // C# 侧在 Init 中校验，不匹配时抛出明确异常
    uint32_t structSize = 0;
};

class ENGINE_API ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    bool Initialize(CoreCLRHost& host);
    void Shutdown();
    void Update(float dt);

    bool IsInitialized() const { return m_initialized; }
    const PrismaAPI& GetAPI() const { return m_api; }
    void GetCameraPos(float* x, float* y) const { *x = m_cameraPosX; *y = m_cameraPosY; }

private:
    static uint32_t S_CreateEntity();
    static void     S_DestroyEntity(uint32_t handle);
    static TransformDataLayout* S_GetTransformA();
    static TransformDataLayout* S_GetTransformB();
    static RenderDataLayout*    S_GetRenderData();
    static void     S_SetCameraPos(float x, float y);
    static void     S_GetCameraPos(float* x, float* y);
    static uint32_t S_GetEntityCapacity();

    CoreCLRHost* m_host = nullptr;
    bool  m_initialized = false;
    void (*m_bootstrapFn)(void* api) = nullptr;
    void (*m_onFrameFn)(float) = nullptr;

    PrismaAPI m_api = {};

    float m_cameraPosX = 0.0f, m_cameraPosY = 0.0f;
    float m_lastDeltaTime = 0.016f;
};

} // namespace Scripting
} // namespace Prisma
