#include "ScriptEngine.h"
#include "CoreCLRHost.h"
#include "Logger.h"
#include "app/Engine.h"
#include "input/InputManager.h"
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Prisma {
namespace Scripting {

static thread_local ScriptEngine* s_activeEngine = nullptr;

// 每个子数组（posX/posY/...）的跨步 = 1M float * 4 字节 = 4MB
static constexpr size_t kFieldStride = kMaxVirtualEntities * sizeof(float);

// ============================================================================
// 平台内存抽象
// ============================================================================

static void* osReserve(size_t bytes) {
#ifdef _WIN32
    return VirtualAlloc(nullptr, bytes, MEM_RESERVE, PAGE_READWRITE);
#else
    return mmap(nullptr, bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
#endif
}

static void osCommit(void* addr, size_t bytes) {
#ifdef _WIN32
    VirtualAlloc(addr, bytes, MEM_COMMIT, PAGE_READWRITE);
#else
    // 用 MAP_FIXED 替换 PROT_NONE 映射为可读写
    mmap(addr, bytes, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
}

static void osRelease(void* addr, size_t bytes) {
#ifdef _WIN32
    VirtualFree(addr, 0, MEM_RELEASE);
#else
    munmap(addr, bytes);
#endif
}

// ============================================================================
// ScriptEngine
// ============================================================================

ScriptEngine::ScriptEngine() {
    // 预留 3 个 VA 块：Transform A / Transform B / Render
    // Transform: 5 float 数组 × 4MB = 20MB/块
    // Render: 8 数组 (2 uint32 + 6 float) × 4MB = 32MB
    size_t blockSizeA = 5 * kFieldStride;  // 20MB
    size_t blockSizeR = 8 * kFieldStride;  // 32MB

    m_blockABase = osReserve(blockSizeA);
    m_blockBBase = osReserve(blockSizeA);
    m_blockRBase = osReserve(blockSizeR);

    // 初始化布局指针（设置一次永不改变）
    initLayoutPointers();
}

ScriptEngine::~ScriptEngine() {
    Shutdown();
    if (m_blockABase) osRelease(m_blockABase, 5 * kFieldStride);
    if (m_blockBBase) osRelease(m_blockBBase, 5 * kFieldStride);
    if (m_blockRBase) osRelease(m_blockRBase, 8 * kFieldStride);
}

void ScriptEngine::initLayoutPointers() {
    auto initTransform = [](TransformBufferSoA& layout, void* base) {
        auto* bytes = (uint8_t*)base;
        layout.posX     = (float*)(bytes + 0 * kFieldStride);
        layout.posY     = (float*)(bytes + 1 * kFieldStride);
        layout.rotation = (float*)(bytes + 2 * kFieldStride);
        layout.scaleX   = (float*)(bytes + 3 * kFieldStride);
        layout.scaleY   = (float*)(bytes + 4 * kFieldStride);
    };

    auto initRender = [](RenderBufferSoA& layout, void* base) {
        auto* bytes = (uint8_t*)base;
        layout.active     = (uint32_t*)(bytes + 0 * kFieldStride);
        layout.generation = (uint32_t*)(bytes + 1 * kFieldStride);
        layout.colorR     = (float*)(bytes + 2 * kFieldStride);
        layout.colorG     = (float*)(bytes + 3 * kFieldStride);
        layout.colorB     = (float*)(bytes + 4 * kFieldStride);
        layout.colorA     = (float*)(bytes + 5 * kFieldStride);
        layout.sizeW      = (float*)(bytes + 6 * kFieldStride);
        layout.sizeH      = (float*)(bytes + 7 * kFieldStride);
    };

    initTransform(m_layoutA, m_blockABase);
    initTransform(m_layoutB, m_blockBBase);
    initRender(m_layoutR, m_blockRBase);
}

// 确保 entityIndex 对应的物理页已提交
void ScriptEngine::commitRange(uint32_t fromEntity, uint32_t toEntity) {
    if (toEntity <= fromEntity) return;

    // 对齐到页边界
    size_t start = (fromEntity * sizeof(float) / 4096) * 4096;
    size_t end   = ((toEntity   * sizeof(float) + 4095) / 4096) * 4096;
    size_t size  = end - start;
    if (size == 0) return;

    auto commitField = [&](void* base, uint32_t fieldCount) {
        for (uint32_t f = 0; f < fieldCount; f++) {
            void* addr = (uint8_t*)base + f * kFieldStride + start;
            osCommit(addr, size);
        }
    };

    commitField(m_blockABase, 5);
    commitField(m_blockBBase, 5);
    commitField(m_blockRBase, 8);
}

// ---- 句柄编码 ----
inline uint32_t EncodeHandle(uint32_t index, uint32_t gen) { return (index & 0xFFFF) | ((gen & 0xFFFF) << 16); }
inline uint32_t DecodeIndex(uint32_t handle) { return handle & 0xFFFF; }
inline uint32_t DecodeGen(uint32_t handle) { return handle >> 16; }

uint32_t ScriptEngine::CreateEntity() {
    // 找空闲槽
    for (uint32_t i = 0; i < m_aliveCount; ++i) {
        if (m_layoutR.active[i] == 0) {
            m_layoutR.active[i] = 1;
            m_layoutA.posX[i] = m_layoutB.posX[i] = 0;
            m_layoutA.posY[i] = m_layoutB.posY[i] = 0;
            m_layoutR.colorA[i] = 1.0f;
            return EncodeHandle(i, m_layoutR.generation[i]);
        }
    }

    // 无空闲槽，追加新槽位
    uint32_t index = m_aliveCount++;
    
    // 确保物理内存已提交
    if (index >= m_committed) {
        uint32_t newCommit = ((index + 1 + kCommitStep - 1) / kCommitStep) * kCommitStep;
        if (newCommit > kMaxVirtualEntities) newCommit = kMaxVirtualEntities;
        commitRange(m_committed, newCommit);
        m_committed = newCommit;
    }

    m_layoutR.active[index]     = 1;
    m_layoutR.generation[index] = 1;
    // scale 默认为 1
    m_layoutA.scaleX[index] = m_layoutA.scaleY[index] = 1.0f;
    m_layoutB.scaleX[index] = m_layoutB.scaleY[index] = 1.0f;
    m_layoutR.colorA[index] = 1.0f;

    return EncodeHandle(index, m_layoutR.generation[index]);
}

void ScriptEngine::DestroyEntity(uint32_t handle) {
    uint32_t index = DecodeIndex(handle);
    uint32_t handleGen = DecodeGen(handle);
    if (index < m_aliveCount && (m_layoutR.generation[index] & 0xFFFF) == handleGen) {
        m_layoutR.active[index] = 0;
        m_layoutR.generation[index]++;
        if ((m_layoutR.generation[index] & 0xFFFF) == 0) m_layoutR.generation[index] = 1;
    }
}

const TransformBufferSoA* ScriptEngine::GetCurrentTransformBuffer() const {
    return (m_currentReadIndex == 0) ? &m_layoutA : &m_layoutB;
}

// ---- 静态回调（通过 s_activeEngine 转发） ----

uint32_t ScriptEngine::S_CreateEntity() { return s_activeEngine ? s_activeEngine->CreateEntity() : 0; }
void ScriptEngine::S_DestroyEntity(uint32_t h) { if (s_activeEngine) s_activeEngine->DestroyEntity(h); }

TransformBufferSoA* ScriptEngine::S_GetTransformBufferA() { return s_activeEngine ? &s_activeEngine->m_layoutA : nullptr; }
TransformBufferSoA* ScriptEngine::S_GetTransformBufferB() { return s_activeEngine ? &s_activeEngine->m_layoutB : nullptr; }
RenderBufferSoA*    ScriptEngine::S_GetRenderBuffer() { return s_activeEngine ? &s_activeEngine->m_layoutR : nullptr; }

uint32_t ScriptEngine::S_GetEntityCapacity() { return s_activeEngine ? s_activeEngine->m_aliveCount : 0; }

void ScriptEngine::S_SetCameraPos(float x, float y) {
    if (s_activeEngine) {
        s_activeEngine->m_cameraPosX = x;
        s_activeEngine->m_cameraPosY = y;
    }
}
void ScriptEngine::S_GetCameraPos(float* x, float* y) { if (x && y) { *x = 0; *y = 0; } }

static bool S_IsKeyDown(int k) { auto* m = Engine::Get().GetInputManager(); return m ? m->IsKeyPressed((Prisma::Input::KeyCode)k) : false; }
static float S_GetMouseX() { auto* m = Engine::Get().GetInputManager(); return m ? m->GetMousePosition().x : 0; }
static float S_GetMouseY() { auto* m = Engine::Get().GetInputManager(); return m ? m->GetMousePosition().y : 0; }
static float S_GetDeltaTime() { return 0.016f; }
static void S_Log(const char* s, const char* m) { if (s && m) printf("[%s] %s\n", s, m); }

bool ScriptEngine::Initialize(CoreCLRHost& host) {
    if (m_initialized) return true;
    m_host = &host;
    s_activeEngine = this;

    m_api.log = S_Log;
    m_api.createEntity = S_CreateEntity;
    m_api.destroyEntity = S_DestroyEntity;
    m_api.getTransformBufferA = S_GetTransformBufferA;
    m_api.getTransformBufferB = S_GetTransformBufferB;
    m_api.getRenderBuffer = S_GetRenderBuffer;
    m_api.isKeyDown = S_IsKeyDown;
    m_api.getMouseX = S_GetMouseX;
    m_api.getMouseY = S_GetMouseY;
    m_api.getDeltaTime = S_GetDeltaTime;
    m_api.setCameraPos = S_SetCameraPos;
    m_api.getCameraPos = S_GetCameraPos;
    m_api.getEntityCapacity = S_GetEntityCapacity;

    const std::string& scriptsDir = host.GetScriptsDir();
    std::string assemblyPath = scriptsDir + "/GameScripts.dll";
    m_bootstrapFn = (void (*)(void*))host.GetFunctionPointer(assemblyPath, "GameScripts.ScriptEntry, GameScripts", "Bootstrap");
    m_onFrameFn = (void (*)(float))host.GetFunctionPointer(assemblyPath, "GameScripts.ScriptEntry, GameScripts", "OnFrame");

    if (!m_bootstrapFn || !m_onFrameFn) return false;

    m_bootstrapFn(&m_api);

    // --- Active Range Copy: 只拷贝活跃实体区间 ---
    // Bootstrap 写入了 Write 缓冲区（默认为 B），将活跃区数据镜像到 Read 缓冲区（A）
    size_t copyBytes = m_aliveCount * sizeof(float);
    if (copyBytes > 0) {
        std::memcpy(m_layoutA.posX,     m_layoutB.posX,     copyBytes);
        std::memcpy(m_layoutA.posY,     m_layoutB.posY,     copyBytes);
        std::memcpy(m_layoutA.rotation, m_layoutB.rotation, copyBytes);
        std::memcpy(m_layoutA.scaleX,   m_layoutB.scaleX,   copyBytes);
        std::memcpy(m_layoutA.scaleY,   m_layoutB.scaleY,   copyBytes);
    }

    m_initialized = true;
    s_activeEngine = nullptr;
    return true;
}

void ScriptEngine::Update(float dt) {
    if (!m_initialized || !m_onFrameFn) return;
    s_activeEngine = this;
    m_onFrameFn(dt);
    m_currentReadIndex = 1 - m_currentReadIndex;
    s_activeEngine = nullptr;
}

void ScriptEngine::Shutdown() { m_initialized = false; }

} // namespace Scripting
} // namespace Prisma
