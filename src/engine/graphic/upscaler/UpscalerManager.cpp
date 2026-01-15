#include "UpscalerManager.h"
#include "interfaces/IRenderDevice.h"
#include <algorithm>
#include <cmath>

// FSR 适配器
#if defined(PRISMA_ENABLE_UPSCALER_FSR)
    #include "adapters/FSR/UpscalerFSR.h"
#endif

// DLSS 适配器
#if defined(PRISMA_ENABLE_UPSCALER_DLSS)
    #include "adapters/DLSS/UpscalerDLSS.h"
#endif

// TSR 适配器
#if defined(PRISMA_ENABLE_UPSCALER_TSR)
    #include "adapters/TSR/UpscalerTSR.h"
#endif

namespace PrismaEngine::Graphic {

// ========== UpscalerManager Implementation ==========

UpscalerManager& UpscalerManager::Instance() {
    static UpscalerManager instance;
    return instance;
}

void UpscalerManager::Initialize(IRenderDevice* device) {
    if (m_initialized) {
        return;
    }

    CreateAvailableUpscalers(device);

    // 设置默认活动超分辨率器
    if (!m_upscalers.empty()) {
        auto defaultTech = GetDefaultTechnology();
        auto it = m_upscalers.find(defaultTech);
        if (it != m_upscalers.end()) {
            m_activeUpscaler = it->second.get();
        } else {
            // 如果默认技术不可用，使用第一个可用的
            m_activeUpscaler = m_upscalers.begin()->second.get();
        }
    }

    m_initialized = true;
}

void UpscalerManager::Shutdown() {
    m_upscalers.clear();
    m_activeUpscaler = nullptr;
    m_initialized = false;
}

std::vector<UpscalerTechnology> UpscalerManager::GetAvailableTechnologies() const {
    std::vector<UpscalerTechnology> technologies;
    technologies.reserve(m_upscalers.size());

    for (const auto& pair : m_upscalers) {
        technologies.push_back(pair.first);
    }

    return technologies;
}

IUpscaler* UpscalerManager::CreateUpscaler(UpscalerTechnology technology,
                                            const UpscalerInitDesc& desc) {
    // 注意：这个方法将在后续实现 FSR/DLSS/TSR 适配器后完善
    // 当前返回 nullptr，待适配器实现后补充
    auto it = m_upscalers.find(technology);
    if (it != m_upscalers.end()) {
        // 如果已经创建了实例，尝试重新初始化
        if (it->second->Initialize(desc)) {
            return it->second.get();
        }
    }
    return nullptr;
}

void UpscalerManager::SetActiveUpscaler(IUpscaler* upscaler) {
    m_activeUpscaler = upscaler;
}

IUpscaler* UpscalerManager::GetUpscaler(UpscalerTechnology technology) const {
    auto it = m_upscalers.find(technology);
    if (it != m_upscalers.end()) {
        return it->second.get();
    }
    return nullptr;
}

UpscalerTechnology UpscalerManager::GetDefaultTechnology() {
#if defined(PRISMA_PLATFORM_WINDOWS)
    // Windows: 优先 DLSS，然后 FSR
#if defined(PRISMA_ENABLE_UPSCALER_DLSS)
    return UpscalerTechnology::DLSS;
#elif defined(PRISMA_ENABLE_UPSCALER_FSR)
    return UpscalerTechnology::FSR;
#else
    return UpscalerTechnology::TSR;
#endif
#elif defined(PRISMA_PLATFORM_ANDROID)
    // Android: FSR 或 TSR（DLSS 不支持）
#if defined(PRISMA_ENABLE_UPSCALER_FSR)
    return UpscalerTechnology::FSR;
#else
    return UpscalerTechnology::TSR;
#endif
#else
    // Linux: 优先 DLSS (Vulkan)，然后 FSR
#if defined(PRISMA_ENABLE_UPSCALER_DLSS)
    return UpscalerTechnology::DLSS;
#elif defined(PRISMA_ENABLE_UPSCALER_FSR)
    return UpscalerTechnology::FSR;
#else
    return UpscalerTechnology::TSR;
#endif
#endif
}

UpscalerInfo UpscalerManager::GetTechnologyInfo(UpscalerTechnology technology) const {
    IUpscaler* upscaler = GetUpscaler(technology);
    if (upscaler) {
        return upscaler->GetInfo();
    }
    return UpscalerInfo{};
}

bool UpscalerManager::IsTechnologyAvailable(UpscalerTechnology technology) const {
    return m_upscalers.find(technology) != m_upscalers.end();
}

void UpscalerManager::CreateAvailableUpscalers(IRenderDevice* device) {
#if defined(PRISMA_ENABLE_UPSCALER_FSR)
    auto fsrUpscaler = std::make_unique<UpscalerFSR>();
    // 注意：不在这里初始化，初始化在 CreateUpscaler 中进行
    m_upscalers[UpscalerTechnology::FSR] = std::move(fsrUpscaler);
#endif

#if defined(PRISMA_ENABLE_UPSCALER_DLSS)
    auto dlssUpscaler = std::make_unique<UpscalerDLSS>();
    // 注意：不在这里初始化，初始化在 CreateUpscaler 中进行
    m_upscalers[UpscalerTechnology::DLSS] = std::move(dlssUpscaler);
#endif

#if defined(PRISMA_ENABLE_UPSCALER_TSR)
    auto tsrUpscaler = std::make_unique<UpscalerTSR>();
    // 注意：不在这里初始化，初始化在 CreateUpscaler 中进行
    m_upscalers[UpscalerTechnology::TSR] = std::move(tsrUpscaler);
#endif
}


// ========== UpscalerHelper Implementation ==========

namespace UpscalerHelper {

std::string GetTechnologyName(UpscalerTechnology technology) {
    switch (technology) {
        case UpscalerTechnology::FSR:
            return "FSR";
        case UpscalerTechnology::DLSS:
            return "DLSS";
        case UpscalerTechnology::TSR:
            return "TSR";
        case UpscalerTechnology::None:
        default:
            return "None";
    }
}

std::string GetQualityName(UpscalerQuality quality) {
    switch (quality) {
        case UpscalerQuality::UltraQuality:
            return "Ultra Quality";
        case UpscalerQuality::Quality:
            return "Quality";
        case UpscalerQuality::Balanced:
            return "Balanced";
        case UpscalerQuality::Performance:
            return "Performance";
        case UpscalerQuality::UltraPerformance:
            return "Ultra Performance";
        case UpscalerQuality::None:
        default:
            return "None";
    }
}

float GetScaleFactor(UpscalerQuality quality) {
    switch (quality) {
        case UpscalerQuality::UltraQuality:
            return 1.3f;
        case UpscalerQuality::Quality:
            return 1.5f;
        case UpscalerQuality::Balanced:
            return 1.7f;
        case UpscalerQuality::Performance:
            return 2.0f;
        case UpscalerQuality::UltraPerformance:
            return 3.0f;
        case UpscalerQuality::None:
        default:
            return 1.0f;
    }
}

void CalculateRenderResolution(UpscalerQuality quality,
                                uint32_t displayWidth,
                                uint32_t displayHeight,
                                uint32_t& outWidth,
                                uint32_t& outHeight) {
    float scaleFactor = GetScaleFactor(quality);

    if (scaleFactor <= 1.0f) {
        outWidth = displayWidth;
        outHeight = displayHeight;
        return;
    }

    // 计算渲染分辨率（向上取整以确保至少为 1 像素）
    outWidth = std::max(1u, static_cast<uint32_t>(std::ceil(displayWidth / scaleFactor)));
    outHeight = std::max(1u, static_cast<uint32_t>(std::ceil(displayHeight / scaleFactor)));

    // 确保渲染分辨率为偶数（某些超分技术要求）
    // 向上取整到最近的偶数
    if (outWidth % 2 != 0) outWidth += 1;
    if (outHeight % 2 != 0) outHeight += 1;
}

void GenerateHaltonSequence(int index, float& x, float& y) {
    // Halton 序列 (2, 3) - 用于低差异采样
    // 生成高质量的抖动序列，减少采样模式中的重复

    // Halton(2, index) - X 分量
    float xValue = 0.0f;
    float f = 0.5f;
    int i = index;
    while (i > 0) {
        xValue += f * (i % 2);
        i = i / 2;
        f = f * 0.5f;
    }

    // Halton(3, index) - Y 分量
    float yValue = 0.0f;
    f = 1.0f / 3.0f;
    i = index;
    while (i > 0) {
        yValue += f * (i % 3);
        i = i / 3;
        f = f / 3.0f;
    }

    // 转换到 [-0.5, 0.5] 范围（相对于像素中心）
    x = xValue - 0.5f;
    y = yValue - 0.5f;
}

} // namespace UpscalerHelper

} // namespace PrismaEngine::Graphic
