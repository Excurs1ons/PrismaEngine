#include "UIPass.h"
#include "UIComponent.h"
#include "ShaderVulkan.h"
#include "AndroidOut.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <stdexcept>
#include <cstring>

namespace PrismaEngine {

// ============================================================
// 配置方法实现
// ============================================================

void UIPass::addUIComponent(std::shared_ptr<UIComponent> component) {
    if (component) {
        m_uiComponents.push_back(component);
        m_vertexDataDirty = true;  // 顶点数据需要更新
    }
}

void UIPass::setSwapChainExtent(VkExtent2D extent) {
    swapChainExtent_ = extent;
}

void UIPass::setAndroidApp(android_app* app) {
    app_ = app;
}

void UIPass::setPhysicalDevice(VkPhysicalDevice physicalDevice) {
    physicalDevice_ = physicalDevice;
}

void UIPass::setCurrentFrame(uint32_t currentFrame) {
    currentFrame_ = currentFrame;
}

// ============================================================
// RenderPass 接口实现
// ============================================================

void UIPass::initialize(VkDevice device, VkRenderPass renderPass) {
    device_ = device;  // 保存 device 引用
    createPipeline(device, renderPass);
    createVertexBuffer(device);
}

void UIPass::cleanup(VkDevice device) {
    // 清理 Pipeline
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    // 清理顶点缓冲区
    if (vertexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer_, nullptr);
        vertexBuffer_ = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexBufferMemory_, nullptr);
        vertexBufferMemory_ = VK_NULL_HANDLE;
    }
}

// ============================================================
// Pipeline 创建
// ============================================================

void UIPass::createPipeline(VkDevice device, VkRenderPass renderPass) {
    if (app_ == nullptr) {
        throw std::runtime_error("UIPass::createPipeline: android_app not set!");
    }

    // 加载 UI 着色器
    aout << "正在加载 UI shader..." << std::endl;
    auto vertShaderCode = ShaderVulkan::loadShader(app_->activity->assetManager, "shaders/ui.vert.spv");
    auto fragShaderCode = ShaderVulkan::loadShader(app_->activity->assetManager, "shaders/ui.frag.spv");

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        throw std::runtime_error("Failed to load UI shader files!");
    }

    aout << "成功加载 UI shader!" << std::endl;

    // 创建着色器模块
    VkShaderModule vertShaderModule;
    VkShaderModuleCreateInfo vertCreateInfo{};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertCreateInfo.codeSize = vertShaderCode.size() * 4;
    vertCreateInfo.pCode = vertShaderCode.data();
    if (vkCreateShaderModule(device, &vertCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create UI vertex shader module!");
    }

    VkShaderModule fragShaderModule;
    VkShaderModuleCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragCreateInfo.codeSize = fragShaderCode.size() * 4;
    fragCreateInfo.pCode = fragShaderCode.data();
    if (vkCreateShaderModule(device, &fragCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        throw std::runtime_error("Failed to create UI fragment shader module!");
    }

    // 着色器阶段
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 顶点输入状态（与 ui.vert 匹配：position, uv, color）
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(UIVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    // location 0: inPosition (vec2)
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(UIVertex, position);

    // location 1: inUV (vec2)
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(UIVertex, uv);

    // location 2: inColor (vec4)
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(UIVertex, color);

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // 输入装配状态（使用 TRIANGLE_LIST）
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport 和 Scissor（全屏）
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent_.width;
    viewport.height = (float) swapChainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent_;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // 光栅化状态（无背面剔除，双面渲染）
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // 无背面剔除
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // 多重采样状态
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 颜色混合状态（UI 需要透明混合）
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 深度/模板状态（UI 不需要深度测试）
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    // Pipeline Layout（UI 不需要 descriptor sets）
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        throw std::runtime_error("Failed to create UI pipeline layout!");
    }

    // 创建 Graphics Pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        throw std::runtime_error("Failed to create UI graphics pipeline!");
    }

    // 清理着色器模块
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    aout << "UIPass: Graphics pipeline created successfully." << std::endl;
}

// ============================================================
// 顶点缓冲区管理
// ============================================================

uint32_t UIPass::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("UIPass: Failed to find suitable memory type!");
}

void UIPass::createVertexBuffer(VkDevice device) {
    // 首次创建时分配缓冲区
    // 实际数据在 record() 时更新
    VkDeviceSize bufferSize = sizeof(UIVertex) * 10000;  // 预分配足够空间

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create UI vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, vertexBuffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory_) != VK_SUCCESS) {
        vkDestroyBuffer(device, vertexBuffer_, nullptr);
        throw std::runtime_error("Failed to allocate UI vertex buffer memory!");
    }

    vkBindBufferMemory(device, vertexBuffer_, vertexBufferMemory_, 0);

    aout << "UIPass: Vertex buffer created." << std::endl;
}

void UIPass::updateVertexBuffer(VkDevice device) {
    if (!m_vertexDataDirty) {
        return;
    }

    m_vertexData.clear();

    static int updateCount = 0;
    if (updateCount < 3) {
        aout << "UIPass: 更新顶点缓冲区，UI 组件数量: " << m_uiComponents.size() << std::endl;
        updateCount++;
    }

    // 遍历所有 UI 组件，生成顶点数据
    for (const auto& component : m_uiComponents) {
        if (!component || !component->IsVisible()) {
            continue;
        }

        const auto& pos = component->GetScreenPosition();
        const auto& size = component->GetSize();
        const auto& color = component->GetColor();

        // 调试输出
        static int debugCount = 0;
        if (debugCount < 10) {
            aout << "UI 组件: pos=(" << pos.x << ", " << pos.y
                 << ") size=(" << size.x << "x" << size.y
                 << ") color=(" << color.x << ", " << color.y << ", " << color.z << ", " << color.w << ")" << std::endl;
            debugCount++;
        }

        // 将屏幕坐标转换为 NDC (-1 到 1)
        // 屏幕坐标系：左上角为原点，X 向右，Y 向下
        // NDC：中心为原点，X 向右，Y 向上
        float ndcLeft = (pos.x / swapChainExtent_.width) * 2.0f - 1.0f;
        float ndcRight = ((pos.x + size.x) / swapChainExtent_.width) * 2.0f - 1.0f;
        float ndcTop = 1.0f - (pos.y / swapChainExtent_.height) * 2.0f;  // Y 翻转
        float ndcBottom = 1.0f - ((pos.y + size.y) / swapChainExtent_.height) * 2.0f;  // Y 翻转

        // 生成两个三角形（一个矩形）
        // Triangle 1: 左上, 左下, 右上
        m_vertexData.push_back({{ndcLeft, ndcTop}, {0.0f, 0.0f}, {color.x, color.y, color.z, color.w}});
        m_vertexData.push_back({{ndcLeft, ndcBottom}, {0.0f, 1.0f}, {color.x, color.y, color.z, color.w}});
        m_vertexData.push_back({{ndcRight, ndcTop}, {1.0f, 0.0f}, {color.x, color.y, color.z, color.w}});

        // Triangle 2: 左下, 右下, 右上
        m_vertexData.push_back({{ndcLeft, ndcBottom}, {0.0f, 1.0f}, {color.x, color.y, color.z, color.w}});
        m_vertexData.push_back({{ndcRight, ndcBottom}, {1.0f, 1.0f}, {color.x, color.y, color.z, color.w}});
        m_vertexData.push_back({{ndcRight, ndcTop}, {1.0f, 0.0f}, {color.x, color.y, color.z, color.w}});
    }

    // 上传到 GPU
    if (!m_vertexData.empty()) {
        void* data;
        VkDeviceSize bufferSize = sizeof(UIVertex) * m_vertexData.size();
        vkMapMemory(device, vertexBufferMemory_, 0, bufferSize, 0, &data);
        memcpy(data, m_vertexData.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, vertexBufferMemory_);
    }

    m_vertexDataDirty = false;

    static int debugCount = 0;
    if (debugCount < 3) {
        aout << "UIPass: Updated vertex buffer with " << m_vertexData.size() / 6 << " UI elements." << std::endl;
        debugCount++;
    }
}

// ============================================================
// 渲染命令录制
// ============================================================

void UIPass::record(VkCommandBuffer cmdBuffer) {
    static int recordCount = 0;
    if (recordCount < 3) {
        aout << "UIPass::record() 被调用，UI 组件数量: " << m_uiComponents.size() << std::endl;
        recordCount++;
    }

    if (m_uiComponents.empty()) {
        return;
    }

    // 更新顶点缓冲区（使用保存的 device 引用）
    updateVertexBuffer(device_);

    // 设置 Viewport 和 Scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent_.width;
    viewport.height = (float) swapChainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent_;
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    // 绑定 Graphics Pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    // 绑定顶点缓冲区
    VkBuffer vertexBuffers[] = {vertexBuffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);

    // 绘制
    uint32_t vertexCount = static_cast<uint32_t>(m_vertexData.size());
    if (vertexCount > 0) {
        vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
    }
}

} // namespace PrismaEngine
