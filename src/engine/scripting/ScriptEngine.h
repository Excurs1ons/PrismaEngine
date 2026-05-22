#pragma once

#include <cstdint>
#include <string>
#include <deque>
#include <mutex>

#include "Export.h"
#include "core/Node.h"
#include "SRPGraphicsAPI.h"

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

    // ===== SRP Graphics API (C# 可编程渲染管线) =====

    // Shader
    uint32_t (*srpCreateShader)(const char* source, uint32_t sourceLen, uint32_t stage);
    void     (*srpDestroyShader)(uint32_t handle);

    // Pipeline
    uint32_t (*srpCreatePipeline)(const SRPPipelineDesc* desc);
    void     (*srpDestroyPipeline)(uint32_t handle);

    // Render Target
    uint32_t (*srpCreateRenderTarget)(int w, int h, uint32_t format, int samples);
    uint32_t (*srpCreateDepthTarget)(int w, int h, uint32_t format);
    void     (*srpDestroyRenderTarget)(uint32_t handle);
    void     (*srpDestroyDepthTarget)(uint32_t handle);

    // Buffers
    uint32_t (*srpCreateVertexBuffer)(const void* data, uint32_t size, uint32_t stride);
    uint32_t (*srpCreateIndexBuffer)(const void* data, uint32_t size, int is32Bit);
    void     (*srpDestroyBuffer)(uint32_t handle);

    // Texture
    uint32_t (*srpCreateTexture2D)(int w, int h, uint32_t format, const void* pixels, uint32_t pixelSize);
    void     (*srpDestroyTexture)(uint32_t handle);

    // Sampler
    uint32_t (*srpCreateSampler)(const SRPSamplerDesc* desc);
    void     (*srpDestroySampler)(uint32_t handle);

    // Texture binding
    void     (*srpCmdBindTexture)(uint32_t slot, uint32_t tex, uint32_t sampler);
    void     (*srpCmdBlitRenderTarget)(uint32_t dstTex);

    // Frame
    void (*srpBeginFrame)();
    void (*srpEndFrame)();

    // Command buffer (called between BeginFrame/EndFrame)
    void (*srpCmdBeginRenderPass)(uint32_t rtCount, const uint32_t* rtHandles, uint32_t depthHandle, const float* clearColors, float depthClear, int viewW, int viewH);
    void (*srpCmdEndRenderPass)();
    void (*srpCmdBindPipeline)(uint32_t handle);
    void (*srpCmdBindVertexBuffer)(uint32_t handle, uint32_t slot, uint32_t offset);
    void (*srpCmdBindIndexBuffer)(uint32_t handle, uint32_t offset, int is32Bit);
    void (*srpCmdSetViewport)(int x, int y, int w, int h);
    void (*srpCmdSetScissor)(int x, int y, int w, int h);
    void (*srpCmdPushConstants)(uint32_t offset, uint32_t size, const void* data);
    void (*srpCmdDraw)(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
    void (*srpCmdDrawIndexed)(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset);
    void (*srpCmdDrawFullScreenQuad)();

    // Compute pipeline
    uint32_t (*srpCreateComputePipeline)(uint32_t shader, uint32_t pushConstSize);
    void     (*srpDestroyComputePipeline)(uint32_t handle);
    void     (*srpCmdBindComputePipeline)(uint32_t handle);
    void     (*srpCmdDispatch)(uint32_t x, uint32_t y, uint32_t z);
    void     (*srpCmdBindComputeTexture)(uint32_t slot, uint32_t tex, uint32_t sampler);
    void     (*srpCmdBindStorageImage)(uint32_t slot, uint32_t tex);
    void     (*srpCmdBindStorageBuffer)(uint32_t slot, uint32_t buf);

    void (*srpShutdown)();

    // ===== 3D Camera API (for Template3D/PathTracing) =====
    void (*setCamera3DPos)(float x, float y, float z);
    void (*getCamera3DPos)(float* x, float* y, float* z);
    void (*setCameraRotation)(float pitch, float yaw);
    void (*moveCameraLocal)(float forward, float right, float up);

    // ===== Enhanced Input API =====
    float (*getMouseDeltaX)();
    float (*getMouseDeltaY)();
    float (*getMouseScrollX)();
    float (*getMouseScrollY)();
    void (*setMouseCapture)(bool capture);
    bool (*isKeyJustPressed)(int key);

    // ===== Path Tracing Pipeline Control =====
    void (*ptSetMaxSamples)(uint32_t samples);
    uint32_t (*ptGetFrameCount)();
    void (*ptResetAccumulation)();
    void (*ptSetNEE)(bool enabled);
    bool (*ptGetNEE)();
    void (*ptCycleMode)();
    void (*ptGetModeName)(char* buffer, uint32_t bufferSize);
    bool (*ptIsConverged)();
    uint32_t (*ptGetMaxSamples)();

    // [诊断] 结构体大小，用于 C++/C# 版本校验
    // C++ 侧在 Initialize 中设置为 sizeof(PrismaAPI)
    // C# 侧在 Init 中校验，不匹配时抛出明确异常
    uint32_t structSize = 0;
};

class ENGINE_API ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    bool Initialize(CoreCLRHost& host, const std::string& gameDir = "");
    void Shutdown();
    void Update(float dt);
    void Render(float dt);

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
    std::string m_gameDir;                   // 游戏 DLL 所在目录（与 Host 运行时目录分离）
    bool  m_initialized = false;
    void (*m_bootstrapFn)(void* api) = nullptr;
    void (*m_onFrameFn)(float) = nullptr;
    void (*m_srpRenderFn)(float) = nullptr;

    PrismaAPI m_api = {};

    float m_cameraPosX = 0.0f, m_cameraPosY = 0.0f;
    float m_lastDeltaTime = 0.016f;
};

} // namespace Scripting
} // namespace Prisma
