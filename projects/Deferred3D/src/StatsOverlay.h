#pragma once

#include "core/Component.h"
#include <string>
#include <memory>

namespace Prisma {

struct ApplicationSpecification;

class StatsOverlay : public Component {
public:
    StatsOverlay(const ApplicationSpecification& spec);
    ~StatsOverlay() override = default;

    ComponentId GetComponentId() const override { return GetComponentTypeId<StatsOverlay>(); }
    const char* GetComponentTypeName() const override { return "StatsOverlay"; }
    void Update(Timestep ts) override;

private:
    const ApplicationSpecification& m_spec;

    std::string m_overlayTimingInfo = "Calculating...";
    std::string m_overlayStatusStr = "Loading...";
    std::string m_overlayResInfo;
    std::string m_gBufferInfo;
    std::string m_passInfo;
    float m_overlayRefreshTimer = 0.0f;
    double m_overlayLastTime = 0.0;
};

} // namespace Prisma
