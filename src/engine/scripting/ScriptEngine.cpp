#include "ScriptEngine.h"
#include "CoreCLRHost.h"
#include "Logger.h"
#include "app/Engine.h"
#include "input/InputManager.h"
#include <cstring>
#include <mutex>

namespace Prisma {
namespace Scripting {

static thread_local ScriptEngine* s_activeEngine = nullptr;

ScriptEngine::ScriptEngine() {
    m_transformBufferA = new TransformBufferSoA();
    m_transformBufferB = new TransformBufferSoA();
    m_renderBuffer = new RenderBufferSoA();
    
    std::memset(m_transformBufferA, 0, sizeof(TransformBufferSoA));
    std::memset(m_transformBufferB, 0, sizeof(TransformBufferSoA));
    std::memset(m_renderBuffer, 0, sizeof(RenderBufferSoA));
    
    for (uint32_t i = 0; i < kMaxEntities; ++i) {
        // 初始 generation 设置为 1
        m_renderBuffer->generation[i] = 1;
        // 初始缩放
        m_transformBufferA->scaleX[i] = m_transformBufferA->scaleY[i] = 1.0f;
        m_transformBufferB->scaleX[i] = m_transformBufferB->scaleY[i] = 1.0f;
    }
}

ScriptEngine::~ScriptEngine() {
    Shutdown();
    delete m_transformBufferA;
    delete m_transformBufferB;
    delete m_renderBuffer;
}

// ---- 句柄解构 ----
inline uint32_t EncodeHandle(uint32_t index, uint32_t gen) { return (index & 0xFFFF) | ((gen & 0xFFFF) << 16); }
inline uint32_t DecodeIndex(uint32_t handle) { return handle & 0xFFFF; }
inline uint32_t DecodeGen(uint32_t handle) { return handle >> 16; }

uint32_t ScriptEngine::CreateEntity() {
    // 简单的线性分配器 (实际应用中应配合 freeList)
    for (uint32_t i = 0; i < kMaxEntities; ++i) {
        if (m_renderBuffer->active[i] == 0) {
            m_renderBuffer->active[i] = 1;
            // 初始化数据
            m_transformBufferA->posX[i] = m_transformBufferB->posX[i] = 0;
            m_transformBufferA->posY[i] = m_transformBufferB->posY[i] = 0;
            m_renderBuffer->colorA[i] = 1.0f;
            return EncodeHandle(i, m_renderBuffer->generation[i]);
        }
    }
    return 0;
}

void ScriptEngine::DestroyEntity(uint32_t handle) {
    uint32_t index = DecodeIndex(handle);
    uint32_t handleGen = DecodeGen(handle);
    if (index < kMaxEntities && (m_renderBuffer->generation[index] & 0xFFFF) == handleGen) {
        m_renderBuffer->active[index] = 0;
        m_renderBuffer->generation[index]++; // 增加版本号，失效旧句柄
        if ((m_renderBuffer->generation[index] & 0xFFFF) == 0) m_renderBuffer->generation[index] = 1;
    }
}

const TransformBufferSoA* ScriptEngine::GetCurrentTransformBuffer() const {
    // 这里的逻辑需要根据 C# 侧的 SwapBuffers 保持同步
    // 假设 C# 侧每帧开始前会读取 Swap 后的 A/B 指针
    return (m_currentReadIndex == 0) ? m_transformBufferA : m_transformBufferB;
}

uint32_t ScriptEngine::S_CreateEntity() { return s_activeEngine ? s_activeEngine->CreateEntity() : 0; }
void ScriptEngine::S_DestroyEntity(uint32_t h) { if (s_activeEngine) s_activeEngine->DestroyEntity(h); }

TransformBufferSoA* ScriptEngine::S_GetTransformBufferA() { return s_activeEngine ? s_activeEngine->m_transformBufferA : nullptr; }
TransformBufferSoA* ScriptEngine::S_GetTransformBufferB() { return s_activeEngine ? s_activeEngine->m_transformBufferB : nullptr; }
RenderBufferSoA* ScriptEngine::S_GetRenderBuffer() { return s_activeEngine ? s_activeEngine->m_renderBuffer : nullptr; }

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

    const std::string& scriptsDir = host.GetScriptsDir();
    std::string assemblyPath = scriptsDir + "/GameScripts.dll";
    m_bootstrapFn = (void (*)(void*))host.GetFunctionPointer(assemblyPath, "GameScripts.ScriptEntry, GameScripts", "Bootstrap");
    m_onFrameFn = (void (*)(float))host.GetFunctionPointer(assemblyPath, "GameScripts.ScriptEntry, GameScripts", "OnFrame");
    
    if (!m_bootstrapFn || !m_onFrameFn) return false;
    
    m_bootstrapFn(&m_api);
    m_initialized = true;
    s_activeEngine = nullptr;
    return true;
}

void ScriptEngine::Update(float dt) {
    if (!m_initialized || !m_onFrameFn) return;
    s_activeEngine = this;
    
    // 调用 C# 侧更新
    // C# 侧会在 OnFrame 内部执行：Logic -> SwapBuffers
    m_onFrameFn(dt);
    
    // 同步 C++ 侧的读取索引
    // 这是一个简化的假设，实际中应由 C# 传回或通过原子变量共享
    m_currentReadIndex = 1 - m_currentReadIndex;
    
    s_activeEngine = nullptr;
}

void ScriptEngine::Shutdown() { m_initialized = false; }

} // namespace Scripting
} // namespace Prisma
