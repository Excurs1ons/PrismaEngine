#pragma once

#include "SceneData.h"
#include "../ipc/BlenderIPCServer.h"

#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>

namespace Prisma {

// 导入进度回调
using ImportProgressCallback = std::function<void(float progress, const std::string& message)>;
using ImportCompleteCallback = std::function<void(const SceneData& scene_data, bool success, const std::string& error)>;

// 导入选项
struct ImportOptions {
    bool import_meshes = true;
    bool import_materials = true;
    bool import_textures = true;
    bool import_animations = false;
    bool import_lights = true;
    bool import_cameras = true;
    
    // 网格优化
    bool optimize_meshes = true;
    bool generate_normals = false; // 如果没有法线则生成
    bool generate_tangents = false;
    
    // 材质设置
    bool create_default_materials = true;
    bool load_textures_async = true;
    
    // 变换
    float scale_factor = 1.0f;
    bool convert_to_y_up = true; // Blender是Z-up，转换为Y-up
    
    // 性能
    size_t max_texture_size = 4096; // 最大纹理尺寸
    bool use_compressed_textures = true;
};

class BlenderSceneImporter {
public:
    BlenderSceneImporter();
    ~BlenderSceneImporter();
    
    // 单例访问
    static std::shared_ptr<BlenderSceneImporter> Get();
    
    // 导入控制
    bool StartImport(const ImportOptions& options = ImportOptions());
    void StopImport();
    bool IsImporting() const { return importing_; }
    
    // 回调注册
    void SetProgressCallback(ImportProgressCallback callback);
    void SetCompleteCallback(ImportCompleteCallback callback);
    
    // 场景管理
    const SceneData& GetCurrentScene() const { return current_scene_; }
    void ClearCurrentScene();
    
    // 实时同步
    void EnableRealtimeSync(bool enable);
    bool IsRealtimeSyncEnabled() const { return realtime_sync_enabled_; }
    
    // 统计信息
    struct ImportStats {
        size_t total_objects = 0;
        size_t imported_objects = 0;
        size_t total_triangles = 0;
        size_t total_vertices = 0;
        size_t total_textures = 0;
        double import_time_ms = 0.0;
        
        void Reset() {
            total_objects = 0;
            imported_objects = 0;
            total_triangles = 0;
            total_vertices = 0;
            total_textures = 0;
            import_time_ms = 0.0;
        }
    };
    
    const ImportStats& GetStats() const { return stats_; }
    
private:
    // 导入流程
    void ImportThread();
    bool ImportSceneData(const SceneData& scene_data);
    
    // 组件导入
    bool ImportObject(const ObjectData& object_data);
    bool ImportMesh(const ObjectData& object_data);
    bool ImportMaterial(const ObjectData& object_data);
    bool ImportCamera(const ObjectData& object_data);
    bool ImportLight(const ObjectData& object_data);
    
    // 资源创建
    std::shared_ptr<class Mesh> CreateMeshFromData(const MeshData& mesh_data, const std::string& name);
    std::shared_ptr<class Material> CreateMaterialFromData(const MaterialData& material_data);
    std::shared_ptr<class Camera> CreateCameraFromData(const ObjectData& camera_data);
    std::shared_ptr<class Light> CreateLightFromData(const ObjectData& light_data);
    
    // 工具函数
    void ApplyTransformCorrection(Transform& transform) const;
    void OptimizeMesh(MeshData& mesh_data) const;
    void GenerateNormals(MeshData& mesh_data) const;
    
    // 进度报告
    void ReportProgress(float progress, const std::string& message);
    void ReportComplete(bool success, const std::string& error = "");
    
    // IPC回调
    void OnSceneUpdate(const SceneData& scene_data);
    void OnObjectAdded(const ObjectData& object_data);
    void OnObjectTransformed(const ObjectData& object_data);
    
private:
    std::atomic<bool> importing_{false};
    std::atomic<bool> realtime_sync_enabled_{false};
    
    ImportOptions import_options_;
    SceneData current_scene_;
    ImportStats stats_;
    
    std::thread import_thread_;
    std::mutex scene_mutex_;
    
    // 回调
    ImportProgressCallback progress_callback_;
    ImportCompleteCallback complete_callback_;
    
    // 资源映射
    std::unordered_map<std::string, std::shared_ptr<class Mesh>> mesh_cache_;
    std::unordered_map<std::string, std::shared_ptr<class Material>> material_cache_;
    std::unordered_map<std::string, std::shared_ptr<class Entity>> entity_map_;
    
    // IPC连接
    std::shared_ptr<BlenderIPCServer> ipc_server_;
    
    // 时间统计
    std::chrono::steady_clock::time_point import_start_time_;
};

} // namespace Prisma