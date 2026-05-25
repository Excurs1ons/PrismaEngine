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
#include "audio/AudioAPI.h"
#include "audio/AudioTypes.h"
#include "audio/dsp/AudioNode.h"
#include "audio/dsp/nodes/OscillatorNode.h"
#include "audio/dsp/nodes/ADSRNode.h"
#include "audio/dsp/nodes/LFONode.h"
#include "audio/dsp/nodes/SVFNode.h"
#include "audio/dsp/nodes/DistortionNode.h"
#include "audio/dsp/nodes/DelayNode.h"
#include "audio/dsp/nodes/ChorusNode.h"
#include "audio/dsp/nodes/FlangerNode.h"
#include "audio/dsp/nodes/PhaserNode.h"
#include "audio/dsp/nodes/CompressorNode.h"
#include "audio/dsp/nodes/BiquadFilterNode.h"
#include "audio/dsp/nodes/GraphicEQNode.h"
#include "audio/dsp/nodes/ReverbNode.h"
#include "audio/dsp/nodes/ConvolutionReverbNode.h"
#include "audio/dsp/nodes/MasterBusNode.h"
#include "audio/dsp/nodes/GroupBusNode.h"
#include "audio/dsp/nodes/AuxBusNode.h"
#include "audio/dsp/nodes/SidechainNode.h"
#include "audio/dsp/nodes/LevelMeterNode.h"
#include "audio/dsp/nodes/SampleRateConverterNode.h"
#include "audio/dsp/SpectrumAnalyzer.h"
#include "audio/dsp/nodes/MixerManagerNode.h"
#include <unordered_map>
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

// ==================== Audio API ====================

static bool S_IsAudioInitialized() {
    auto& engine = Engine::Get();
    // Check via Engine audio device or AudioAPI
    return false; // Stub — will be wired when audio system is integrated
}

static float S_AudioGetMasterVolume() {
    return 1.0f;
}

static void S_AudioSetMasterVolume(float vol) {
}

static uint32_t S_AudioPlayClip(const char* path, void* desc) {
    if (!path) return 0;
    auto clip = Audio::AudioAPI::LoadClip(path);
    if (!clip) return 0;
    Audio::PlayDesc playDesc;
    auto* ad = (Prisma::Audio::PlayDesc*)desc; // Placeholder cast
    return 1; // Voice ID stub
}

static void S_AudioStop(uint32_t voiceId) {
}

static void S_AudioStopAll() {
}

static void S_AudioSetVolume(uint32_t voiceId, float vol) {
}

static bool S_AudioIsPlaying(uint32_t voiceId) {
    return false;
}

// ==================== AudioGraph API ====================

using namespace Prisma::Audio::DSP;

// 句柄跟踪：graph 句柄 → AudioGraph*, node 句柄 → 节点 + 所属 graph
struct NodeEntry {
    std::shared_ptr<AudioNode> node;
    uint64_t graphHandle;
};
thread_local uint64_t s_nextAudioGraphHandle = 1;
thread_local uint64_t s_nextAudioNodeHandle = 1;
thread_local std::unordered_map<uint64_t, std::unique_ptr<AudioGraph>> s_audioGraphs;
thread_local std::unordered_map<uint64_t, NodeEntry> s_audioNodes;
thread_local std::string s_audioNameBuf;

static uint64_t S_AudioCreateGraph(uint32_t sampleRate, uint32_t framesPerBlock) {
    auto graph = std::make_unique<AudioGraph>(sampleRate, framesPerBlock);
    uint64_t handle = s_nextAudioGraphHandle++;
    s_audioGraphs[handle] = std::move(graph);
    return handle;
}

static void S_AudioDestroyGraph(uint64_t graphHandle) {
    auto git = s_audioGraphs.find(graphHandle);
    if (git == s_audioGraphs.end()) return;

    // 移除该 graph 关联的所有 node 条目
    for (auto it = s_audioNodes.begin(); it != s_audioNodes.end(); ) {
        if (it->second.graphHandle == graphHandle)
            it = s_audioNodes.erase(it);
        else
            ++it;
    }

    s_audioGraphs.erase(git);
}

static uint64_t S_AudioGraphCreateNode(uint64_t graphHandle, const char* nodeType, const char* name) {
    auto git = s_audioGraphs.find(graphHandle);
    if (git == s_audioGraphs.end()) return 0;
    AudioGraph* graph = git->second.get();
    if (!graph) return 0;

    std::shared_ptr<AudioNode> node;
    std::string type(nodeType ? nodeType : "");

    if (type == "Oscillator")          node = graph->CreateNode(std::make_unique<OscillatorNode>());
    else if (type == "ADSR")           node = graph->CreateNode(std::make_unique<ADSRNode>());
    else if (type == "LFO")            node = graph->CreateNode(std::make_unique<LFONode>());
    else if (type == "SVF")            node = graph->CreateNode(std::make_unique<SVFNode>());
    else if (type == "Distortion")     node = graph->CreateNode(std::make_unique<DistortionNode>());
    else if (type == "Delay")          node = graph->CreateNode(std::make_unique<DelayNode>());
    else if (type == "Chorus")         node = graph->CreateNode(std::make_unique<ChorusNode>());
    else if (type == "Flanger")        node = graph->CreateNode(std::make_unique<FlangerNode>());
    else if (type == "Phaser")         node = graph->CreateNode(std::make_unique<PhaserNode>());
    else if (type == "Compressor")     node = graph->CreateNode(std::make_unique<CompressorNode>());
    else if (type == "BiquadFilter")   node = graph->CreateNode(std::make_unique<BiquadFilterNode>());
    else if (type == "GraphicEQ")      node = graph->CreateNode(std::make_unique<GraphicEQNode>());
    else if (type == "Reverb")         node = graph->CreateNode(std::make_unique<ReverbNode>());
    else if (type == "ConvolutionReverb") node = graph->CreateNode(std::make_unique<ConvolutionReverbNode>());
    else if (type == "MasterBus")      node = graph->CreateNode(std::make_unique<MasterBusNode>());
    else if (type == "GroupBus")       node = graph->CreateNode(std::make_unique<GroupBusNode>());
    else if (type == "AuxBus")         node = graph->CreateNode(std::make_unique<AuxBusNode>());
    else if (type == "Sidechain")      node = graph->CreateNode(std::make_unique<SidechainNode>());
    else if (type == "LevelMeter")     node = graph->CreateNode(std::make_unique<LevelMeterNode>());
    else if (type == "SRC")            node = graph->CreateNode(std::make_unique<SampleRateConverterNode>());
    else if (type == "MixerManager")   node = graph->CreateNode(std::make_unique<MixerManagerNode>());
    else {
        LOG_ERROR("ScriptEngine", "Unknown AudioNode type: {}", type);
        return 0;
    }

    if (!node) return 0;

    if (name && name[0] != '\0')
        node->SetName(name);

    uint64_t nodeHandle = s_nextAudioNodeHandle++;
    s_audioNodes[nodeHandle] = { node, graphHandle };
    return nodeHandle;
}

static void S_AudioGraphRemoveNode(uint64_t graphHandle, uint64_t nodeHandle) {
    auto git = s_audioGraphs.find(graphHandle);
    auto nit = s_audioNodes.find(nodeHandle);
    if (git == s_audioGraphs.end() || nit == s_audioNodes.end()) return;
    if (nit->second.graphHandle != graphHandle) return;

    git->second->RemoveNode(nit->second.node);
    s_audioNodes.erase(nit);
}

static bool S_AudioGraphConnect(uint64_t graphHandle, uint64_t srcHandle, const char* srcPin,
                                uint64_t dstHandle, const char* dstPin) {
    auto git = s_audioGraphs.find(graphHandle);
    if (git == s_audioGraphs.end()) return false;

    auto srcIt = s_audioNodes.find(srcHandle);
    auto dstIt = s_audioNodes.find(dstHandle);
    if (srcIt == s_audioNodes.end() || dstIt == s_audioNodes.end()) return false;
    if (srcIt->second.graphHandle != graphHandle || dstIt->second.graphHandle != graphHandle) return false;

    return git->second->Connect(
        srcIt->second.node,
        srcPin ? std::string(srcPin) : "",
        dstIt->second.node,
        dstPin ? std::string(dstPin) : ""
    );
}

static bool S_AudioGraphDisconnect(uint64_t graphHandle, uint64_t srcHandle, uint64_t dstHandle) {
    auto git = s_audioGraphs.find(graphHandle);
    if (git == s_audioGraphs.end()) return false;

    auto srcIt = s_audioNodes.find(srcHandle);
    auto dstIt = s_audioNodes.find(dstHandle);
    if (srcIt == s_audioNodes.end() || dstIt == s_audioNodes.end()) return false;

    return git->second->Disconnect(srcIt->second.node, dstIt->second.node);
}

static void S_AudioNodeSetParam(uint64_t nodeHandle, const char* name, float value) {
    auto it = s_audioNodes.find(nodeHandle);
    if (it == s_audioNodes.end()) return;
    if (!name) return;
    it->second.node->SetParameter(name, value);
}

static float S_AudioNodeGetParam(uint64_t nodeHandle, const char* name) {
    auto it = s_audioNodes.find(nodeHandle);
    if (it == s_audioNodes.end() || !name) return 0.0f;
    return it->second.node->GetParameter(name);
}

static const char* S_AudioNodeGetName(uint64_t nodeHandle) {
    auto it = s_audioNodes.find(nodeHandle);
    if (it == s_audioNodes.end()) return "";
    s_audioNameBuf = it->second.node->GetName();
    return s_audioNameBuf.c_str();
}

static void S_AudioNodeSetName(uint64_t nodeHandle, const char* name) {
    auto it = s_audioNodes.find(nodeHandle);
    if (it == s_audioNodes.end() || !name) return;
    it->second.node->SetName(name);
}

static void S_AudioNodeDestroy(uint64_t nodeHandle) {
    auto it = s_audioNodes.find(nodeHandle);
    if (it == s_audioNodes.end()) return;

    // 从所属 graph 中移除
    auto git = s_audioGraphs.find(it->second.graphHandle);
    if (git != s_audioGraphs.end()) {
        git->second->RemoveNode(it->second.node);
    }

    s_audioNodes.erase(it);
}

// ==================== Level Meter Readback ====================

static bool S_AudioGetLevelMeterData(uint64_t nodeHandle, uint32_t channel, float* peak, float* rms, float* peakDb, float* rmsDb) {
    auto it = s_audioNodes.find(nodeHandle);
    if (it == s_audioNodes.end() || !peak || !rms || !peakDb || !rmsDb) return false;

    auto* meter = dynamic_cast<LevelMeterNode*>(it->second.node.get());
    if (!meter) return false;

    if (channel >= meter->GetChannelCount()) return false;
    const auto& data = meter->GetChannelData(channel);
    *peak = data.peak;
    *rms = data.rms;
    *peakDb = data.peakDb;
    *rmsDb = data.rmsDb;
    return true;
}

// ==================== Spectrum Analyzer (standalone) ====================

thread_local std::unordered_map<uint64_t, std::unique_ptr<SpectrumAnalyzer>> s_spectrumAnalyzers;
thread_local uint64_t s_nextSpectrumId = 1;

static uint64_t S_AudioCreateSpectrumAnalyzer(uint32_t fftSize) {
    auto sa = std::make_unique<SpectrumAnalyzer>(fftSize);
    uint64_t id = s_nextSpectrumId++;
    s_spectrumAnalyzers[id] = std::move(sa);
    return id;
}

static void S_AudioDestroySpectrumAnalyzer(uint64_t handle) {
    s_spectrumAnalyzers.erase(handle);
}

static void S_AudioSpectrumProcessFloats(uint64_t handle, const float* input, uint32_t frames, uint32_t sampleRate) {
    auto it = s_spectrumAnalyzers.find(handle);
    if (it == s_spectrumAnalyzers.end() || !input) return;
    it->second->Process(input, frames, sampleRate);
}

static uint32_t S_AudioSpectrumGetBins(uint64_t handle, float* freqOut, float* magOut, float* phaseOut, uint32_t maxBins) {
    auto it = s_spectrumAnalyzers.find(handle);
    if (it == s_spectrumAnalyzers.end()) return 0;

    const auto& bins = it->second->GetBins();
    uint32_t count = std::min(static_cast<uint32_t>(bins.size()), maxBins);
    for (uint32_t i = 0; i < count; ++i) {
        if (freqOut) freqOut[i] = bins[i].frequency;
        if (magOut) magOut[i] = bins[i].magnitude;
        if (phaseOut) phaseOut[i] = bins[i].phase;
    }
    return count;
}

static float S_AudioSpectrumGetPeak(uint64_t handle) {
    auto it = s_spectrumAnalyzers.find(handle);
    if (it == s_spectrumAnalyzers.end()) return -100.0f;
    return it->second->GetPeakMagnitude();
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
    // Audio API
    m_api.isAudioInitialized = S_IsAudioInitialized;
    m_api.audioGetMasterVolume = S_AudioGetMasterVolume;
    m_api.audioSetMasterVolume = S_AudioSetMasterVolume;
    m_api.audioPlayClip = S_AudioPlayClip;
    m_api.audioStop = S_AudioStop;
    m_api.audioStopAll = S_AudioStopAll;
    m_api.audioSetVolume = S_AudioSetVolume;
    m_api.audioIsPlaying = S_AudioIsPlaying;

    // AudioGraph API
    m_api.audioCreateGraph = S_AudioCreateGraph;
    m_api.audioDestroyGraph = S_AudioDestroyGraph;
    m_api.audioGraphCreateNode = S_AudioGraphCreateNode;
    m_api.audioGraphRemoveNode = S_AudioGraphRemoveNode;
    m_api.audioGraphConnect = S_AudioGraphConnect;
    m_api.audioGraphDisconnect = S_AudioGraphDisconnect;
    m_api.audioNodeSetParam = S_AudioNodeSetParam;
    m_api.audioNodeGetParam = S_AudioNodeGetParam;
    m_api.audioNodeGetName = S_AudioNodeGetName;
    m_api.audioNodeSetName = S_AudioNodeSetName;
    m_api.audioNodeDestroy = S_AudioNodeDestroy;

    // Level Meter + Spectrum Readback
    m_api.audioGetLevelMeterData = S_AudioGetLevelMeterData;
    m_api.audioCreateSpectrumAnalyzer = S_AudioCreateSpectrumAnalyzer;
    m_api.audioDestroySpectrumAnalyzer = S_AudioDestroySpectrumAnalyzer;
    m_api.audioSpectrumProcessFloats = S_AudioSpectrumProcessFloats;
    m_api.audioSpectrumGetBins = S_AudioSpectrumGetBins;
    m_api.audioSpectrumGetPeak = S_AudioSpectrumGetPeak;

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

    // 填充 EditorAPI 函数指针表，供 C# 通过 GetEditorAPI() 获取
    FillEditorAPI(m_editorAPI);

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
