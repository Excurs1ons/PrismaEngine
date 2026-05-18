#include "Template3DApp.h"
#include "PathtraceCompSPIRV.h"
#include "FullscreenVertSPIRV.h"
#include "PresentFragSPIRV.h"

#include "graphic/RenderSystem.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/adapters/vulkan/RenderDeviceVulkan.h"
#include "graphic/adapters/vulkan/VulkanResources.h"
#include "graphic/RenderDesc.h"
#include "app/Engine.h"
#include "core/EntityManager.h"
#include "SceneManager.h"
#include "scene/Scene.h"
#include "core/Event.h"
#include "Logger.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <stb_image_write.h>

#include <glaze/glaze.hpp>
#include <SDL3/SDL_scancode.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>

namespace Prisma {

static VkShaderModule CreateShaderModule(VkDevice device, const uint32_t* code, uint32_t size) {
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = size * sizeof(uint32_t);
    ci.pCode = code;
    VkShaderModule sm = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &ci, nullptr, &sm);
    return sm;
}

} // namespace Prisma

// JSON scene structs for Glaze deserialization (global scope)
struct CameraConfig {
    std::array<double, 3> position = {0.0, 0.0, 2.5};
    double fov = 70.0;
};

struct ObjectConfig {
    std::string type = "plane";
    std::array<double, 3> point = {0.0, 0.0, 0.0};
    std::array<double, 3> normal = {0.0, 1.0, 0.0};
    std::array<double, 4> bounds = {-1.0, -1.0, 1.0, 1.0};
    std::array<double, 3> center = {0.0, 0.0, 0.0};
    std::array<double, 3> halfSize = {0.2, 0.2, 0.2};
    double radius = 0.25;
    double rotation = 0.0;
    std::array<double, 3> axis = {0.0, 1.0, 0.0};
    std::array<double, 3> color = {0.5, 0.5, 0.5};
    double emissive = 0.0;
};

struct SceneConfig {
    int version = 1;
    CameraConfig camera;
    std::vector<ObjectConfig> objects;
};

namespace glz {
template<> struct meta<CameraConfig> {
    using T = CameraConfig;
    static constexpr auto value = object(
        &T::position, &T::fov
    );
};

template<> struct meta<ObjectConfig> {
    using T = ObjectConfig;
    static constexpr auto value = object(
        &T::type, &T::point, &T::normal, &T::bounds,
        &T::center, &T::halfSize, &T::radius, &T::rotation,
        &T::axis, &T::color, &T::emissive
    );
};

template<> struct meta<SceneConfig> {
    using T = SceneConfig;
    static constexpr auto value = object(
        &T::version, &T::camera, &T::objects
    );
};
} // namespace glz

namespace Prisma {
using namespace Graphic;

// ============================================================================
// Template3DApp
// ============================================================================

Template3DApp::Template3DApp()
    : Application({"Template3D", "", 1280, 720, false, true, Graphic::PresentMode::Mailbox, 0})
{
}

Template3DApp::~Template3DApp() {
}

void Template3DApp::LoadSceneFromJSON(const std::string& path) {
    LOG_INFO("Template3D", "加载场景文件: {}", path);

    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_ERROR("Template3D", "无法打开场景文件: {}", path);
        return;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    std::string jsonStr = buf.str();

    SceneConfig cfg;
    auto ec = glz::read_json(cfg, jsonStr);
    if (ec) {
        LOG_ERROR("Template3D", "场景 JSON 解析失败: {}", glz::format_error(ec, jsonStr));
        return;
    }

    // 相机
    m_camera.position = glm::vec3(
        (float)cfg.camera.position[0],
        (float)cfg.camera.position[1],
        (float)cfg.camera.position[2]
    );
    m_camera.fov = (float)cfg.camera.fov;
    LOG_INFO("Template3D", "  相机: pos=({:.1f},{:.1f},{:.1f}) fov={:.1f}",
             m_camera.position.x, m_camera.position.y, m_camera.position.z, m_camera.fov);

    // 填充 SSBO 数据
    SceneDataSSBO ssboData{};
    uint32_t objCount = std::min((uint32_t)cfg.objects.size(), (uint32_t)32);
    ssboData.objectCount = (int)objCount;

    for (uint32_t i = 0; i < objCount; i++) {
        auto& obj = cfg.objects[i];
        auto& ptObj = ssboData.objects[i];

        if (obj.type == "plane") {
            ptObj.p0[0] = (float)obj.point[0];
            ptObj.p0[1] = (float)obj.point[1];
            ptObj.p0[2] = (float)obj.point[2];
            ptObj.p0[3] = 0.0f; // type=0
            ptObj.p1[0] = (float)obj.normal[0];
            ptObj.p1[1] = (float)obj.normal[1];
            ptObj.p1[2] = (float)obj.normal[2];
            ptObj.p1[3] = 0.0f;
            ptObj.p2[0] = (float)obj.bounds[0];
            ptObj.p2[1] = (float)obj.bounds[1];
            ptObj.p2[2] = (float)obj.bounds[2];
            ptObj.p2[3] = (float)obj.bounds[3];
            LOG_INFO("Template3D", "  对象[{}]: plane point=({:.2f},{:.2f},{:.2f}) normal=({:.2f},{:.2f},{:.2f})",
                     i, ptObj.p0[0], ptObj.p0[1], ptObj.p0[2],
                     ptObj.p1[0], ptObj.p1[1], ptObj.p1[2]);
        } else if (obj.type == "sphere") {
            ptObj.p0[0] = (float)obj.center[0];
            ptObj.p0[1] = (float)obj.center[1];
            ptObj.p0[2] = (float)obj.center[2];
            ptObj.p0[3] = 1.0f; // type=1
            ptObj.p1[0] = (float)obj.radius;
            ptObj.p1[1] = 0.0f;
            ptObj.p1[2] = 0.0f;
            ptObj.p1[3] = 0.0f;
            LOG_INFO("Template3D", "  对象[{}]: sphere center=({:.2f},{:.2f},{:.2f}) r={:.2f}",
                     i, ptObj.p0[0], ptObj.p0[1], ptObj.p0[2], ptObj.p1[0]);
        } else if (obj.type == "box") {
            ptObj.p0[0] = (float)obj.center[0];
            ptObj.p0[1] = (float)obj.center[1];
            ptObj.p0[2] = (float)obj.center[2];
            ptObj.p0[3] = 2.0f; // type=2
            ptObj.p1[0] = (float)obj.halfSize[0];
            ptObj.p1[1] = (float)obj.halfSize[1];
            ptObj.p1[2] = (float)obj.halfSize[2];
            ptObj.p1[3] = 0.0f;
            // 旋转矩阵
            float angleRad = glm::radians((float)obj.rotation);
            ptObj.p2[0] = cos(angleRad);
            ptObj.p2[1] = sin(angleRad);
            ptObj.p2[2] = 0.0f;
            ptObj.p2[3] = 0.0f;
            LOG_INFO("Template3D", "  对象[{}]: box center=({:.2f},{:.2f},{:.2f}) hs=({:.2f},{:.2f},{:.2f}) rot={:.1f}",
                     i, ptObj.p0[0], ptObj.p0[1], ptObj.p0[2],
                     ptObj.p1[0], ptObj.p1[1], ptObj.p1[2], obj.rotation);
        } else {
            LOG_WARN("Template3D", "  对象[{}]: 未知类型 '{}'，跳过", i, obj.type);
            ssboData.objectCount--;
            continue;
        }

        ptObj.color[0] = (float)obj.color[0];
        ptObj.color[1] = (float)obj.color[1];
        ptObj.color[2] = (float)obj.color[2];
        ptObj.color[3] = (float)obj.emissive;
    }

    // 上传 SSBO 数据（通过抽象接口自动处理 map/flush/unmap）
    if (m_ptRes.sceneSSBO) {
        m_ptRes.sceneSSBO->UpdateData(&ssboData, sizeof(ssboData), 0);

        LOG_INFO("Template3D", "  上传 {} 个场景对象到 SSBO ({} bytes)", ssboData.objectCount, sizeof(ssboData));

        // 验证回读 SSBO 数据
        SceneDataSSBO checkData{};
        m_ptRes.sceneSSBO->ReadData(&checkData, sizeof(checkData), 0);
        LOG_INFO("Template3D", "  SSBO 回读: count={} sizeof={}", checkData.objectCount, (int)sizeof(checkData));
        if (checkData.objectCount > 0) {
            LOG_INFO("Template3D", "  首对象: p0=({:.1f},{:.1f},{:.1f}) c=({:.1f},{:.1f},{:.1f}) e={:.1f}",
                     checkData.objects[0].p0[0], checkData.objects[0].p0[1], checkData.objects[0].p0[2],
                     checkData.objects[0].color[0], checkData.objects[0].color[1], checkData.objects[0].color[2],
                     checkData.objects[0].color[3]);
            // Check light object
            for (int i = 0; i < checkData.objectCount; i++) {
                if (checkData.objects[i].color[3] > 1.0f) {
                    LOG_INFO("Template3D", "  找到光源对象[{}]: emissive={}", i, checkData.objects[i].color[3]);
                }
            }
        } else {
            LOG_WARN("Template3D", "  警告: SSBO 对象数为零或负数!");
        }
    }

    m_sceneLoaded = true;
}

int Template3DApp::OnInitialize() {
    LOG_INFO("Template3D", "3D 模板初始化 (路径追踪场景 SSBO)");

    if (m_headlessCfg.enabled) {
        m_Spec.Width = m_headlessCfg.width;
        m_Spec.Height = m_headlessCfg.height;
        LOG_INFO("Template3D", "头模式分辨率: {}x{}", m_Spec.Width, m_Spec.Height);
    }

    m_device = Engine::Get().GetRenderSystem()->GetDevice();
    if (!m_device) {
        LOG_ERROR("Template3D", "无法获取渲染设备");
        return -1;
    }

    InitForwardResources();
    InitPathTracingResources();

    // 加载场景文件（相对于可执行文件目录）
    std::string scenePath = "assets/scenes/pt_scene.json";
    LOG_INFO("Template3D", "工作目录: {} 场景路径: {}", std::filesystem::current_path().string(), scenePath);
    LoadSceneFromJSON(scenePath);
    LOG_INFO("Template3D", "场景加载状态: m_sceneLoaded={}", m_sceneLoaded);

    if (!m_headlessCfg.enabled) {
        InitPresentResources();
    } else {
        LOG_INFO("Template3D", "头模式：跳过 present 资源初始化");
    }

    LOG_INFO("Template3D", "按 P 切换渲染模式，R 重置路径追踪累积");
    return 0;
}

void Template3DApp::InitForwardResources() {
    LOG_INFO("Template3D", "Forward3D 资源初始化（占位）");
}

void Template3DApp::InitPathTracingResources() {
    auto& pt = m_ptRes;
    pt.width = m_Spec.Width;
    pt.height = m_Spec.Height;

    auto* factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("Template3D", "无法获取资源工厂");
        return;
    }

    // 1. 创建存储纹理（计算着色器写入，片段着色器采样）
    TextureDesc texDesc{};
    texDesc.type = TextureType::Texture2D;
    texDesc.format = TextureFormat::RGBA32_Float;
    texDesc.width = pt.width;
    texDesc.height = pt.height;
    texDesc.depth = 1;
    texDesc.mipLevels = 1;
    texDesc.arraySize = 1;
    texDesc.allowShaderResource = true;
    texDesc.allowUnorderedAccess = true;
    pt.storageTexture = factory->CreateTextureImpl(texDesc);
    if (!pt.storageTexture) {
        LOG_ERROR("Template3D", "创建存储纹理失败");
        return;
    }

    // 2. 创建 Camera UBO
    BufferDesc uboDesc{};
    uboDesc.type = BufferType::Constant;
    uboDesc.size = sizeof(CameraUBO);
    uboDesc.usage = BufferUsage::Dynamic;
    pt.cameraUBO = factory->CreateBufferImpl(uboDesc);
    if (!pt.cameraUBO) {
        LOG_ERROR("Template3D", "创建 Camera UBO 失败");
        return;
    }

    // 3. 创建 Scene SSBO
    BufferDesc ssboDesc{};
    ssboDesc.type = BufferType::Structured;
    ssboDesc.size = sizeof(SceneDataSSBO);
    ssboDesc.usage = BufferUsage::Dynamic;
    pt.sceneSSBO = factory->CreateBufferImpl(ssboDesc);
    if (!pt.sceneSSBO) {
        LOG_ERROR("Template3D", "创建 Scene SSBO 失败");
        return;
    }

    // 清空 SSBO（防止未初始化的对象数据）
    SceneDataSSBO emptySSBO{};
    pt.sceneSSBO->UpdateData(&emptySSBO, sizeof(emptySSBO), 0);

    // 4. 从 SPIRV 创建计算着色器
    std::vector<uint8_t> bytecode(
        reinterpret_cast<const uint8_t*>(PATHTRACE_COMP_SPV_SPV),
        reinterpret_cast<const uint8_t*>(PATHTRACE_COMP_SPV_SPV + PATHTRACE_COMP_SPV_SPV_SIZE));

    ShaderReflection reflection;
    reflection.Resources = {
        {"outputImage", ShaderResource::Type::Image2D, 0, 0, 1, 0},
        {"accumImage",  ShaderResource::Type::Image2D, 0, 1, 1, 0},
        {"cameraUBO",   ShaderResource::Type::UniformBuffer, 0, 2, 1, sizeof(CameraUBO)},
        {"sceneSSBO",   ShaderResource::Type::StorageBuffer, 0, 3, 1, sizeof(SceneDataSSBO)},
    };

    ShaderDesc shaderDesc{};
    shaderDesc.type = ShaderType::Compute;
    shaderDesc.entryPoint = "main";
    shaderDesc.language = ShaderLanguage::SPIRV;
    shaderDesc.filename = "pathtrace.comp";

    auto shader = factory->CreateShaderImpl(shaderDesc, bytecode, reflection);
    if (!shader) {
        LOG_ERROR("Template3D", "创建计算着色器失败");
        return;
    }

    // 5. 创建计算管线（封装 ShaderModule + PipelineLayout + ComputePipeline）
    pt.computePipeline = factory->CreateComputePipelineImpl();
    if (!pt.computePipeline) {
        LOG_ERROR("Template3D", "创建计算管线对象失败");
        return;
    }

    pt.computePipeline->SetShader(std::move(shader));
    if (!pt.computePipeline->Create(m_device)) {
        LOG_ERROR("Template3D", "编译计算管线失败");
        return;
    }

    // 6. 创建描述符集
    const auto& layouts = pt.computePipeline->GetDescriptorSetLayouts();
    if (layouts.empty()) {
        LOG_ERROR("Template3D", "计算管线没有描述符集布局");
        return;
    }

    pt.descriptorSet = factory->CreateDescriptorSet(layouts[0].get());
    if (!pt.descriptorSet) {
        LOG_ERROR("Template3D", "创建描述符集失败");
        return;
    }

    // 7. 绑定资源到描述符集
    pt.descriptorSet->BindStorageImage(0, pt.storageTexture.get());
    pt.descriptorSet->BindStorageImage(1, pt.storageTexture.get());
    pt.descriptorSet->BindBuffer(2, pt.cameraUBO.get(), 0, sizeof(CameraUBO),
                                 DescriptorType::UniformBuffer);
    pt.descriptorSet->BindBuffer(3, pt.sceneSSBO.get(), 0, sizeof(SceneDataSSBO),
                                 DescriptorType::StorageBuffer);
    pt.descriptorSet->Update();

    pt.initialized = true;
    LOG_INFO("Template3D", "路径追踪资源初始化完成 ({}x{})", pt.width, pt.height);
}

void Template3DApp::InitPresentResources() {
    auto& pr = m_presentRes;
    VkDevice vkDev = m_device->GetVkDevice();
    if (!vkDev) return;
    pr.vkDevice = vkDev;
    VkResult err;

    auto* factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("Template3D", "无法获取资源工厂");
        return;
    }

    pr.vertShaderModule = CreateShaderModule(vkDev, FULLSCREEN_VERT_SPV_SPV, FULLSCREEN_VERT_SPV_SPV_SIZE);
    pr.fragShaderModule = CreateShaderModule(vkDev, PRESENT_FRAG_SPV_SPV, PRESENT_FRAG_SPV_SPV_SIZE);
    if (!pr.vertShaderModule || !pr.fragShaderModule) {
        LOG_ERROR("Template3D", "创建 present 着色器模块失败");
        return;
    }

    // 通过工厂创建抽象描述符集布局
    {
        std::vector<ShaderResource> presentResources;
        presentResources.push_back({"presentTexture", ShaderResource::Type::Sampler2D, 0, 0, 1, 0});
        pr.descriptorSetLayout = factory->CreateDescriptorSetLayout(presentResources);
        if (!pr.descriptorSetLayout) {
            LOG_ERROR("Template3D", "创建 present 描述符集布局失败");
            return;
        }
    }

    // 通过工厂创建抽象采样器
    {
        SamplerDesc samplerDesc{};
        samplerDesc.filter = TextureFilter::Linear;
        samplerDesc.addressU = TextureAddressMode::Clamp;
        samplerDesc.addressV = TextureAddressMode::Clamp;
        samplerDesc.addressW = TextureAddressMode::Clamp;
        pr.sampler = factory->CreateSamplerImpl(samplerDesc);
        if (!pr.sampler) {
            LOG_ERROR("Template3D", "创建 sampler 失败");
            return;
        }
    }

    // 通过工厂创建抽象描述符集
    pr.descriptorSet = factory->CreateDescriptorSet(pr.descriptorSetLayout.get());
    if (!pr.descriptorSet) {
        LOG_ERROR("Template3D", "分配 present 描述符集失败");
        return;
    }

    VkPipelineLayoutCreateInfo plCI{};
    plCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plCI.setLayoutCount = 1;
    VkDescriptorSetLayout vkLayout = (VkDescriptorSetLayout)pr.descriptorSetLayout->GetNativeHandle();
    plCI.pSetLayouts = &vkLayout;
    err = vkCreatePipelineLayout(vkDev, &plCI, nullptr, &pr.pipelineLayout);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建 present 管线布局失败");
        return;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = pr.vertShaderModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = pr.fragShaderModule;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo viCI{};
    viCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo iaCI{};
    iaCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynCI{};
    dynCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynCI.dynamicStateCount = 2;
    dynCI.pDynamicStates = dynStates;

    VkPipelineViewportStateCreateInfo vpCI{};
    vpCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpCI.viewportCount = 1;
    vpCI.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasCI{};
    rasCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasCI.cullMode = VK_CULL_MODE_NONE;
    rasCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasCI.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo msCI{};
    msCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo dsCI{};
    dsCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsCI.depthTestEnable = VK_FALSE;
    dsCI.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState cbAtt{};
    cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cbAtt.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo cbCI{};
    cbCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbCI.attachmentCount = 1;
    cbCI.pAttachments = &cbAtt;

    VkRenderPass overlayRP = m_device->GetOverlayRenderPass();

    VkGraphicsPipelineCreateInfo gpCI{};
    gpCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpCI.stageCount = 2;
    gpCI.pStages = stages;
    gpCI.pVertexInputState = &viCI;
    gpCI.pInputAssemblyState = &iaCI;
    gpCI.pViewportState = &vpCI;
    gpCI.pRasterizationState = &rasCI;
    gpCI.pMultisampleState = &msCI;
    gpCI.pDepthStencilState = &dsCI;
    gpCI.pColorBlendState = &cbCI;
    gpCI.pDynamicState = &dynCI;
    gpCI.layout = pr.pipelineLayout;
    gpCI.renderPass = overlayRP;
    gpCI.subpass = 0;

    err = vkCreateGraphicsPipelines(vkDev, VK_NULL_HANDLE, 1, &gpCI, nullptr, &pr.pipeline);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建 present 图形管线失败");
        return;
    }

    // 通过抽象接口绑定纹理到描述符集
    pr.descriptorSet->BindTexture(0, m_ptRes.storageTexture.get(), pr.sampler.get());
    pr.descriptorSet->Update();

    auto vkRenderDev = dynamic_cast<Graphic::Vulkan::RenderDeviceVulkan*>(m_device);
    if (vkRenderDev) {
        vkRenderDev->SetOverlayRenderCallback([this](VkCommandBuffer cmd) {
            OnPresentOverlay(cmd);
        });
    }

    pr.extent = { m_Spec.Width, m_Spec.Height };
    pr.initialized = true;
    LOG_INFO("Template3D", "Present 资源初始化完成");
}

void Template3DApp::OnRender() {
    if (m_renderMode == RenderMode::PathTracing && m_ptRes.initialized &&
        (m_headlessCfg.enabled || m_presentRes.initialized)) {
        RenderPathTracing();
    } else {
        RenderForward3D();
    }
}

void Template3DApp::RenderPathTracing() {
    auto& pt = m_ptRes;

    auto vkRenderDev = dynamic_cast<Graphic::Vulkan::RenderDeviceVulkan*>(m_device);
    if (!vkRenderDev) return;

    // 获取抽象命令缓冲区（无需 dynamic_cast 到 VulkanCommandBuffer）
    auto* cmdBuffer = vkRenderDev->GetCurrentCommandBuffer();
    if (!cmdBuffer) return;

    if (!vkRenderDev->IsHeadless()) {
        vkRenderDev->SuspendDefaultRenderPass();
    }

    bool firstFrame = (pt.frameCount == 0);

    // 1. 管线屏障：将存储图像转换到 GENERAL 布局供计算着色器写入
    cmdBuffer->PipelineBarrier({{
        pt.storageTexture.get(),
        firstFrame ? Graphic::ResourceState::Undefined : Graphic::ResourceState::ShaderRead,
        Graphic::ResourceState::UnorderedAccess
    }});

    // 2. 填充 Camera UBO（通过抽象接口自动处理 map/flush/unmap）
    {
        glm::vec3 dir = glm::normalize(m_camera.target - m_camera.position);
        glm::vec3 right = glm::normalize(glm::cross(dir, m_camera.up));
        glm::vec3 up = glm::normalize(glm::cross(right, dir));

        CameraUBO ubo{};
        std::memcpy(ubo.cameraPos, &m_camera.position, sizeof(float) * 3);
        std::memcpy(ubo.cameraDir, &dir, sizeof(float) * 3);
        std::memcpy(ubo.cameraUp, &up, sizeof(float) * 3);
        std::memcpy(ubo.cameraRight, &right, sizeof(float) * 3);
        ubo.fov = glm::radians(m_camera.fov);
        ubo.aspectRatio = (float)pt.width / (float)pt.height;
        ubo.frameCount = (int)pt.frameCount;
        ubo.maxBounces = 8;
        ubo.samplesPerPixel = 1;
        ubo.useAccumulation = 1;
        ubo.resetAccumulation = m_pathTracingDirty ? 1 : 0;
        ubo.padUBO = 0;

        pt.cameraUBO->UpdateData(&ubo, sizeof(ubo), 0);
    }

    // 3. 绑定计算管线和描述符集，调度
    cmdBuffer->SetComputePipeline(pt.computePipeline.get());
    cmdBuffer->BindDescriptorSet(0, pt.descriptorSet.get());

    uint32_t groupX = (pt.width + 7) / 8;
    uint32_t groupY = (pt.height + 7) / 8;
    cmdBuffer->Dispatch(groupX, groupY, 1);

    // 4. 管线屏障：将存储图像转换到 ShaderRead 供展示
    cmdBuffer->PipelineBarrier({{
        pt.storageTexture.get(),
        Graphic::ResourceState::UnorderedAccess,
        Graphic::ResourceState::ShaderRead
    }});

    pt.frameCount++;
    m_pathTracingDirty = false;
}

void Template3DApp::OnPresentOverlay(VkCommandBuffer cmd) {
    auto& pr = m_presentRes;
    if (!pr.initialized) return;

    VkViewport vp{};
    vp.width = (float)pr.extent.width;
    vp.height = (float)pr.extent.height;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D sc{};
    sc.extent = pr.extent;
    vkCmdSetScissor(cmd, 0, 1, &sc);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pr.pipeline);
    // 通过抽象描述符集的原生句柄进行 Vulkan 绑定
    VkDescriptorSet vkDescSet = (VkDescriptorSet)pr.descriptorSet->GetNativeHandle();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pr.pipelineLayout, 0, 1, &vkDescSet, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

void Template3DApp::SavePathTracingOutput() {
    auto vkRenderDev = dynamic_cast<Graphic::Vulkan::RenderDeviceVulkan*>(m_device);
    if (!vkRenderDev || !vkRenderDev->IsInitialized()) {
        LOG_ERROR("Template3D", "保存输出失败：渲染设备未初始化");
        return;
    }

    VkDevice vkDev = vkRenderDev->GetVkDevice();
    if (!vkDev) return;

    vkDeviceWaitIdle(vkDev);

    auto& pt = m_ptRes;
    uint32_t w = pt.width;
    uint32_t h = pt.height;
    size_t pixelCount = size_t(w) * h;
    size_t bufSize = pixelCount * 4 * sizeof(float);

    auto* vkTexture = dynamic_cast<Graphic::Vulkan::VulkanTexture*>(pt.storageTexture.get());
    if (!vkTexture) {
        LOG_ERROR("Template3D", "无法获取 Vulkan 纹理句柄");
        return;
    }

    std::vector<float> pixels(pixelCount * 4);

    if (!vkRenderDev->ReadbackImage(vkTexture->GetVkImage(), w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
                                    pixels.data(), bufSize)) {
        LOG_ERROR("Template3D", "图像回读失败");
        return;
    }

    std::vector<uint8_t> rgba8(pixelCount * 4);
    for (size_t i = 0; i < pixelCount; i++) {
        float r = std::clamp(pixels[i * 4 + 0], 0.0f, 1.0f);
        float g = std::clamp(pixels[i * 4 + 1], 0.0f, 1.0f);
        float b = std::clamp(pixels[i * 4 + 2], 0.0f, 1.0f);
        float a = std::clamp(pixels[i * 4 + 3], 0.0f, 1.0f);

        r = powf(r, 1.0f / 2.2f);
        g = powf(g, 1.0f / 2.2f);
        b = powf(b, 1.0f / 2.2f);

        rgba8[i * 4 + 0] = uint8_t(r * 255.0f + 0.5f);
        rgba8[i * 4 + 1] = uint8_t(g * 255.0f + 0.5f);
        rgba8[i * 4 + 2] = uint8_t(b * 255.0f + 0.5f);
        rgba8[i * 4 + 3] = uint8_t(a * 255.0f + 0.5f);
    }

    const std::string& outPath = m_headlessCfg.outputPath;
    int result = stbi_write_png(outPath.c_str(), int(w), int(h), 4, rgba8.data(), int(w) * 4);
    if (result) {
        LOG_INFO("Template3D", "路径追踪输出已保存: {} ({}x{}, {} frames)",
                 outPath, w, h, m_ptRes.frameCount);
    } else {
        LOG_ERROR("Template3D", "保存 PNG 失败: {}", outPath);
    }
}

void Template3DApp::RenderForward3D() {
    auto* scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    if (!camera) return;

    Graphic::Renderer2D::BeginGizmo(*std::dynamic_pointer_cast<Graphic::OrthographicCamera>(camera).get());

    Graphic::Renderer2D::DrawString("Forward3D Mode — Press P for Path Tracing",
                                    {30.0f, 30.0f}, 2.0f, {0.6f, 0.6f, 0.6f, 1.0f});

    Graphic::Renderer2D::EndGizmo();
}

void Template3DApp::OnUpdate(Timestep ts) {
    (void)ts;

    if (m_headlessCfg.enabled && m_ptRes.initialized) {
        if (m_ptRes.frameCount >= m_headlessCfg.totalFrames) {
            LOG_INFO("Template3D", "头模式：累积完成 ({} 帧)，正在保存输出...",
                     m_ptRes.frameCount);
            SavePathTracingOutput();
            Close();
        }
    }
}

void Template3DApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) {
            Close();
            return true;
        }
        if (ev.GetKeyCode() == SDL_SCANCODE_P && !ev.IsRepeat()) {
            m_renderMode = (m_renderMode == RenderMode::PathTracing)
                           ? RenderMode::Forward3D
                           : RenderMode::PathTracing;
            m_pathTracingDirty = true;
            m_ptRes.frameCount = 0;
            LOG_INFO("Template3D", "切换到 {} 模式",
                     m_renderMode == RenderMode::PathTracing ? "路径追踪" : "Forward3D");
            return true;
        }
        if (ev.GetKeyCode() == SDL_SCANCODE_R && !ev.IsRepeat()) {
            m_pathTracingDirty = true;
            m_ptRes.frameCount = 0;
            LOG_INFO("Template3D", "重置路径追踪累积");
            return true;
        }
        return false;
    });
}

void Template3DApp::OnShutdown() {
    if (m_device) {
        m_device->WaitForIdle();
    }

    auto vkRenderDev = dynamic_cast<Graphic::Vulkan::RenderDeviceVulkan*>(m_device);
    if (vkRenderDev) {
        vkRenderDev->SetOverlayRenderCallback(nullptr);
    }

    if (!m_headlessCfg.enabled) {
        CleanupPresentResources();
    }
    CleanupPathTracingResources();

    LOG_INFO("Template3D", "应用已关闭");
}

void Template3DApp::CleanupPathTracingResources() {
    auto& pt = m_ptRes;
    // 所有资源由 unique_ptr/shared_ptr 自动销毁，无需手动 Vulkan 清理
    pt.descriptorSet.reset();
    pt.computePipeline.reset();
    pt.sceneSSBO.reset();
    pt.cameraUBO.reset();
    pt.storageTexture.reset();
    pt.initialized = false;
}

void Template3DApp::CleanupPresentResources() {
    auto& pr = m_presentRes;
    VkDevice vkDev = pr.vkDevice;
    if (!vkDev) return;

    // 原生 Vulkan 资源需要手动销毁
    if (pr.pipeline) {
        vkDestroyPipeline(vkDev, pr.pipeline, nullptr);
        pr.pipeline = VK_NULL_HANDLE;
    }
    if (pr.pipelineLayout) {
        vkDestroyPipelineLayout(vkDev, pr.pipelineLayout, nullptr);
        pr.pipelineLayout = VK_NULL_HANDLE;
    }
    if (pr.vertShaderModule) {
        vkDestroyShaderModule(vkDev, pr.vertShaderModule, nullptr);
        pr.vertShaderModule = VK_NULL_HANDLE;
    }
    if (pr.fragShaderModule) {
        vkDestroyShaderModule(vkDev, pr.fragShaderModule, nullptr);
        pr.fragShaderModule = VK_NULL_HANDLE;
    }
    // sampler / descriptorSetLayout / descriptorSet 由 shared_ptr/unique_ptr 自动清理
    pr.descriptorSet.reset();
    pr.sampler.reset();
    pr.descriptorSetLayout.reset();
    pr.initialized = false;
}

} // namespace Prisma
