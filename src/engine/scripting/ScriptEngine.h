#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <mutex> // 引入锁以支持线程安全同步

#include "Export.h"

namespace Prisma {
namespace Scripting {

class CoreCLRHost;

// ============================================================================
// 实体数据池 - SoA (Struct of Arrays) 布局
// ============================================================================

constexpr uint32_t kMaxEntities = 32768; // 与 C# NativeAPI.MaxEntities 保持一致

struct TransformBufferSoA {
    float posX[kMaxEntities];
    float posY[kMaxEntities];
    float rotation[kMaxEntities];
    float scaleX[kMaxEntities];
    float scaleY[kMaxEntities];
};

struct RenderBufferSoA {
    uint32_t active[kMaxEntities];
    uint32_t generation[kMaxEntities];
    float    colorR[kMaxEntities];
    float    colorG[kMaxEntities];
    float    colorB[kMaxEntities];
    float    colorA[kMaxEntities];
    float    sizeW[kMaxEntities];
    float    sizeH[kMaxEntities];
};

struct PrismaAPI {
    void (*log)(const char* subsystem, const char* msg);
    uint32_t (*createEntity)();
    void (*destroyEntity)(uint32_t handle);
    
    // 缓冲区获取 (双缓冲支持)
    TransformBufferSoA* (*getTransformBufferA)();
    TransformBufferSoA* (*getTransformBufferB)();
    RenderBufferSoA*    (*getRenderBuffer)();
    
    // 输入与环境
    bool (*isKeyDown)(int key);
    float (*getMouseX)();
    float (*getMouseY)();
    float (*getDeltaTime)();
    
    // 相机与全局状态
    void (*setCameraPos)(float x, float y);
    void (*getCameraPos)(float* x, float* y);
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

    // 渲染器调用此接口获取当前可读的数据
    const TransformBufferSoA* GetCurrentTransformBuffer() const;
    const RenderBufferSoA*    GetRenderBuffer() const { return m_renderBuffer; }

private:
    uint32_t CreateEntity();
    void     DestroyEntity(uint32_t handle);

    static uint32_t S_CreateEntity();
    static void     S_DestroyEntity(uint32_t handle);
    static TransformBufferSoA* S_GetTransformBufferA();
    static TransformBufferSoA* S_GetTransformBufferB();
    static RenderBufferSoA*    S_GetRenderBuffer();
    static void     S_SetCameraPos(float x, float y);
    static void     S_GetCameraPos(float* x, float* y);

    CoreCLRHost* m_host = nullptr;
    bool  m_initialized = false;
    void (*m_bootstrapFn)(void* api) = nullptr;
    void (*m_onFrameFn)(float) = nullptr;

    PrismaAPI m_api = {};
    
    // 真正的 Ping-Pong 缓冲区
    TransformBufferSoA* m_transformBufferA = nullptr;
    TransformBufferSoA* m_transformBufferB = nullptr;
    RenderBufferSoA*    m_renderBuffer = nullptr; 
    
    // 追踪当前 C# 侧作为 "Read" 的缓冲区索引 (0=A, 1=B)
    // 注意：C# 侧每帧会交换，C++ 需要同步或感知
    mutable int m_currentReadIndex = 0;

    float m_cameraPosX = 0.0f, m_cameraPosY = 0.0f;
    float m_lastDeltaTime = 0.016f;
};

} // namespace Scripting
} // namespace Prisma
