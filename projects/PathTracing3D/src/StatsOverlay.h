#pragma once

#include "core/Component.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include <string>
#include <memory>

namespace Prisma {

struct ApplicationSpecification;

class StatsOverlay : public Component {
public:
    StatsOverlay(const ApplicationSpecification& spec, Graphic::PathTracingPipeline* pipeline,
                 bool enableNEE, bool usePrimitiveSphere);
    ~StatsOverlay() override = default;

    const char* GetComponentTypeName() const override { return "StatsOverlay"; }
    void Update(Timestep ts) override;

    void SetNEEEnabled(bool enabled) { m_enableNEE = enabled; }
    void SetUsePrimitiveSphere(bool use) { m_usePrimitiveSphere = use; }

private:
    const ApplicationSpecification& m_spec;
    Graphic::PathTracingPipeline* m_pipeline;

    // Stat cache (moved from PathTracing3DApp)
    std::string m_overlayTimingInfo = "Calculating...";
    std::string m_overlayStatusStr = "Loading...";
    std::string m_overlayResInfo;
    PrismaMath::vec4 m_overlayStatusColor = {0.2f, 1.0f, 0.2f, 1.0f};
    float m_overlayRefreshTimer = 0.0f;
    double m_overlayLastTime = 0.0;
    double m_accumStartTime = 0.0;
    uint32_t m_cachedSPS = 0;

    // State for NEE and Primitive status overlay
    bool m_enableNEE = false;
    bool m_usePrimitiveSphere = true;
};

} // namespace Prisma
