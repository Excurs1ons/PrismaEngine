#include "Template3DApp.h"
#include "PathtraceCompSPIRV.h"
#include "FullscreenVertSPIRV.h"
#include "PresentFragSPIRV.h"

#include "graphic/RenderSystem.h"
#include "graphic/Renderer2D.h"
#include "graphic/Renderer.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/adapters/vulkan/RenderDeviceVulkan.h"
#include "graphic/adapters/vulkan/VulkanCommandBuffer.h"
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

static void TransitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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

    // 上传 SSBO 数据
    if (m_ptRes.sceneSSBOMapped) {
        std::memcpy(m_ptRes.sceneSSBOMapped, &ssboData, sizeof(ssboData));

        VmaAllocationInfo allocInfo{};
        vmaGetAllocationInfo(m_ptRes.vmaAllocator, m_ptRes.sceneSSBOAllocation, &allocInfo);
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = allocInfo.deviceMemory;
        range.offset = allocInfo.offset;
        range.size = sizeof(ssboData);
        vkFlushMappedMemoryRanges(m_ptRes.vkDevice, 1, &range);

        LOG_INFO("Template3D", "  上传 {} 个场景对象到 SSBO ({} bytes)", ssboData.objectCount, sizeof(ssboData));

        // 验证回读 SSBO 数据
        SceneDataSSBO checkData{};
        std::memcpy(&checkData, m_ptRes.sceneSSBOMapped, sizeof(checkData));
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

    VkDevice vkDev = m_device->GetVkDevice();
    pt.vkDevice = vkDev;
    pt.vmaAllocator = m_device->GetVmaAllocator();
    pt.vkPhysicalDevice = m_device->GetPhysicalDevice();
    pt.graphicsQueue = m_device->GetGraphicsQueue();
    pt.graphicsQueueFamily = m_device->GetGraphicsQueueFamily();

    if (!vkDev || !pt.vmaAllocator) {
        LOG_ERROR("Template3D", "无法获取 Vulkan 设备句柄");
        return;
    }

    VkResult err;

    pt.computeShaderModule = CreateShaderModule(vkDev, PATHTRACE_COMP_SPV_SPV, PATHTRACE_COMP_SPV_SPV_SIZE);
    if (!pt.computeShaderModule) {
        LOG_ERROR("Template3D", "创建计算着色器模块失败");
        return;
    }

    VkImageCreateInfo imgCI{};
    imgCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgCI.imageType = VK_IMAGE_TYPE_2D;
    imgCI.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imgCI.extent = { pt.width, pt.height, 1 };
    imgCI.mipLevels = 1;
    imgCI.arrayLayers = 1;
    imgCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgCI.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    err = vmaCreateImage(pt.vmaAllocator, &imgCI, &allocCI,
                         &pt.storageImage, &pt.storageImageAllocation, nullptr);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建存储图像失败: {}", (int)err);
        return;
    }

    VkImageViewCreateInfo viewCI{};
    viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCI.image = pt.storageImage;
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.layerCount = 1;
    err = vkCreateImageView(vkDev, &viewCI, nullptr, &pt.storageImageView);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建图像视图失败");
        return;
    }

    // --- Camera UBO (仅相机 + 累积参数) ---
    VkBufferCreateInfo bufCI{};
    bufCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.size = sizeof(CameraUBO);
    bufCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo bufAllocCI{};
    bufAllocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    bufAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    err = vmaCreateBuffer(pt.vmaAllocator, &bufCI, &bufAllocCI,
                          &pt.cameraUBO, &pt.cameraUBOAllocation, nullptr);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建 Camera UBO 失败");
        return;
    }
    vmaMapMemory(pt.vmaAllocator, pt.cameraUBOAllocation, &pt.cameraUBOMapped);

    // --- Scene SSBO (场景对象数据) ---
    VkBufferCreateInfo ssboBufCI{};
    ssboBufCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ssboBufCI.size = sizeof(SceneDataSSBO);
    ssboBufCI.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VmaAllocationCreateInfo ssboAllocCI{};
    ssboAllocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    ssboAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    err = vmaCreateBuffer(pt.vmaAllocator, &ssboBufCI, &ssboAllocCI,
                          &pt.sceneSSBO, &pt.sceneSSBOAllocation, nullptr);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建 Scene SSBO 失败");
        return;
    }
    vmaMapMemory(pt.vmaAllocator, pt.sceneSSBOAllocation, &pt.sceneSSBOMapped);

    // 清空 SSBO（防止未初始化的对象数据）
    SceneDataSSBO emptySSBO{};
    std::memcpy(pt.sceneSSBOMapped, &emptySSBO, sizeof(emptySSBO));
    VmaAllocationInfo ssboInfo{};
    vmaGetAllocationInfo(pt.vmaAllocator, pt.sceneSSBOAllocation, &ssboInfo);
    VkMappedMemoryRange ssboRange{};
    ssboRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    ssboRange.memory = ssboInfo.deviceMemory;
    ssboRange.offset = ssboInfo.offset;
    ssboRange.size = sizeof(SceneDataSSBO);
    vkFlushMappedMemoryRanges(pt.vkDevice, 1, &ssboRange);

    // --- 描述符集布局: binding 0=output image, 1=accum image, 2=camera UBO, 3=scene SSBO ---
    VkDescriptorSetLayoutBinding computeBindings[4] = {};
    computeBindings[0].binding = 0;
    computeBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    computeBindings[0].descriptorCount = 1;
    computeBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    computeBindings[1].binding = 1;
    computeBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    computeBindings[1].descriptorCount = 1;
    computeBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    computeBindings[2].binding = 2;
    computeBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    computeBindings[2].descriptorCount = 1;
    computeBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    computeBindings[3].binding = 3;
    computeBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[3].descriptorCount = 1;
    computeBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo dslCI{};
    dslCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslCI.bindingCount = 4;
    dslCI.pBindings = computeBindings;
    err = vkCreateDescriptorSetLayout(vkDev, &dslCI, nullptr, &pt.computeDescriptorSetLayout);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建计算描述符集布局失败");
        return;
    }

    VkPipelineLayoutCreateInfo plCI{};
    plCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plCI.setLayoutCount = 1;
    plCI.pSetLayouts = &pt.computeDescriptorSetLayout;
    err = vkCreatePipelineLayout(vkDev, &plCI, nullptr, &pt.computePipelineLayout);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建计算管线布局失败");
        return;
    }

    VkComputePipelineCreateInfo cpCI{};
    cpCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpCI.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cpCI.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cpCI.stage.module = pt.computeShaderModule;
    cpCI.stage.pName = "main";
    cpCI.layout = pt.computePipelineLayout;
    err = vkCreateComputePipelines(vkDev, VK_NULL_HANDLE, 1, &cpCI, nullptr, &pt.computePipeline);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建计算管线失败");
        return;
    }

    VkDescriptorPoolSize poolSizes[4] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = 2;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = 1;

    VkDescriptorPoolCreateInfo dpCI{};
    dpCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpCI.poolSizeCount = 4;
    dpCI.pPoolSizes = poolSizes;
    dpCI.maxSets = 2;
    dpCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    err = vkCreateDescriptorPool(vkDev, &dpCI, nullptr, &pt.descriptorPool);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建描述符池失败");
        return;
    }

    VkDescriptorSetAllocateInfo dsaCI{};
    dsaCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsaCI.descriptorPool = pt.descriptorPool;
    dsaCI.descriptorSetCount = 1;
    dsaCI.pSetLayouts = &pt.computeDescriptorSetLayout;
    err = vkAllocateDescriptorSets(vkDev, &dsaCI, &pt.descriptorSet);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "分配计算描述符集失败");
        return;
    }

    // --- 写入描述符集 ---
    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = pt.storageImageView;

    VkWriteDescriptorSet writeOutput{};
    writeOutput.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeOutput.dstSet = pt.descriptorSet;
    writeOutput.dstBinding = 0;
    writeOutput.descriptorCount = 1;
    writeOutput.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeOutput.pImageInfo = &imgInfo;

    VkWriteDescriptorSet writeAccum{};
    writeAccum.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeAccum.dstSet = pt.descriptorSet;
    writeAccum.dstBinding = 1;
    writeAccum.descriptorCount = 1;
    writeAccum.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeAccum.pImageInfo = &imgInfo;

    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = pt.cameraUBO;
    uboInfo.range = sizeof(CameraUBO);

    VkWriteDescriptorSet writeUBO{};
    writeUBO.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeUBO.dstSet = pt.descriptorSet;
    writeUBO.dstBinding = 2;
    writeUBO.descriptorCount = 1;
    writeUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeUBO.pBufferInfo = &uboInfo;

    VkDescriptorBufferInfo ssboInfoDesc{};
    ssboInfoDesc.buffer = pt.sceneSSBO;
    ssboInfoDesc.range = sizeof(SceneDataSSBO);

    VkWriteDescriptorSet writeSSBO{};
    writeSSBO.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSSBO.dstSet = pt.descriptorSet;
    writeSSBO.dstBinding = 3;
    writeSSBO.descriptorCount = 1;
    writeSSBO.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeSSBO.pBufferInfo = &ssboInfoDesc;

    VkWriteDescriptorSet writes[] = { writeOutput, writeAccum, writeUBO, writeSSBO };
    vkUpdateDescriptorSets(vkDev, 4, writes, 0, nullptr);

    pt.initialized = true;
    LOG_INFO("Template3D", "路径追踪资源初始化完成 ({}x{})", pt.width, pt.height);
}

void Template3DApp::InitPresentResources() {
    auto& pr = m_presentRes;
    VkDevice vkDev = m_device->GetVkDevice();
    if (!vkDev) return;
    pr.vkDevice = vkDev;
    VkResult err;

    pr.vertShaderModule = CreateShaderModule(vkDev, FULLSCREEN_VERT_SPV_SPV, FULLSCREEN_VERT_SPV_SPV_SIZE);
    pr.fragShaderModule = CreateShaderModule(vkDev, PRESENT_FRAG_SPV_SPV, PRESENT_FRAG_SPV_SPV_SIZE);
    if (!pr.vertShaderModule || !pr.fragShaderModule) {
        LOG_ERROR("Template3D", "创建 present 着色器模块失败");
        return;
    }

    VkDescriptorSetLayoutBinding bind{};
    bind.binding = 0;
    bind.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bind.descriptorCount = 1;
    bind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dslCI{};
    dslCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslCI.bindingCount = 1;
    dslCI.pBindings = &bind;
    err = vkCreateDescriptorSetLayout(vkDev, &dslCI, nullptr, &pr.descriptorSetLayout);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建 present 描述符集布局失败");
        return;
    }

    VkSamplerCreateInfo sampCI{};
    sampCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampCI.magFilter = VK_FILTER_LINEAR;
    sampCI.minFilter = VK_FILTER_LINEAR;
    sampCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampCI.maxLod = VK_LOD_CLAMP_NONE;
    err = vkCreateSampler(vkDev, &sampCI, nullptr, &pr.sampler);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "创建 sampler 失败");
        return;
    }

    VkDescriptorSetAllocateInfo dsaCI{};
    dsaCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsaCI.descriptorPool = m_ptRes.descriptorPool;
    dsaCI.descriptorSetCount = 1;
    dsaCI.pSetLayouts = &pr.descriptorSetLayout;
    err = vkAllocateDescriptorSets(vkDev, &dsaCI, &pr.descriptorSet);
    if (err != VK_SUCCESS) {
        LOG_ERROR("Template3D", "分配 present 描述符集失败");
        return;
    }

    VkPipelineLayoutCreateInfo plCI{};
    plCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plCI.setLayoutCount = 1;
    plCI.pSetLayouts = &pr.descriptorSetLayout;
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

    VkDescriptorImageInfo presentImgInfo{};
    presentImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    presentImgInfo.imageView = m_ptRes.storageImageView;
    presentImgInfo.sampler = pr.sampler;

    VkWriteDescriptorSet writeSet{};
    writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.dstSet = pr.descriptorSet;
    writeSet.dstBinding = 0;
    writeSet.descriptorCount = 1;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSet.pImageInfo = &presentImgInfo;
    vkUpdateDescriptorSets(vkDev, 1, &writeSet, 0, nullptr);

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

    //LOG_DEBUG("Template3D", "渲染路径追踪帧 #{}", pt.frameCount);

    auto vkRenderDev = dynamic_cast<Graphic::Vulkan::RenderDeviceVulkan*>(m_device);
    if (!vkRenderDev) return;

    auto* vkCmdBuf = dynamic_cast<Graphic::Vulkan::VulkanCommandBuffer*>(
        vkRenderDev->GetCurrentCommandBuffer());
    if (!vkCmdBuf) return;
    VkCommandBuffer cmd = vkCmdBuf->GetVkCommandBuffer();

    if (!vkRenderDev->IsHeadless()) {
        vkRenderDev->SuspendDefaultRenderPass();
    }

    bool firstFrame = (pt.frameCount == 0);
    VkImageLayout srcLayout = firstFrame
        ? VK_IMAGE_LAYOUT_UNDEFINED
        : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    TransitionImageLayout(cmd, pt.storageImage,
                          srcLayout,
                          VK_IMAGE_LAYOUT_GENERAL,
                          firstFrame ? 0 : VK_ACCESS_SHADER_READ_BIT,
                          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                          firstFrame ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                     : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    // 填充 Camera UBO
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

        std::memcpy(pt.cameraUBOMapped, &ubo, sizeof(ubo));

        VmaAllocationInfo allocInfo{};
        vmaGetAllocationInfo(pt.vmaAllocator, pt.cameraUBOAllocation, &allocInfo);
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = allocInfo.deviceMemory;
        range.offset = allocInfo.offset;
        range.size = sizeof(ubo);
        vkFlushMappedMemoryRanges(pt.vkDevice, 1, &range);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pt.computePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pt.computePipelineLayout, 0, 1, &pt.descriptorSet, 0, nullptr);

    uint32_t groupX = (pt.width + 7) / 8;
    uint32_t groupY = (pt.height + 7) / 8;
    vkCmdDispatch(cmd, groupX, groupY, 1);

    TransitionImageLayout(cmd, pt.storageImage,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

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
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pr.pipelineLayout, 0, 1, &pr.descriptorSet, 0, nullptr);
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

    std::vector<float> pixels(pixelCount * 4);

    if (!vkRenderDev->ReadbackImage(pt.storageImage, w, h, VK_FORMAT_R32G32B32A32_SFLOAT,
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
    VkDevice vkDev = pt.vkDevice;
    if (!vkDev) return;

    if (pt.computePipeline) {
        vkDestroyPipeline(vkDev, pt.computePipeline, nullptr);
        pt.computePipeline = VK_NULL_HANDLE;
    }
    if (pt.computePipelineLayout) {
        vkDestroyPipelineLayout(vkDev, pt.computePipelineLayout, nullptr);
        pt.computePipelineLayout = VK_NULL_HANDLE;
    }
    if (pt.computeDescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(vkDev, pt.computeDescriptorSetLayout, nullptr);
        pt.computeDescriptorSetLayout = VK_NULL_HANDLE;
    }
    if (pt.descriptorPool) {
        vkDestroyDescriptorPool(vkDev, pt.descriptorPool, nullptr);
        pt.descriptorPool = VK_NULL_HANDLE;
    }
    if (pt.computeShaderModule) {
        vkDestroyShaderModule(vkDev, pt.computeShaderModule, nullptr);
        pt.computeShaderModule = VK_NULL_HANDLE;
    }
    if (pt.storageImageView) {
        vkDestroyImageView(vkDev, pt.storageImageView, nullptr);
        pt.storageImageView = VK_NULL_HANDLE;
    }
    if (pt.storageImage && pt.vmaAllocator) {
        vmaDestroyImage(pt.vmaAllocator, pt.storageImage, pt.storageImageAllocation);
        pt.storageImage = VK_NULL_HANDLE;
        pt.storageImageAllocation = VK_NULL_HANDLE;
    }
    // Camera UBO
    if (pt.cameraUBOMapped) {
        vmaUnmapMemory(pt.vmaAllocator, pt.cameraUBOAllocation);
        pt.cameraUBOMapped = nullptr;
    }
    if (pt.cameraUBO && pt.vmaAllocator) {
        vmaDestroyBuffer(pt.vmaAllocator, pt.cameraUBO, pt.cameraUBOAllocation);
        pt.cameraUBO = VK_NULL_HANDLE;
        pt.cameraUBOAllocation = VK_NULL_HANDLE;
    }
    // Scene SSBO
    if (pt.sceneSSBOMapped) {
        vmaUnmapMemory(pt.vmaAllocator, pt.sceneSSBOAllocation);
        pt.sceneSSBOMapped = nullptr;
    }
    if (pt.sceneSSBO && pt.vmaAllocator) {
        vmaDestroyBuffer(pt.vmaAllocator, pt.sceneSSBO, pt.sceneSSBOAllocation);
        pt.sceneSSBO = VK_NULL_HANDLE;
        pt.sceneSSBOAllocation = VK_NULL_HANDLE;
    }
    pt.initialized = false;
}

void Template3DApp::CleanupPresentResources() {
    auto& pr = m_presentRes;
    VkDevice vkDev = pr.vkDevice;
    if (!vkDev) return;

    if (pr.pipeline) {
        vkDestroyPipeline(vkDev, pr.pipeline, nullptr);
        pr.pipeline = VK_NULL_HANDLE;
    }
    if (pr.pipelineLayout) {
        vkDestroyPipelineLayout(vkDev, pr.pipelineLayout, nullptr);
        pr.pipelineLayout = VK_NULL_HANDLE;
    }
    if (pr.descriptorSetLayout) {
        vkDestroyDescriptorSetLayout(vkDev, pr.descriptorSetLayout, nullptr);
        pr.descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (pr.sampler) {
        vkDestroySampler(vkDev, pr.sampler, nullptr);
        pr.sampler = VK_NULL_HANDLE;
    }
    if (pr.vertShaderModule) {
        vkDestroyShaderModule(vkDev, pr.vertShaderModule, nullptr);
        pr.vertShaderModule = VK_NULL_HANDLE;
    }
    if (pr.fragShaderModule) {
        vkDestroyShaderModule(vkDev, pr.fragShaderModule, nullptr);
        pr.fragShaderModule = VK_NULL_HANDLE;
    }
    pr.descriptorSet = VK_NULL_HANDLE;
    pr.initialized = false;
}

} // namespace Prisma
