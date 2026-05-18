#pragma once

#include "app/Application.h"
#include "scripting/ScriptEngine.h"
#include "core/EntityManager.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include <memory>
#include <vector>
#include <functional>

namespace Prisma {
namespace Graphic {
    class OrthographicCamera;
}

class Template3DApp : public Application {
public:
    Template3DApp();
    ~Template3DApp() override;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }
    void SetHeadlessConfig(uint32_t totalFrames, const std::string& outputPath,
                           uint32_t width = 80, uint32_t height = 60) {
        m_headlessCfg.enabled = true;
        m_headlessCfg.totalFrames = totalFrames;
        m_headlessCfg.outputPath = outputPath;
        m_headlessCfg.width = width;
        m_headlessCfg.height = height;
    }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;
    void OnShutdown() override;

private:
    // 渲染模式
    enum class RenderMode {
        Forward3D,     // 传统前向渲染: 用 ForwardPipeline 渲染 Cornell Box
        PathTracing    // 计算着色器路径追踪
    };

    // 初始化
    void InitForwardResources();
    void InitPathTracingResources();
    void InitPresentResources();

    // Forward 3D 渲染
    void RenderForward3D();

    // 路径追踪渲染
    void RenderPathTracing();

    // Overlay 回调 (fullscreen quad 显示路径追踪结果)
    void OnPresentOverlay(VkCommandBuffer cmd);

    void SavePathTracingOutput();

    // 调试统计覆盖层
    void DrawStatsOverlay();

    // 清理
    void CleanupPathTracingResources();
    void CleanupPresentResources();

    // Gizmo overlay 渲染（处理 Renderer gizmo 队列）
    void InitGizmoResources();
    void ProcessGizmoOverlay(VkCommandBuffer cmd);

    // 窗口 resize 处理
    void OnWindowResize(uint32_t w, uint32_t h);

    // 场景加载
    void LoadSceneFromJSON(const std::string& path);

    bool m_autoQuit = false;
    RenderMode m_renderMode = RenderMode::PathTracing;

    // ========== Forward 3D ==========
    std::unique_ptr<Graphic::IBuffer> m_cornellBoxVB;      // 顶点缓冲 (Vertex 格式)
    std::unique_ptr<Graphic::IBuffer> m_cornellBoxIB;      // 索引缓冲
    uint32_t m_cornellBoxIndexCount = 0;                   // 总索引数

    // ========== 路径追踪（抽象接口） ==========
    struct PathTracingResources {
        // 存储纹理（计算着色器写入，片段着色器采样，由 ResourceFactory 创建）
        std::unique_ptr<Graphic::ITexture> storageTexture;

        // Uniform 缓冲 (Camera UBO)
        std::unique_ptr<Graphic::IBuffer> cameraUBO;

        // SSBO (场景对象数据)
        std::unique_ptr<Graphic::IBuffer> sceneSSBO;

        // 计算管线（封装 VkPipeline + VkPipelineLayout + VkDescriptorSetLayout）
        std::unique_ptr<Graphic::IComputePipeline> computePipeline;

        // 描述符集
        std::shared_ptr<Graphic::IDescriptorSet> descriptorSet;

        // 累积帧计数器
        uint32_t frameCount = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        bool initialized = false;
    } m_ptRes;

    // ========== Present (全屏四边形覆盖) ==========
    struct PresentResources {
        VkDevice vkDevice = VK_NULL_HANDLE;

        // 全屏四边形绘制管线（保持原生，需 VkRenderPass 创建）
        VkShaderModule vertShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragShaderModule = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        // 抽象资源
        std::shared_ptr<Graphic::IDescriptorSetLayout> descriptorSetLayout;
        std::unique_ptr<Graphic::ISampler> sampler;
        std::shared_ptr<Graphic::IDescriptorSet> descriptorSet;

        // 当前帧图像信息
        VkExtent2D extent = {};

        bool initialized = false;
    } m_presentRes;

    // ========== 场景 SSBO 数据结构 ==========
    // 必须与 pathtrace.comp 中的 SceneObject 布局完全一致（std430）
    // 每个对象 4 x vec4 = 64 字节
    struct PTSceneObject {
        float p0[4];    // type=0(plane): point + type; type=1(sphere): center; type=2(box): center
        float p1[4];    // plane: normal; sphere: (r,0,0,0); box: (hx,hy,hz,0)
        float p2[4];    // plane: (umin,vmin,umax,vmax); box: (cosA,sinA,0,0); sphere: unused
        float color[4]; // rgb + emissive in w
    }; // 64 bytes

    // SSBO 顶层布局（std430，首个 int 后填充到 16 字节对齐）
    struct SceneDataSSBO {
        int objectCount;
        float pad1, pad2, pad3; // padding to 16B boundary
        PTSceneObject objects[32]; // max 32 scene objects
    };

    // ========== 相机 UBO 数据 ==========
    // 必须与 pathtrace.comp 中的 CameraUBO 布局完全一致（std140）
    struct CameraUBO {
        float cameraPos[4];     // offset 0
        float cameraDir[4];     // offset 16
        float cameraUp[4];      // offset 32
        float cameraRight[4];   // offset 48
        float fov;              // offset 64
        float aspectRatio;      // offset 68
        int frameCount;         // offset 72
        int maxBounces;         // offset 76
        int samplesPerPixel;    // offset 80
        int useAccumulation;    // offset 84
        int resetAccumulation;  // offset 88
        int padUBO;             // offset 92 (pad to 96 = next vec4 boundary in std140)
    }; // total 96 bytes

    // 相机控制器
    struct CameraControl {
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 2.5f);
        glm::vec3 target   = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);
        float fov          = 70.0f;
        float yaw          = 0.0f;
        float pitch        = 0.0f;
    } m_camera;

    struct HeadlessConfig {
        bool enabled = false;
        uint32_t totalFrames = 100;
        uint32_t width = 80;
        uint32_t height = 60;
        std::string outputPath = "output.png";
    } m_headlessCfg;

    // 状态
    bool m_pathTracingDirty = true;  // true → 重置累积
    bool m_sceneLoaded = false;

    // IRenderDevice 缓存
    Graphic::IRenderDevice* m_device = nullptr;

    // Gizmo overlay 资源
    std::shared_ptr<Graphic::IShader> m_gizmoVertShader;
    std::shared_ptr<Graphic::IShader> m_gizmoFragShader;
    std::shared_ptr<Graphic::IPipelineState> m_gizmoPSO;
    std::shared_ptr<Graphic::OrthographicCamera> m_gizmoCamera;
};

} // namespace Prisma
