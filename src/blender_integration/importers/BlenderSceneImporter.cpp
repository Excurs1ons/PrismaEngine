#include "BlenderSceneImporter.h"

#include "../ipc/BlenderIPCServer.h"
#include "../renderer/BlenderMesh.h"
#include "../renderer/BlenderMaterial.h"
#include "../../engine/Logger.h"
#include "../../engine/SceneManager.h"
#include "../../engine/Scene.h"
#include "../../engine/Entity.h"
#include "../../engine/transform/TransformComponent.h"
#include "../../engine/renderer/MeshComponent.h"
#include "../../engine/renderer/Material.h"
#include "../../engine/renderer/CameraComponent.h"
#include "../../engine/renderer/LightComponent.h"

#include <chrono>

namespace Prisma {

BlenderSceneImporter::BlenderSceneImporter() {
    // 获取IPC服务器单例
    ipc_server_ = BlenderIPCServer::Get();
    
    // 注册场景更新回调
    if (ipc_server_) {
        ipc_server_->RegisterSceneUpdateHandler(
            [this](const SceneData& scene_data) {
                OnSceneUpdate(scene_data);
            }
        );
    }
}

BlenderSceneImporter::~BlenderSceneImporter() {
    StopImport();
}

std::shared_ptr<BlenderSceneImporter> BlenderSceneImporter::Get() {
    static std::shared_ptr<BlenderSceneImporter> instance = std::make_shared<BlenderSceneImporter>();
    return instance;
}

bool BlenderSceneImporter::StartImport(const ImportOptions& options) {
    if (importing_) {
        LOG_WARNING("BlenderSceneImporter", "Import already in progress");
        return false;
    }
    
    import_options_ = options;
    importing_ = true;
    stats_.Reset();
    import_start_time_ = std::chrono::steady_clock::now();
    
    // 启动导入线程
    import_thread_ = std::thread(&BlenderSceneImporter::ImportThread, this);
    
    LOG_INFO("BlenderSceneImporter", "Starting import with options: meshes={}, materials={}, textures={}",
             options.import_meshes, options.import_materials, options.import_textures);
    
    return true;
}

void BlenderSceneImporter::StopImport() {
    if (!importing_) return;
    
    importing_ = false;
    
    if (import_thread_.joinable()) {
        import_thread_.join();
    }
    
    LOG_INFO("BlenderSceneImporter", "Import stopped");
}

void BlenderSceneImporter::SetProgressCallback(ImportProgressCallback callback) {
    progress_callback_ = std::move(callback);
}

void BlenderSceneImporter::SetCompleteCallback(ImportCompleteCallback callback) {
    complete_callback_ = std::move(callback);
}

void BlenderSceneImporter::ClearCurrentScene() {
    std::lock_guard<std::mutex> lock(scene_mutex_);
    current_scene_.Clear();
    mesh_cache_.clear();
    material_cache_.clear();
    entity_map_.clear();
}

void BlenderSceneImporter::EnableRealtimeSync(bool enable) {
    realtime_sync_enabled_ = enable;
    
    if (enable && ipc_server_) {
        // 请求Blender发送完整场景
        ipc_server_->SendSceneRequest();
    }
    
    LOG_INFO("BlenderSceneImporter", "Realtime sync {}", enable ? "enabled" : "disabled");
}

void BlenderSceneImporter::ImportThread() {
    LOG_INFO("BlenderSceneImporter", "Import thread started");
    
    try {
        // 等待IPC服务器接收场景数据
        // 在实际实现中，这里会等待并处理来自Blender的消息
        
        // 模拟导入过程（实际会从IPC队列获取场景数据）
        ReportProgress(0.1f, "Connecting to Blender...");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        ReportProgress(0.3f, "Receiving scene data...");
        
        // 如果IPC服务器已连接，请求同步
        if (ipc_server_ && ipc_server_->IsConnected()) {
            ipc_server_->SendSceneRequest();
            
            // 等待场景数据（这里简化处理）
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 处理消息队列
        if (ipc_server_) {
            ipc_server_->ProcessMessages();
        }
        
        ReportProgress(0.5f, "Processing objects...");
        
        // 导入场景数据
        {
            std::lock_guard<std::mutex> lock(scene_mutex_);
            if (!ImportSceneData(current_scene_)) {
                ReportComplete(false, "Failed to import scene data");
                return;
            }
        }
        
        ReportProgress(0.8f, "Finalizing...");
        
        // 计算统计信息
        auto now = std::chrono::steady_clock::now();
        stats_.import_time_ms = std::chrono::duration<double, std::milli>(now - import_start_time_).count();
        
        ReportProgress(1.0f, "Import complete");
        ReportComplete(true);
        
    } catch (const std::exception& e) {
        LOG_ERROR("BlenderSceneImporter", "Import failed: {}", e.what());
        ReportComplete(false, e.what());
    }
    
    importing_ = false;
}

bool BlenderSceneImporter::ImportSceneData(const SceneData& scene_data) {
    size_t total_objects = scene_data.objects.size() + 
                           scene_data.cameras.size() + 
                           scene_data.lights.size();
    
    stats_.total_objects = total_objects;
    size_t processed = 0;
    
    // 导入所有对象（可以并行执行）
    auto import_task = [this, &processed, total_objects](const ObjectData& obj_data) {
        bool result = ImportObject(obj_data);
        processed++;
        ReportProgress(0.5f + (0.4f * processed / total_objects), 
                      "Importing: " + obj_data.name);
        return result;
    };
    
    // 并行导入网格对象
    for (const auto& obj : scene_data.objects) {
        if (!import_task(obj)) {
            LOG_ERROR("BlenderSceneImporter", "Failed to import object: {}", obj.name);
        }
    }
    
    // 导入相机
    if (import_options_.import_cameras) {
        for (const auto& cam : scene_data.cameras) {
            if (!import_task(cam)) {
                LOG_ERROR("BlenderSceneImporter", "Failed to import camera: {}", cam.name);
            }
        }
    }
    
    // 导入光源
    if (import_options_.import_lights) {
        for (const auto& light : scene_data.lights) {
            if (!import_task(light)) {
                LOG_ERROR("BlenderSceneImporter", "Failed to import light: {}", light.name);
            }
        }
    }
    
    stats_.imported_objects = processed;
    
    LOG_INFO("BlenderSceneImporter", "Scene import complete: {} objects", processed);
    return true;
}

bool BlenderSceneImporter::ImportObject(const ObjectData& object_data) {
    switch (object_data.type) {
        case ObjectType::MESH:
            return ImportMesh(object_data);
        case ObjectType::CAMERA:
            return ImportCamera(object_data);
        case ObjectType::LIGHT:
            return ImportLight(object_data);
        default:
            return true; // 空对象跳过
    }
}

bool BlenderSceneImporter::ImportMesh(const ObjectData& object_data) {
    if (!import_options_.import_meshes) return true;
    
    try {
        // 创建网格
        std::shared_ptr<Mesh> mesh = nullptr;
        if (object_data.mesh_data) {
            mesh = CreateMeshFromData(*object_data.mesh_data, object_data.name);
            
            if (import_options_.optimize_meshes) {
                OptimizeMesh(*object_data.mesh_data);
            }
        }
        
        // 创建材质
        std::shared_ptr<Material> material = nullptr;
        if (object_data.material_data && import_options_.import_materials) {
            material = CreateMaterialFromData(*object_data.material_data);
        }
        
        // 创建实体
        auto scene = SceneManager::Get().GetCurrentScene();
        if (scene) {
            auto entity = scene->CreateEntity(object_data.name);
            
            // 添加变换组件
            auto& transform = entity->AddComponent<TransformComponent>();
            Transform corrected_transform = object_data.transform;
            if (import_options_.convert_to_y_up) {
                ApplyTransformCorrection(corrected_transform);
            }
            transform.SetLocalMatrix(corrected_transform.ToMatrix());
            
            // 添加网格组件
            if (mesh) {
                auto& mesh_comp = entity->AddComponent<MeshComponent>();
                mesh_comp.SetMesh(mesh);
                if (material) {
                    mesh_comp.SetMaterial(material);
                }
            }
            
            // 保存实体映射
            entity_map_[object_data.id] = entity;
            
            stats_.total_vertices += mesh ? mesh->GetVertexCount() : 0;
            stats_.total_triangles += mesh ? mesh->GetTriangleCount() : 0;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("BlenderSceneImporter", "Failed to import mesh {}: {}", object_data.name, e.what());
        return false;
    }
}

bool BlenderSceneImporter::ImportCamera(const ObjectData& object_data) {
    if (!import_options_.import_cameras) return true;
    
    try {
        auto scene = SceneManager::Get().GetCurrentScene();
        if (!scene) return false;
        
        auto entity = scene->CreateEntity(object_data.name);
        
        // 添加变换组件
        auto& transform = entity->AddComponent<TransformComponent>();
        Transform corrected_transform = object_data.transform;
        if (import_options_.convert_to_y_up) {
            ApplyTransformCorrection(corrected_transform);
        }
        transform.SetLocalMatrix(corrected_transform.ToMatrix());
        
        // 添加相机组件
        auto& camera = entity->AddComponent<CameraComponent>();
        
        // 设置相机属性
        camera.SetFov(object_data.camera_props.fov);
        camera.SetNearPlane(object_data.camera_props.near_plane);
        camera.SetFarPlane(object_data.camera_props.far_plane);
        
        if (object_data.camera_props.camera_type == CameraType::ORTHOGRAPHIC) {
            camera.SetOrthographic(true);
            camera.SetOrthoSize(object_data.camera_props.ortho_size);
        }
        
        entity_map_[object_data.id] = entity;
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("BlenderSceneImporter", "Failed to import camera {}: {}", object_data.name, e.what());
        return false;
    }
}

bool BlenderSceneImporter::ImportLight(const ObjectData& object_data) {
    if (!import_options_.import_lights) return true;
    
    try {
        auto scene = SceneManager::Get().GetCurrentScene();
        if (!scene) return false;
        
        auto entity = scene->CreateEntity(object_data.name);
        
        // 添加变换组件
        auto& transform = entity->AddComponent<TransformComponent>();
        Transform corrected_transform = object_data.transform;
        if (import_options_.convert_to_y_up) {
            ApplyTransformCorrection(corrected_transform);
        }
        transform.SetLocalMatrix(corrected_transform.ToMatrix());
        
        // 添加光源组件
        auto& light = entity->AddComponent<LightComponent>();
        
        // 设置光源类型
        switch (object_data.light_props.light_type) {
            case LightType::POINT:
                light.SetLightType(LightComponent::LightType::POINT);
                break;
            case LightType::DIRECTIONAL:
                light.SetLightType(LightComponent::LightType::DIRECTIONAL);
                break;
            case LightType::SPOT:
                light.SetLightType(LightComponent::LightType::SPOT);
                break;
            case LightType::AREA:
                light.SetLightType(LightComponent::LightType::AREA);
                break;
        }
        
        // 设置光源属性
        light.SetColor(object_data.light_props.color);
        light.SetIntensity(object_data.light_props.intensity);
        light.SetRange(object_data.light_props.range);
        
        if (object_data.light_props.light_type == LightType::SPOT) {
            light.SetSpotAngle(object_data.light_props.spot_angle);
            light.SetInnerSpotAngle(object_data.light_props.inner_spot_angle);
        }
        
        entity_map_[object_data.id] = entity;
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("BlenderSceneImporter", "Failed to import light {}: {}", object_data.name, e.what());
        return false;
    }
}

bool BlenderSceneImporter::ImportMaterial(const ObjectData& object_data) {
    // 材质导入已整合到ImportMesh中
    return true;
}

std::shared_ptr<class Mesh> BlenderSceneImporter::CreateMeshFromData(
    const MeshData& mesh_data, const std::string& name) {
    
    // 检查缓存
    auto it = mesh_cache_.find(mesh_data.name);
    if (it != mesh_cache_.end()) {
        return it->second;
    }
    
    // 使用新的BlenderMesh类创建网格
    auto blender_mesh = Renderer::BlenderMesh::CreateFromBlenderData(mesh_data);
    if (!blender_mesh || !blender_mesh->IsValid()) {
        LOG_WARNING("BlenderSceneImporter", "Failed to create mesh from data: {}", mesh_data.name);
        return nullptr;
    }
    
    // 优化网格
    if (import_options_.optimize_meshes) {
        blender_mesh->Optimize();
    }
    
    // 生成法线（如果需要）
    if (import_options_.generate_normals && !mesh_data.HasNormals()) {
        blender_mesh->GenerateNormals();
    }
    
    // TODO: 将BlenderMesh转换为引擎的Mesh类
    // 这里需要调用引擎的渲染系统创建实际的渲染网格
    
    // 临时：将BlenderMesh存储为引擎Mesh
    auto mesh = std::make_shared<class Mesh>();
    // 设置网格数据...
    
    // 添加到缓存
    mesh_cache_[mesh_data.name] = mesh;
    
    LOG_INFO("BlenderSceneImporter", "Created mesh: {} ({} vertices, {} triangles)", 
             mesh_data.name, blender_mesh->GetVertexCount(), blender_mesh->GetTriangleCount());
    
    return mesh;
}

std::shared_ptr<class Material> BlenderSceneImporter::CreateMaterialFromData(
    const MaterialData& material_data) {
    
    // 检查缓存
    auto it = material_cache_.find(material_data.name);
    if (it != material_cache_.end()) {
        return it->second;
    }
    
    // 使用新的BlenderMaterial类创建材质
    auto blender_material = Renderer::BlenderMaterial::CreateFromBlenderData(material_data);
    if (!blender_material || !blender_material->IsValid()) {
        if (import_options_.create_default_materials) {
            LOG_INFO("BlenderSceneImporter", "Creating default material for: {}", material_data.name);
            // 创建默认材质
            auto default_mat = std::make_shared<class Material>();
            default_mat->SetName(material_data.name);
            material_cache_[material_data.name] = default_mat;
            return default_mat;
        }
        LOG_WARNING("BlenderSceneImporter", "Failed to create material from data: {}", material_data.name);
        return nullptr;
    }
    
    // TODO: 将BlenderMaterial转换为引擎的Material类
    // 这里需要调用引擎的渲染系统创建实际的渲染材质
    
    // 临时：将BlenderMaterial存储为引擎Material
    auto material = std::make_shared<class Material>();
    material->SetName(material_data.name);
    
    // 设置PBR属性
    material->SetAlbedoColor(glm::vec4(blender_material->GetAlbedoColor(), blender_material->GetAlpha()));
    material->SetMetallic(blender_material->GetMetallic());
    material->SetRoughness(blender_material->GetRoughness());
    material->SetEmissiveColor(blender_material->GetEmissiveColor());
    material->SetEmissiveIntensity(blender_material->GetEmissiveIntensity());
    
    // 设置透明度
    if (blender_material->IsTransparent()) {
        material->SetBlendMode(Material::BlendMode::TRANSPARENT);
    }
    
    // 设置双面渲染
    if (blender_material->IsDoubleSided()) {
        material->SetCullMode(Material::CullMode::NONE);
    }
    
    // 异步加载纹理
    if (import_options_.import_textures && import_options_.load_textures_async) {
        // TODO: 异步纹理加载
    }
    
    // 添加到缓存
    material_cache_[material_data.name] = material;
    
    LOG_INFO("BlenderSceneImporter", "Created material: {} (PBR)", material_data.name);
    
    return material;
}

std::shared_ptr<class Camera> BlenderSceneImporter::CreateCameraFromData(
    const ObjectData& camera_data) {
    // TODO: 实现相机创建
    return nullptr;
}

std::shared_ptr<class Light> BlenderSceneImporter::CreateLightFromData(
    const ObjectData& light_data) {
    // TODO: 实现光源创建
    return nullptr;
}

void BlenderSceneImporter::ApplyTransformCorrection(Transform& transform) const {
    if (!import_options_.convert_to_y_up) return;
    
    // Blender是Z-up，引擎使用Y-up
    // 需要旋转 -90 度绕X轴
    // 这是一个简化的实现
    
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::vec3 position = transform.position.ToGlm();
    glm::quat rotation_glm = transform.rotation.ToGlm();
    glm::vec3 scale = transform.scale.ToGlm();
    
    // 应用旋转
    position = rotation * glm::vec4(position, 1.0f);
    
    transform.position = Vector3(position.x, position.y, position.z);
    // 旋转修正更复杂，需要四元数乘法
}

void BlenderSceneImporter::OptimizeMesh(MeshData& mesh_data) const {
    if (!import_options_.optimize_meshes) return;
    
    // TODO: 实现网格优化
    // - 移除重复顶点
    // - 简化网格
    // - 重用顶点
}

void BlenderSceneImporter::GenerateNormals(MeshData& mesh_data) const {
    if (mesh_data.HasNormals()) return;
    
    // 生成平滑法线
    // TODO: 实现法线生成
}

void BlenderSceneImporter::ReportProgress(float progress, const std::string& message) {
    LOG_INFO("BlenderSceneImporter", "[{:.0}%] {}", progress * 100, message);
    
    if (progress_callback_) {
        progress_callback_(progress, message);
    }
}

void BlenderSceneImporter::ReportComplete(bool success, const std::string& error) {
    importing_ = false;
    
    if (success) {
        LOG_INFO("BlenderSceneImporter", "Import complete");
    } else {
        LOG_ERROR("BlenderSceneImporter", "Import failed: {}", error);
    }
    
    if (complete_callback_) {
        complete_callback_(current_scene_, success, error);
    }
}

void BlenderSceneImporter::OnSceneUpdate(const SceneData& scene_data) {
    if (!realtime_sync_enabled_) return;
    
    std::lock_guard<std::mutex> lock(scene_mutex_);
    current_scene_ = scene_data;
    
    LOG_INFO("BlenderSceneImporter", "Received scene update: {} objects", 
             scene_data.objects.size());
    
    // 增量更新（后续可以实现）
    // 目前是完整替换
    ImportSceneData(scene_data);
}

void BlenderSceneImporter::OnObjectAdded(const ObjectData& object_data) {
    std::lock_guard<std::mutex> lock(scene_mutex_);
    ImportObject(object_data);
}

void BlenderSceneImporter::OnObjectTransformed(const ObjectData& object_data) {
    auto it = entity_map_.find(object_data.id);
    if (it == entity_map_.end()) return;
    
    auto entity = it->second;
    auto transform_comp = entity->GetComponent<TransformComponent>();
    
    if (transform_comp) {
        Transform corrected_transform = object_data.transform;
        if (import_options_.convert_to_y_up) {
            ApplyTransformCorrection(corrected_transform);
        }
        transform_comp->SetLocalMatrix(corrected_transform.ToMatrix());
    }
}

} // namespace Prisma