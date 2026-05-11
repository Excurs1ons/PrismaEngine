#pragma once

#include <cstdint>
#include <string>
#include <deque>
#include <mutex>

#include "Export.h"

namespace Prisma {
namespace Scripting {

class CoreCLRHost;

// ============================================================================
// 实体数据池 - 虚拟内存 SoA (Virtual Memory Reservation)
// ============================================================================
// 启动时 VirtualAlloc(MEM_RESERVE) 预留 1M 实体的虚拟地址空间，
// CreateEntity 时按需 VirtualAlloc(MEM_COMMIT) 提交物理内存。
// 指针从创建到销毁永不改变，C# 侧不会拿到野指针。

static constexpr uint32_t kMaxVirtualEntities = 1024 * 1024; // 1M VA 上限
static constexpr uint32_t kCommitStep = 16384;                // 每次提交 16K 槽位

// Transform 双缓冲 SoA — 字段为指针，指向 VA block 内的固定偏移
struct TransformBufferSoA {
    float* posX;
    float* posY;
    float* rotation;
    float* scaleX;
    float* scaleY;
};

// Render SoA（无缓冲）
struct RenderBufferSoA {
    uint32_t* active;
    uint32_t* generation;
    float*    colorR;
    float*    colorG;
    float*    colorB;
    float*    colorA;
    float*    sizeW;
    float*    sizeH;
};

struct PrismaAPI {
    void (*log)(const char* subsystem, const char* msg);
    uint32_t (*createEntity)();
    void (*destroyEntity)(uint32_t handle);
    
    TransformBufferSoA* (*getTransformBufferA)();
    TransformBufferSoA* (*getTransformBufferB)();
    RenderBufferSoA*    (*getRenderBuffer)();
    
    bool (*isKeyDown)(int key);
    float (*getMouseX)();
    float (*getMouseY)();
    float (*getDeltaTime)();
    
    void (*setCameraPos)(float x, float y);
    void (*getCameraPos)(float* x, float* y);
    
    // 已提交的实体槽位数（C# 用于边界检查）
    uint32_t (*getEntityCapacity)();
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

    const TransformBufferSoA* GetCurrentTransformBuffer() const;
    const RenderBufferSoA*    GetRenderBuffer() const { return &m_layoutR; }
    uint32_t GetEntityCapacity() const { return m_aliveCount; }

private:
    uint32_t CreateEntity();
    void     DestroyEntity(uint32_t handle);

    // ---- VA 内存管理 ----
    void* reserveBlock(size_t bytes);
    void  commitRange(uint32_t fromEntity, uint32_t toEntity);
    void  initLayoutPointers();

    static uint32_t S_CreateEntity();
    static void     S_DestroyEntity(uint32_t handle);
    static TransformBufferSoA* S_GetTransformBufferA();
    static TransformBufferSoA* S_GetTransformBufferB();
    static RenderBufferSoA*    S_GetRenderBuffer();
    static void     S_SetCameraPos(float x, float y);
    static void     S_GetCameraPos(float* x, float* y);

    // ---- VA 预留块（3 个：Transform A / Transform B / Render） ----
    void* m_blockABase = nullptr;  // 5 float arrays
    void* m_blockBBase = nullptr;  // 5 float arrays
    void* m_blockRBase = nullptr;  // 8 arrays (2 uint32 + 6 float)
    uint32_t m_aliveCount = 0;     // 活跃实体数（高水位线，Create 递增）
    uint32_t m_committed = 0;      // 已提交的物理槽位数（按 kCommitStep 增长）

    // ---- 固定偏移的 SoA 布局（指针设一次永远不变） ----
    TransformBufferSoA m_layoutA{};
    TransformBufferSoA m_layoutB{};
    RenderBufferSoA    m_layoutR{};

    CoreCLRHost* m_host = nullptr;
    bool  m_initialized = false;
    void (*m_bootstrapFn)(void* api) = nullptr;
    void (*m_onFrameFn)(float) = nullptr;

    PrismaAPI m_api = {};
    mutable int m_currentReadIndex = 0;

    float m_cameraPosX = 0.0f, m_cameraPosY = 0.0f;
    float m_lastDeltaTime = 0.016f;
    
    static uint32_t S_GetEntityCapacity();
};

} // namespace Scripting
} // namespace Prisma
