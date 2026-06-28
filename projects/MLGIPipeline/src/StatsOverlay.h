#pragma once

#include "core/Component.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include "SampleSource.h"
#include <string>
#include <memory>

namespace Prisma {

class MLGISystem;
struct ApplicationSpecification;

class StatsOverlay : public Component {
public:
    StatsOverlay(const ApplicationSpecification& spec, Graphic::PathTracingPipeline* pipeline,
                 bool enableNEE, bool usePrimitiveSphere, SampleSource* sampleSource = nullptr,
                 MLGISystem* mlgiSystem = nullptr);
    ~StatsOverlay() override = default;

    ComponentId GetComponentId() const override { return GetComponentTypeId<StatsOverlay>(); }
    const char* GetComponentTypeName() const override { return "StatsOverlay"; }
    void Update(Timestep ts) override;

    void SetNEEEnabled(bool enabled) { m_enableNEE = enabled; }
    void SetUsePrimitiveSphere(bool use) { m_usePrimitiveSphere = use; }

private:
    const ApplicationSpecification& m_spec;
    Graphic::PathTracingPipeline* m_pipeline;

    std::string m_overlayTimingInfo = "Calculating...";
    std::string m_overlayStatusStr = "Loading...";
    std::string m_overlayResInfo;
    PrismaMath::vec4 m_overlayStatusColor = {0.2f, 1.0f, 0.2f, 1.0f};
    float m_overlayRefreshTimer = 0.0f;
    double m_overlayLastTime = 0.0;
    double m_accumStartTime = 0.0;
    uint32_t m_cachedSPS = 0;

    bool m_enableNEE = false;
    bool m_usePrimitiveSphere = true;

    SampleSource* m_sampleSource = nullptr;
    MLGISystem* m_mlgiSystem = nullptr;

    std::string m_mlgiMetricsStr;
    std::string m_mlgiFallbackStr;
    PrismaMath::vec4 m_mlgiColor = {0.2f, 1.0f, 0.2f, 1.0f};
};

} // namespace Prisma
