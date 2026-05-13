#include "EditorService.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "scene/GameObject.h"
#include "core/AssetDatabase.h"
#include "transform/Transform.h"
#include "transform/Camera.h"
#include "EditorSharedMemory.h"
#include "logger/Logger.h"
#include <glaze/glaze.hpp>
#include <cstring>
#include <cmath>

namespace Prisma {

static std::vector<uint8_t> s_FallbackBuffer;
static std::unique_ptr<EditorSharedMemory> s_SceneSHM;
static std::unique_ptr<EditorSharedMemory> s_GameSHM;

struct EditorCamera {
    float x = 0, y = 0, zoom = 1.0f;
} g_EditorCam;

nlohmann::json EditorService::Dispatch(const std::string& action, const nlohmann::json& params) {
    auto& engine = Engine::Get();
    auto* s = engine.GetSceneManager()->GetCurrentScene();

    if (action == "console/get") {
        auto logs = Logger::Get().GetRecentLogs(30);
        nlohmann::json res = nlohmann::json::array();
        for(auto& l : logs) res.push_back({{"level", (int)l.level}, {"msg", l.message}, {"tag", l.category}});
        return {{"logs", res}};
    }
    if (action == "assets/list") {
        nlohmann::json res = nlohmann::json::array();
        for(const auto& [p, m] : AssetDatabase::Get().GetAllMetadata()) res.push_back({{"path", p}, {"type", m.type}});
        return {{"assets", res}};
    }
    if (action == "engine/status") {
        return {{"fps", engine.GetFrameStats().FPS}, {"gpu", engine.GetGPUName()}, {"scene", s?s->GetName():"None"}, {"objects", s?(int)s->GetGameObjects().size():0}};
    }
    if (action == "viewport/input") {
        std::string type = params.value("type", "");
        if (type == "mouseMove") { g_EditorCam.x += params.value("dx", 0.0f) * 0.05f; g_EditorCam.y -= params.value("dy", 0.0f) * 0.05f; }
        else if (type == "wheel") { g_EditorCam.zoom *= (params.value("delta", 0.0f) > 0 ? 0.9f : 1.1f); }
        return {{"success", true}};
    }
    if (action == "hierarchy/get") return GetHierarchy();
    if (action == "entity/get") return GetEntity(params.value("id", 0u));
    if (action == "entity/update") return UpdateEntity(params);
    return {{"error", "NA"}};
}

nlohmann::json EditorService::GetHierarchy() {
    auto* s = Engine::Get().GetSceneManager()->GetCurrentScene();
    nlohmann::json res = nlohmann::json::array();
    if(s) for(size_t i=0; i<s->GetGameObjects().size(); ++i) res.push_back({{"id", (uint32_t)i}, {"name", s->GetGameObjects()[i]->name}});
    return {{"entities", res}};
}

nlohmann::json EditorService::GetEntity(uint32_t id) {
    auto* s = Engine::Get().GetSceneManager()->GetCurrentScene();
    if(!s || id >= s->GetGameObjects().size()) return {{"error", "NA"}};
    auto ent = s->GetGameObjects()[id];
    auto p = ent->GetTransform()->GetPosition();
    return {{"name", ent->name}, {"components", {{{"type","Transform"},{"data",{{"position",{p.x,p.y,p.z}}}}}}}};
}

nlohmann::json EditorService::UpdateEntity(const nlohmann::json& params) {
    uint32_t id = params.value("id", 0u);
    auto* s = Engine::Get().GetSceneManager()->GetCurrentScene();
    if(!s || id >= s->GetGameObjects().size()) return {{"success",false}};
    auto data = params["data"];
    Engine::Get().SubmitToMainThread([ent = s->GetGameObjects()[id], data](){
        if(data.contains("transform")) {
            auto p = data["transform"]["position"];
            ent->GetTransform()->SetPosition({p[0], p[1], p[2]});
        }
        if(data.contains("name")) ent->name = data["name"].get<std::string>();
    });
    return {{"success",true}};
}

void InternalDoRender() {
    if (!s_SceneSHM) s_SceneSHM = std::make_unique<EditorSharedMemory>("/prisma_scene_view", 1280 * 720 * 4);
    if (!s_GameSHM) s_GameSHM = std::make_unique<EditorSharedMemory>("/prisma_game_view", 1280 * 720 * 4);
    
    uint8_t* sP = (uint8_t*)s_SceneSHM->GetBuffer();
    uint8_t* gP = (uint8_t*)s_GameSHM->GetBuffer();
    if (!sP) return;

    // 核心修复：明亮的 Unity 蓝背景，确保不是黑屏
    auto drawClear = [](uint8_t* p, uint8_t r, uint8_t g, uint8_t b) {
        for(int i=0; i<1280*720; i++){
            p[i*4]=r; p[i*4+1]=g; p[i*4+2]=b; p[i*4+3]=255;
        }
    };
    drawClear(sP, 49, 77, 121); // Unity Blue
    drawClear(gP, 30, 30, 30);

    // 闪烁的心跳点 (左上角 10x10)
    static uint8_t flash = 0; flash += 10;
    for(int y=0; y<10; y++) for(int x=0; x<10; x++) {
        int idx = (y*1280+x)*4;
        sP[idx]=0; sP[idx+1]=255; sP[idx+2]=0; sP[idx+3]=flash;
    }

    auto* s = Engine::Get().GetSceneManager()->GetCurrentScene();
    if(!s) return;
    for(size_t i=0; i<s->GetGameObjects().size(); i++){
        auto p = s->GetGameObjects()[i]->GetTransform()->GetPosition();
        // Scene View 物体渲染
        int sx=(int)((p.x+g_EditorCam.x)*50*g_EditorCam.zoom)+640, sy=(int)(-(p.y+g_EditorCam.y)*50*g_EditorCam.zoom)+360;
        for(int dy=-15; dy<15; dy++) for(int dx=-15; dx<15; dx++){
            int px=sx+dx, py=sy+dy; if(px>=0&&px<1280&&py>=0&&py<720){
                int idx=(py*1280+px)*4; 
                sP[idx]=255; sP[idx+1]=255; sP[idx+2]=255; sP[idx+3]=255; // 补全 Alpha
            }
        }
        // Game View 物体渲染
        int gx=(int)(p.x*40)+640, gy=(int)(-p.y*40)+360;
        for(int dy=-10; dy<10; dy++) for(int dx=-10; dx<10; dx++){
            int px=gx+dx, py=gy+dy; if(px>=0&&px<1280&&py>=0&&py<720){
                int idx=(py*1280+px)*4; 
                gP[idx]=248; gP[idx+1]=78; gP[idx+2]=78; gP[idx+3]=255; // 补全 Alpha
            }
        }
    }
}

void* EditorService::GetViewportRawBuffer(size_t* s) { InternalDoRender(); *s=1280*720*4; return s_SceneSHM->GetBuffer(); }
void* EditorService::GetGameViewportRawBuffer(size_t* s) { *s=1280*720*4; return s_GameSHM->GetBuffer(); }

} // namespace Prisma
