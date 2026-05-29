#include "EditorService.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "core/AssetDatabase.h"
#include "transform/Transform.h"
#include "transform/Camera.h"
#include "graphic/MeshRenderer.h"
#include "graphic/LightComponent.h"
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

glz::json_t EditorService::Dispatch(const std::string& action, const glz::json_t& params) {
    auto& engine = Engine::Get();
    auto* s = engine.GetSceneManager()->GetCurrentScene();

    if (action == "console/get") {
        auto logs = Logger::Get().GetRecentLogs(30);
        glz::json_t::array_t res;
        for(auto& l : logs) {
            res.push_back(glz::json_t::object_t{
                {"level", static_cast<double>(l.level)}, 
                {"msg", l.message}, 
                {"tag", l.category}
            });
        }
        return glz::json_t::object_t{{"logs", std::move(res)}};
    }
    if (action == "viewport/input") {
        auto& obj = params.get_object();
        std::string type = obj.contains("type") ? obj.at("type").get_string() : "";
        if (type == "mouseMove") { 
            g_EditorCam.x += (obj.contains("dx") ? obj.at("dx").get_number() : 0.0) * 0.05f; 
            g_EditorCam.y -= (obj.contains("dy") ? obj.at("dy").get_number() : 0.0) * 0.05f; 
        }
        else if (type == "wheel") { 
            g_EditorCam.zoom *= ((obj.contains("delta") ? obj.at("delta").get_number() : 0.0) > 0 ? 0.9f : 1.1f); 
        }
        return glz::json_t::object_t{{"success", true}};
    }
    if (action == "hierarchy/get") return GetHierarchy();
    if (action == "entity/get") {
        auto& obj = params.get_object();
        uint32_t id = obj.contains("id") ? static_cast<uint32_t>(obj.at("id").get_number()) : 0u;
        return GetEntity(id);
    }
    if (action == "entity/update") return UpdateEntity(params);
    if (action == "assets/list") return GetAssets();
    if (action == "engine/status") return GetStatus();
    return glz::json_t::object_t{{"error", "NA"}};
}

glz::json_t EditorService::GetStatus() {
    auto& engine = Engine::Get();
    auto* s = engine.GetSceneManager()->GetCurrentScene();
    return glz::json_t::object_t{
        {"fps", engine.GetFrameStats().FPS}, 
        {"gpu", engine.GetGPUName()}, 
        {"scene", s ? s->GetName() : "None"}, 
        {"objects", s ? static_cast<double>(s->GetNodes().size()) : 0.0}
    };
}

glz::json_t EditorService::GetAssets() {
    glz::json_t::array_t res;
    for(const auto& [p, m] : AssetDatabase::Get().GetAllMetadata()) {
        res.push_back(glz::json_t::object_t{{"path", p}, {"type", m.type}});
    }
    return glz::json_t::object_t{{"assets", std::move(res)}};
}

glz::json_t EditorService::GetHierarchy() {
    auto* s = Engine::Get().GetSceneManager()->GetCurrentScene();
    glz::json_t::array_t res;
    if(s) {
        const auto& nodes = s->GetNodes();
        for(size_t i=0; i<nodes.size(); ++i) {
            res.push_back(glz::json_t::object_t{
                {"id", static_cast<double>(i)}, 
                {"name", s->GetNodeName(nodes[i])}
            });
        }
    }
    return glz::json_t::object_t{{"entities", std::move(res)}};
}

static const char* LightTypeToString(Prisma::Graphic::LightComponent::LightType type) {
    using LT = Prisma::Graphic::LightComponent::LightType;
    switch (type) {
        case LT::Directional: return "Directional";
        case LT::Point:       return "Point";
        case LT::Spot:        return "Spot";
        case LT::Ambient:     return "Ambient";
        default:              return "Unknown";
    }
}

glz::json_t EditorService::GetEntity(uint32_t id) {
    auto* s = Engine::Get().GetSceneManager()->GetCurrentScene();
    const auto& nodes = s->GetNodes();
    if(!s || id >= nodes.size()) return glz::json_t::object_t{{"error", "NA"}};
    auto ent = nodes[id];
    
    glz::json_t::array_t components;

    // ── Transform ──
    auto transform = s->GetComponent<Transform>(ent);
    if (transform) {
        auto p = transform->GetPosition();
        auto r = transform->GetEulerAngles(); // pitch, yaw, roll in radians
        auto sc = transform->GetScale();
        glz::json_t::object_t data;
        data["position"] = glz::json_t::array_t{ p.x, p.y, p.z };
        data["rotation"] = glz::json_t::array_t{ glm::degrees(r.x), glm::degrees(r.y), glm::degrees(r.z) };
        data["scale"]    = glz::json_t::array_t{ sc.x, sc.y, sc.z };
        components.push_back(glz::json_t::object_t{{"type", "Transform"}, {"data", std::move(data)}});
    }

    // ── MeshRenderer ──
    auto meshRenderer = s->GetComponent<Prisma::Graphic::MeshRenderer>(ent);
    if (meshRenderer) {
        auto d = meshRenderer->GetData();
        glz::json_t::object_t data;
        data["mesh"]     = d.meshPath;
        data["material"] = d.material;
        data["color"]    = glz::json_t::array_t{ d.color[0], d.color[1], d.color[2], d.color[3] };
        components.push_back(glz::json_t::object_t{{"type", "MeshRenderer"}, {"data", std::move(data)}});
    }

    // ── Camera ──
    auto camera = s->GetComponent<Prisma::Graphic::Camera>(ent);
    if (camera) {
        auto d = camera->GetData();
        glz::json_t::object_t data;
        data["projection"] = d.projectionMode == Prisma::Graphic::ProjectionMode::Perspective ? "Perspective" : "Orthographic";
        data["fov"]  = d.fovDeg;
        data["near"] = d.nearPlane;
        data["far"]  = d.farPlane;
        components.push_back(glz::json_t::object_t{{"type", "Camera"}, {"data", std::move(data)}});
    }

    // ── Light ──
    auto light = s->GetComponent<Prisma::Graphic::LightComponent>(ent);
    if (light) {
        auto d = light->GetData();
        glz::json_t::object_t data;
        data["lightType"] = LightTypeToString(d.type);
        data["color"]     = glz::json_t::array_t{ d.color[0], d.color[1], d.color[2] };
        data["intensity"] = d.intensity;
        data["range"]     = d.range;
        components.push_back(glz::json_t::object_t{{"type", "Light"}, {"data", std::move(data)}});
    }

    return glz::json_t::object_t{
        {"name", s->GetNodeName(ent)}, 
        {"components", std::move(components)}
    };
}

glz::json_t EditorService::UpdateEntity(const glz::json_t& params) {
    auto& obj = params.get_object();
    uint32_t id = obj.contains("id") ? static_cast<uint32_t>(obj.at("id").get_number()) : 0u;
    auto* s = Engine::Get().GetSceneManager()->GetCurrentScene();
    const auto& nodes = s->GetNodes();
    if(!s || id >= nodes.size()) return glz::json_t::object_t{{"success", false}};
    
    if (!obj.contains("data")) return glz::json_t::object_t{{"success", false}};
    auto data = obj.at("data");
    
    Engine::Get().SubmitToMainThread([s, ent = nodes[id], data](){
        auto& dataObj = data.get_object();
        if(dataObj.contains("transform")) {
            auto& trans = dataObj.at("transform").get_object();
            if (trans.contains("position")) {
                auto& p = trans.at("position").get_array();
                auto transform = s->GetComponent<Transform>(ent);
                if (transform) {
                    transform->SetPosition({
                        static_cast<float>(p[0].get_number()), 
                        static_cast<float>(p[1].get_number()), 
                        static_cast<float>(p[2].get_number())
                    });
                }
            }
        }
        if(dataObj.contains("name")) s->SetNodeName(ent, dataObj.at("name").get_string());
    });
    return glz::json_t::object_t{{"success", true}};
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
    const auto& nodes = s->GetNodes();
    for(size_t i=0; i<nodes.size(); i++){
        auto transform = s->GetComponent<Transform>(nodes[i]);
        auto p = transform ? transform->GetPosition() : PrismaMath::vec3(0);
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
