#include "LogicalPipeline.h"
#include "LogicalPass.h"
#include "interfaces/IGBuffer.h"
#include <iostream>

namespace PrismaEngine::Graphic {

LogicalPipeline::LogicalPipeline(const char* name)
    : m_name(name)
    , m_autoSort(true)
    , m_renderTarget(nullptr)
    , m_depthStencil(nullptr)
    , m_width(1920)
    , m_height(1080) {
}

LogicalPipeline::~LogicalPipeline() {
    Clear();
}

bool LogicalPipeline::AddPass(IPass* pass) {
    if (!pass) {
        return false;
    }

    // 检查是否已存在
    for (auto* p : m_passes) {
        if (p == pass) {
            return false;
        }
    }

    m_passes.push_back(pass);

    // 自动排序
    if (m_autoSort) {
        SortByPriority();
    }

    return true;
}

bool LogicalPipeline::RemovePass(IPass* pass) {
    auto it = std::find(m_passes.begin(), m_passes.end(), pass);
    if (it != m_passes.end()) {
        m_passes.erase(it);
        return true;
    }
    return false;
}

IPass* LogicalPipeline::GetPass(size_t index) const {
    if (index >= m_passes.size()) {
        return nullptr;
    }
    return m_passes[index];
}

IPass* LogicalPipeline::FindPass(const char* name) const {
    if (!name) {
        return nullptr;
    }

    for (auto* pass : m_passes) {
        if (pass && std::string(pass->GetName()) == name) {
            return pass;
        }
    }
    return nullptr;
}

void LogicalPipeline::Execute(const PassExecutionContext& context) {
    // 更新执行上下文
    PassExecutionContext execContext = context;
    execContext.renderTarget = execContext.renderTarget ? execContext.renderTarget : m_renderTarget;
    execContext.depthStencil = execContext.depthStencil ? execContext.depthStencil : m_depthStencil;

    // 按优先级排序
    if (m_autoSort) {
        SortByPriority();
    }

    // 执行所有启用的 Pass
    for (auto* pass : m_passes) {
        if (!pass || !pass->IsEnabled()) {
            continue;
        }

        // 设置 Pass 的渲染目标（如果没有设置的话）
        if (context.sceneData) {
            pass->SetViewport(context.sceneData->viewport.width, context.sceneData->viewport.height);
        }

        // 执行 Pass
        pass->Execute(execContext);
    }
}

void LogicalPipeline::SetViewport(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;

    // 通知所有 Pass
    for (auto* pass : m_passes) {
        if (pass) {
            pass->SetViewport(width, height);
        }
    }
}

void LogicalPipeline::SetRenderTarget(IRenderTarget* renderTarget) {
    m_renderTarget = renderTarget;

    // 通知所有 Pass
    for (auto* pass : m_passes) {
        if (pass) {
            pass->SetRenderTarget(renderTarget);
        }
    }
}

void LogicalPipeline::SetDepthStencil(IDepthStencil* depthStencil) {
    m_depthStencil = depthStencil;

    // 通知所有 Pass
    for (auto* pass : m_passes) {
        if (pass) {
            pass->SetDepthStencil(depthStencil);
        }
    }
}

void LogicalPipeline::Clear() {
    m_passes.clear();
}

void LogicalPipeline::SortByPriority() {
    std::sort(m_passes.begin(), m_passes.end(), [](IPass* a, IPass* b) {
        if (!a) return false;
        if (!b) return true;
        return a->GetPriority() < b->GetPriority();
    });
}

// === LogicalForwardPipeline ===
// 注意：这是逻辑渲染管线，不是 Vulkan Pipeline State Object (VkPipeline)

LogicalForwardPipeline::LogicalForwardPipeline()
    : LogicalPipeline("LogicalForwardPipeline") {
    SetAutoSort(true);  // 前向渲染需要按优先级排序
}

void LogicalForwardPipeline::SetRenderTarget(IRenderTarget* renderTarget) {
    LogicalPipeline::SetRenderTarget(renderTarget);
}

void LogicalForwardPipeline::Execute(const PassExecutionContext& context) {
    // 前向渲染的执行流程：
    // 1. Depth PrePass (可选)
    // 2. Opaque Pass (不透明物体)
    // 3. Skybox Pass (天空盒)
    // 4. Transparent Pass (透明物体)
    // 5. UI Pass (用户界面)

    LogicalPipeline::Execute(context);
}

// === LogicalDeferredPipeline ===
// 注意：这是逻辑渲染管线，不是 Vulkan Pipeline State Object (VkPipeline)

LogicalDeferredPipeline::LogicalDeferredPipeline()
    : LogicalPipeline("LogicalDeferredPipeline")
    , m_gBuffer(nullptr) {
    SetAutoSort(true);
}

void LogicalDeferredPipeline::Execute(const PassExecutionContext& context) {
    if (!m_gBuffer) {
        std::cerr << "LogicalDeferredPipeline: G-Buffer not set" << std::endl;
        return;
    }

    // 延迟渲染的执行流程：
    // 1. Geometry Pass (填充 G-Buffer)
    // 2. Lighting Pass (光照计算)
    // 3. Transparency Pass (透明物体 - 使用前向渲染)
    // 4. UI Pass (用户界面)

    LogicalPipeline::Execute(context);
}

} // namespace PrismaEngine::Graphic
